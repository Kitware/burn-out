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

// This file is from hog.cpp in OpenCV

#ifndef _sthog_cache_H_
#define _sthog_cache_H_

#include <opencv2/core/core_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>

namespace cv
{

struct HOGCache
{
  struct BlockData
  {
    BlockData() : histOfs(0), imgOffset() {}
    int histOfs;
    Point imgOffset;
  };

  struct PixData
  {
    size_t gradOfs, qangleOfs;
    int histOfs[4];
    float histWeights[4];
    float gradWeight;
  };

  HOGCache();
  HOGCache(const HOGDescriptor* descriptor,
           const Mat& img, Size paddingTL, Size paddingBR,
           bool useCache, Size cacheStride);
  virtual ~HOGCache() {};
  virtual void init(const HOGDescriptor* descriptor,
                    const Mat& img, Size paddingTL, Size paddingBR,
                    bool useCache, Size cacheStride);

  Size windowsInImage(Size imageSize, Size winStride) const;
  Rect getWindow(Size imageSize, Size winStride, int idx) const;

  const float* getBlock(Point pt, float* buf);
  virtual void normalizeBlockHistogram(float* histogram) const;

  std::vector<PixData> pixData;
  std::vector<BlockData> blockData;

  bool useCache;
  std::vector<int> ymaxCached;
  Size winSize, cacheStride;
  Size nblocks, ncells;
  int blockHistogramSize;
  int count1, count2, count4;
  Point imgoffset;
  Mat_<float> blockCache;
  Mat_<uchar> blockCacheFlags;

  Mat grad, qangle;
  const HOGDescriptor* descriptor;
};

}

#endif
