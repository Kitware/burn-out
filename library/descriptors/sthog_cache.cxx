/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

// This file is similar to HOGCache.cxx in OpenCV

#include <descriptors/sthog_cache.h>

using namespace std;

namespace cv
{

HOGCache::HOGCache()
{
  useCache = false;
  blockHistogramSize = count1 = count2 = count4 = 0;
  descriptor = 0;
}

HOGCache::HOGCache(const HOGDescriptor* _descriptor,
        const Mat& _img, Size _paddingTL, Size _paddingBR,
        bool _useCache, Size _cacheStride)
{
  init(_descriptor, _img, _paddingTL, _paddingBR, _useCache, _cacheStride);
}

void HOGCache::init(const HOGDescriptor* _descriptor,
        const Mat& _img, Size _paddingTL, Size _paddingBR,
        bool _useCache, Size _cacheStride)
{
  descriptor = _descriptor;
  cacheStride = _cacheStride;
  useCache = _useCache;

  descriptor->computeGradient(_img, grad, qangle, _paddingTL, _paddingBR);
  imgoffset = _paddingTL;

  winSize = descriptor->winSize;
  Size blockSize = descriptor->blockSize;
  Size blockStride = descriptor->blockStride;
  Size cellSize = descriptor->cellSize;
  int i, j, nbins = descriptor->nbins;
  int rawBlockSize = blockSize.width*blockSize.height;

  nblocks = Size((winSize.width - blockSize.width)/blockStride.width + 1,
                 (winSize.height - blockSize.height)/blockStride.height + 1);
  ncells = Size(blockSize.width/cellSize.width, blockSize.height/cellSize.height);
  blockHistogramSize = ncells.width*ncells.height*nbins;

  if( useCache )
  {
    Size cacheSize((grad.cols - blockSize.width)/cacheStride.width+1,
                   (winSize.height/cacheStride.height)+1);
    blockCache.create(cacheSize.height, cacheSize.width*blockHistogramSize);
    blockCacheFlags.create(cacheSize);
    size_t cacheRows = blockCache.rows;
    ymaxCached.resize(cacheRows);
    for(size_t ii = 0; ii < cacheRows; ii++ )
      ymaxCached[ii] = -1;
  }

  Mat_<float> weights(blockSize);
  float sigma = static_cast<float>(descriptor->getWinSigma());
  float scale = 1.f/(sigma*sigma*2);

  for(i = 0; i < blockSize.height; i++)
    for(j = 0; j < blockSize.width; j++)
    {
      float di = i - blockSize.height*0.5f;
      float dj = j - blockSize.width*0.5f;
      weights(i,j) = std::exp(-(di*di + dj*dj)*scale);
    }

  blockData.resize(nblocks.width*nblocks.height);
  pixData.resize(rawBlockSize*3);

  // Initialize 2 lookup tables, pixData & blockData.
  // Here is why:
  //
  // The detection algorithm runs in 4 nested loops (at each pyramid layer):
  //  loop over the windows within the input image
  //    loop over the blocks within each window
  //      loop over the cells within each block
  //        loop over the pixels in each cell
  //
  // As each of the loops runs over a 2-dimensional array,
  // we could get 8(!) nested loops in total, which is very-very slow.
  //
  // To speed the things up, we do the following:
  //   1. loop over windows is unrolled in the HOGDescriptor::{compute|detect} methods;
  //         inside we compute the current search window using getWindow() method.
  //         Yes, it involves some overhead (function call + couple of divisions),
  //         but it's tiny in fact.
  //   2. loop over the blocks is also unrolled. Inside we use pre-computed blockData[j]
  //         to set up gradient and histogram pointers.
  //   3. loops over cells and pixels in each cell are merged
  //       (since there is no overlap between cells, each pixel in the block is processed once)
  //      and also unrolled. Inside we use PixData[k] to access the gradient values and
  //      update the histogram
  //
  count1 = count2 = count4 = 0;
  for( j = 0; j < blockSize.width; j++ )
    for( i = 0; i < blockSize.height; i++ )
    {
      PixData* data = 0;
      float cellX = (j+0.5f)/cellSize.width - 0.5f;
      float cellY = (i+0.5f)/cellSize.height - 0.5f;
      int icellX0 = cvFloor(cellX);
      int icellY0 = cvFloor(cellY);
      int icellX1 = icellX0 + 1, icellY1 = icellY0 + 1;
      cellX -= icellX0;
      cellY -= icellY0;

      if( static_cast<unsigned>(icellX0) < static_cast<unsigned>(ncells.width) &&
          static_cast<unsigned>(icellX1) < static_cast<unsigned>(ncells.width) )
      {
        if( static_cast<unsigned>(icellY0) < static_cast<unsigned>(ncells.height) &&
            static_cast<unsigned>(icellY1) < static_cast<unsigned>(ncells.height) )
        {
          data = &pixData[rawBlockSize*2 + (count4++)];
          data->histOfs[0] = (icellX0*ncells.height + icellY0)*nbins;
          data->histWeights[0] = (1.f - cellX)*(1.f - cellY);
          data->histOfs[1] = (icellX1*ncells.height + icellY0)*nbins;
          data->histWeights[1] = cellX*(1.f - cellY);
          data->histOfs[2] = (icellX0*ncells.height + icellY1)*nbins;
          data->histWeights[2] = (1.f - cellX)*cellY;
          data->histOfs[3] = (icellX1*ncells.height + icellY1)*nbins;
          data->histWeights[3] = cellX*cellY;
        }
        else
        {
          data = &pixData[rawBlockSize + (count2++)];
          if( static_cast<unsigned>(icellY0) < static_cast<unsigned>(ncells.height) )
          {
            icellY1 = icellY0;
            cellY = 1.f - cellY;
          }
          data->histOfs[0] = (icellX0*ncells.height + icellY1)*nbins;
          data->histWeights[0] = (1.f - cellX)*cellY;
          data->histOfs[1] = (icellX1*ncells.height + icellY1)*nbins;
          data->histWeights[1] = cellX*cellY;
          data->histOfs[2] = data->histOfs[3] = 0;
          data->histWeights[2] = data->histWeights[3] = 0;
        }
      }
      else
      {
        if( static_cast<unsigned>(icellX0) < static_cast<unsigned>(ncells.width) )
        {
          icellX1 = icellX0;
          cellX = 1.f - cellX;
        }

        if( static_cast<unsigned>(icellY0) < static_cast<unsigned>(ncells.height) &&
            static_cast<unsigned>(icellY1) < static_cast<unsigned>(ncells.height) )
        {
          data = &pixData[rawBlockSize + (count2++)];
          data->histOfs[0] = (icellX1*ncells.height + icellY0)*nbins;
          data->histWeights[0] = cellX*(1.f - cellY);
          data->histOfs[1] = (icellX1*ncells.height + icellY1)*nbins;
          data->histWeights[1] = cellX*cellY;
          data->histOfs[2] = data->histOfs[3] = 0;
          data->histWeights[2] = data->histWeights[3] = 0;
        }
        else
        {
          data = &pixData[count1++];
          if( static_cast<unsigned>(icellY0) < static_cast<unsigned>(ncells.height) )
          {
            icellY1 = icellY0;
            cellY = 1.f - cellY;
          }
          data->histOfs[0] = (icellX1*ncells.height + icellY1)*nbins;
          data->histWeights[0] = cellX*cellY;
          data->histOfs[1] = data->histOfs[2] = data->histOfs[3] = 0;
          data->histWeights[1] = data->histWeights[2] = data->histWeights[3] = 0;
        }
      }
      data->gradOfs = (grad.cols*i + j)*2;
      data->qangleOfs = (qangle.cols*i + j)*2;
      data->gradWeight = weights(i,j);
    }

  assert( count1 + count2 + count4 == rawBlockSize );
  // defragment pixData
  for( j = 0; j < count2; j++ )
    pixData[j + count1] = pixData[j + rawBlockSize];
  for( j = 0; j < count4; j++ )
    pixData[j + count1 + count2] = pixData[j + rawBlockSize*2];
  count2 += count1;
  count4 += count2;

  // initialize blockData
  for( j = 0; j < nblocks.width; j++ )
    for( i = 0; i < nblocks.height; i++ )
    {
      BlockData& data = blockData[j*nblocks.height + i];
      data.histOfs = (j*nblocks.height + i)*blockHistogramSize;
      data.imgOffset = Point(j*blockStride.width,i*blockStride.height);
    }
}

const float* HOGCache::getBlock(Point pt, float* buf)
{
  float* blockHist = buf;
  assert(descriptor != 0);

  Size blockSize = descriptor->blockSize;
  pt += imgoffset;

  CV_Assert( static_cast<unsigned>(pt.x) <= static_cast<unsigned>((grad.cols - blockSize.width)) &&
             static_cast<unsigned>(pt.y) <= static_cast<unsigned>((grad.rows - blockSize.height)) );

  if( useCache )
  {
    CV_Assert( pt.x % cacheStride.width == 0 &&
               pt.y % cacheStride.height == 0 );
    Point cacheIdx(pt.x/cacheStride.width,
                   (pt.y/cacheStride.height) % blockCache.rows);
    if( pt.y != ymaxCached[cacheIdx.y] )
    {
      Mat_<uchar> cacheRow = blockCacheFlags.row(cacheIdx.y);
      cacheRow = static_cast<uchar>(0);
      ymaxCached[cacheIdx.y] = pt.y;
    }

    blockHist = &blockCache[cacheIdx.y][cacheIdx.x*blockHistogramSize];
    uchar& computedFlag = blockCacheFlags(cacheIdx.y, cacheIdx.x);
    if( computedFlag != 0 )
      return blockHist;
    computedFlag = static_cast<uchar>(1); // set it at once, before actual computing
  }

  int k, C1 = count1, C2 = count2, C4 = count4;
  const float* gradPtr = reinterpret_cast<const float*>(grad.data + grad.step*pt.y) + pt.x*2;
  const uchar* qanglePtr = qangle.data + qangle.step*pt.y + pt.x*2;

  CV_Assert( blockHist != 0 );
#ifdef HAVE_IPP
  ippsZero_32f(blockHist,blockHistogramSize);
#else
  for( k = 0; k < blockHistogramSize; k++ )
    blockHist[k] = 0.f;
#endif

  const PixData* _pixData = &pixData[0];

  for( k = 0; k < C1; k++ )
  {
    const PixData& pk = _pixData[k];
    const float* a = gradPtr + pk.gradOfs;
    float w = pk.gradWeight*pk.histWeights[0];
    const uchar* h = qanglePtr + pk.qangleOfs;
    int h0 = h[0], h1 = h[1];
    float* hist = blockHist + pk.histOfs[0];
    float t0 = hist[h0] + a[0]*w;
    float t1 = hist[h1] + a[1]*w;
    hist[h0] = t0; hist[h1] = t1;
  }

  for( ; k < C2; k++ )
  {
    const PixData& pk = _pixData[k];
    const float* a = gradPtr + pk.gradOfs;
    float w, t0, t1, a0 = a[0], a1 = a[1];
    const uchar* h = qanglePtr + pk.qangleOfs;
    int h0 = h[0], h1 = h[1];

    float* hist = blockHist + pk.histOfs[0];
    w = pk.gradWeight*pk.histWeights[0];
    t0 = hist[h0] + a0*w;
    t1 = hist[h1] + a1*w;
    hist[h0] = t0; hist[h1] = t1;

    hist = blockHist + pk.histOfs[1];
    w = pk.gradWeight*pk.histWeights[1];
    t0 = hist[h0] + a0*w;
    t1 = hist[h1] + a1*w;
    hist[h0] = t0; hist[h1] = t1;
  }

  for( ; k < C4; k++ )
  {
    const PixData& pk = _pixData[k];
    const float* a = gradPtr + pk.gradOfs;
    float w, t0, t1, a0 = a[0], a1 = a[1];
    const uchar* h = qanglePtr + pk.qangleOfs;
    int h0 = h[0], h1 = h[1];

    float* hist = blockHist + pk.histOfs[0];
    w = pk.gradWeight*pk.histWeights[0];
    t0 = hist[h0] + a0*w;
    t1 = hist[h1] + a1*w;
    hist[h0] = t0; hist[h1] = t1;

    hist = blockHist + pk.histOfs[1];
    w = pk.gradWeight*pk.histWeights[1];
    t0 = hist[h0] + a0*w;
    t1 = hist[h1] + a1*w;
    hist[h0] = t0; hist[h1] = t1;

    hist = blockHist + pk.histOfs[2];
    w = pk.gradWeight*pk.histWeights[2];
    t0 = hist[h0] + a0*w;
    t1 = hist[h1] + a1*w;
    hist[h0] = t0; hist[h1] = t1;

    hist = blockHist + pk.histOfs[3];
    w = pk.gradWeight*pk.histWeights[3];
    t0 = hist[h0] + a0*w;
    t1 = hist[h1] + a1*w;
    hist[h0] = t0; hist[h1] = t1;
  }

  normalizeBlockHistogram(blockHist);

  return blockHist;
}

void HOGCache::normalizeBlockHistogram(float* _hist) const
{
  float* hist = &_hist[0];
#ifdef HAVE_IPP
  size_t sz = blockHistogramSize;
#else
  size_t i, sz = blockHistogramSize;
#endif

  float sum = 0;
#ifdef HAVE_IPP
  ippsDotProd_32f(hist,hist,sz,&sum);
#else
  for( i = 0; i < sz; i++ )
    sum += hist[i]*hist[i];
#endif

  float scale = 1.f/(std::sqrt(sum)+sz*0.1f);
  float thresh = static_cast<float>(descriptor->L2HysThreshold);
#ifdef HAVE_IPP
  ippsMulC_32f_I(scale,hist,sz);
  ippsThreshold_32f_I( hist, sz, thresh, ippCmpGreater );
  ippsDotProd_32f(hist,hist,sz,&sum);
#else
  for( i = 0, sum = 0; i < sz; i++ )
  {
    hist[i] = std::min(hist[i]*scale, thresh);
    sum += hist[i]*hist[i];
  }
#endif

  scale = 1.f/(std::sqrt(sum)+1e-3f);
#ifdef HAVE_IPP
  ippsMulC_32f_I(scale,hist,sz);
#else
  for( i = 0; i < sz; i++ )
    hist[i] *= scale;
#endif
}


Size HOGCache::windowsInImage(Size imageSize, Size winStride) const
{
  return Size((imageSize.width - winSize.width)/winStride.width + 1,
              (imageSize.height - winSize.height)/winStride.height + 1);
}

Rect HOGCache::getWindow(Size imageSize, Size winStride, int idx) const
{
  int nwindowsX = (imageSize.width - winSize.width)/winStride.width + 1;
  int y = idx / nwindowsX;
  int x = idx - nwindowsX*y;
  return Rect( x*winStride.width, y*winStride.height, winSize.width, winSize.height );
}


void HOGDescriptor::compute(const Mat& img, std::vector<float>& descriptors,
                            Size winStride, Size padding,
                            const std::vector<Point>& locations) const
{
  if( winStride == Size() )
    winStride = cellSize;
  Size cacheStride(gcd(winStride.width, blockStride.width),
                   gcd(winStride.height, blockStride.height));
  size_t nwindows = locations.size();
  padding.width = static_cast<int>(alignSize(std::max(padding.width, 0), cacheStride.width));
  padding.height = static_cast<int>(alignSize(std::max(padding.height, 0), cacheStride.height));
  Size paddedImgSize(img.cols + padding.width*2, img.rows + padding.height*2);

  HOGCache cache(this, img, padding, padding, nwindows == 0, cacheStride);

  if( !nwindows )
    nwindows = cache.windowsInImage(paddedImgSize, winStride).area();

  const HOGCache::BlockData* blockData = &cache.blockData[0];

  int nblocks = cache.nblocks.area();
  int blockHistogramSize = cache.blockHistogramSize;
  size_t dsize = getDescriptorSize();
  descriptors.resize(dsize*nwindows);

  for( size_t i = 0; i < nwindows; i++ )
  {
    float* descriptor = &descriptors[i*dsize];

    Point pt0;
    if( !locations.empty() )
    {
      pt0 = locations[i];
      if( pt0.x < -padding.width || pt0.x > img.cols + padding.width - winSize.width ||
          pt0.y < -padding.height || pt0.y > img.rows + padding.height - winSize.height )
        continue;
    }
    else
    {
      pt0 = cache.getWindow(paddedImgSize, winStride, static_cast<int>(i)).tl() - Point(padding);
      CV_Assert(pt0.x % cacheStride.width == 0 && pt0.y % cacheStride.height == 0);
    }

    for( int j = 0; j < nblocks; j++ )
    {
      const HOGCache::BlockData& bj = blockData[j];
      Point pt = pt0 + bj.imgOffset;

      float* dst = descriptor + bj.histOfs;
      const float* src = cache.getBlock(pt, dst);
      if( src != dst )
#ifdef HAVE_IPP
        ippsCopy_32f(src,dst,blockHistogramSize);
#else
      for( int k = 0; k < blockHistogramSize; k++ )
        dst[k] = src[k];
#endif
    }
  }
}

}
