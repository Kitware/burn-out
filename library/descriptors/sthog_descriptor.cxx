/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/* The following function and structure are derived from OpenCV.
 * sthog_descriptor::compute() is based on HOGDescriptor::compute() in hog.cpp
 * struct sthog_invoker is from hog.cpp
 */

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

#include <stdlib.h>
#include <iostream>

#include <descriptors/sthog_cache.h>
#include <descriptors/sthog_descriptor.h>
#include <utilities/kwocv_utility.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/opencv_modules.hpp>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_sthog_descriptor_cxx__
VIDTK_LOGGER("sthog_descriptor_cxx");

namespace vidtk
{

/** Default constructor */
sthog_descriptor::sthog_descriptor()
  : HOGDescriptor(),
    hit_scores_(),
    hit_models_(),
    nframes_(1),
    sthog_map_(),
    svm_ocv_(new svm_t()),
    multi_svm_ocv_(),
    multi_svm_linear_(),
    sthog_feature_(false)
{
}

/** Constructor */
sthog_descriptor::sthog_descriptor(int _nframes, cv::Size _winSize, cv::Size _blockSize,
                                   cv::Size _blockStride, cv::Size _cellSize, int _nbins,
                                   int _derivAperture, double _winSigma, int _histogramNormType,
                                   double _L2HysThreshold, bool _gammaCorrection)
  : HOGDescriptor(_winSize, _blockSize, _blockStride, _cellSize, _nbins, _derivAperture,
                  _winSigma, _histogramNormType, _L2HysThreshold, _gammaCorrection),
    hit_scores_(),
    hit_models_(),
    nframes_(_nframes),
    sthog_map_(),
    svm_ocv_(new svm_t()),
    multi_svm_ocv_(),
    multi_svm_linear_(),
    sthog_feature_(false)
{
}

/** Constructor */
sthog_descriptor::sthog_descriptor( const sthog_profile& sp )
  : HOGDescriptor(sp.winSize_, sp.blockSize_, sp.blockStride_, sp.cellSize_, sp.nbins_,
                  sp.derivAperture_, sp.winSigma_, sp.histogramNormType_, sp.L2HysThreshold_,
                  sp.gammaCorrection_),
    hit_scores_(),
    hit_models_(),
    nframes_(sp.nframes_),
    sthog_map_(),
    svm_ocv_(new svm_t()),
    multi_svm_ocv_(),
    multi_svm_linear_(),
    sthog_feature_(false)
{
}

/** clear hits and hit_scores_ */
void sthog_descriptor::initialize_detector()
{
  hit_scores_.clear();
}

/** Get STHOG descriptor size */
size_t sthog_descriptor::get_sthog_descriptor_size() const
{
  // concatenation of 2D HOG over nframes frames
  return HOGDescriptor::getDescriptorSize() * nframes_;
}

/** Check if the descriptor size matches the number of images and 2D HOG features */
bool sthog_descriptor::check_detector_size() const
{
  size_t detectorSize = svmDetector.size(), descriptorSize = this->get_sthog_descriptor_size();
  return ( detectorSize == 0 ||
           detectorSize == descriptorSize ||
           detectorSize - descriptorSize <= static_cast<size_t>(nframes_) );
}

/** Compute HOG descriptor on an image */
void sthog_descriptor::compute(const cv::Mat& img, std::vector<float>& descriptors,
                            cv::Size winStride, cv::Size padding,
                            const std::vector<cv::Point>& locations)
{
  if( winStride == cv::Size() )
  {
    winStride = cellSize;
  }

  cv::Size cacheStride(cv::gcd(winStride.width, blockStride.width),
                       cv::gcd(winStride.height, blockStride.height));

  size_t nwindows = locations.size();
  padding.width = static_cast<int>(cv::alignSize(std::max(padding.width, 0), cacheStride.width));
  padding.height = static_cast<int>(cv::alignSize(std::max(padding.height, 0), cacheStride.height));
  cv::Size paddedImgSize(img.cols + padding.width*2, img.rows + padding.height*2);

  cv::HOGCache cache(this, img, padding, padding, nwindows == 0, cacheStride);

  if( !nwindows )
  {
    nwindows = cache.windowsInImage(paddedImgSize, winStride).area();
  }

  const cv::HOGCache::BlockData* blockData = &cache.blockData[0];

  int nblocks = cache.nblocks.area();
  int blockHistogramSize = cache.blockHistogramSize;
  size_t dsize = getDescriptorSize();
  descriptors.resize(dsize*nwindows);

  for( size_t i = 0; i < nwindows; i++ )
  {
    float* descriptor = &descriptors[i*dsize];

    cv::Point pt0;

    if( !locations.empty() )
    {
      pt0 = locations[i];

      if( pt0.x < -padding.width || pt0.x > img.cols + padding.width - winSize.width ||
          pt0.y < -padding.height || pt0.y > img.rows + padding.height - winSize.height )
      {
        continue;
      }
    }
    else
    {
      pt0 = cache.getWindow(paddedImgSize, winStride, static_cast<int>(i)).tl() - cv::Point(padding);
      LOG_ASSERT(pt0.x % cacheStride.width == 0 && pt0.y % cacheStride.height == 0, "stride step");
    }

    for( int j = 0; j < nblocks; j++ )
    {
      const cv::HOGCache::BlockData& bj = blockData[j];
      cv::Point pt = pt0 + bj.imgOffset;

      float* dst = descriptor + bj.histOfs;
      const float* src = cache.getBlock(pt, dst);

      if( src != dst )
      {
        for( int k = 0; k < blockHistogramSize; k++ )
        {
          dst[k] = src[k];
        }
      }
    }

    HOGTYPE ahog;
    float* ptr = descriptor;
    for(unsigned c=0;c<dsize;c++)
    {
      ahog.push_back(*ptr);
      ptr++;
    }

    STHOGVECTYPE ahog_vec = sthog_map_[pt0];
    ahog_vec.push_back(ahog);
    while( ahog_vec.size() > static_cast<size_t>(nframes_) )
    {
      ahog_vec.erase( ahog_vec.begin() );
    }

    sthog_map_[pt0] = ahog_vec;
  }
}

/** Compute HOG descriptor at independent points across frames */
void sthog_descriptor::compute(std::vector<cv::Mat>& imgs, const std::vector<cv::Point>& pt_in_frames)
{
  if( imgs.size() != static_cast<size_t>(nframes_) || pt_in_frames.size() != static_cast<size_t>(nframes_) )
  {
    return;
  }

  STHOGVECTYPE ahog_vec;
  ahog_vec.clear();

  std::vector<float> ahog;
  for(int i=0;i<nframes_; i++)
  {
    ahog.clear();
    std::vector<cv::Point> location;
    location.push_back( pt_in_frames[i] );
    HOGDescriptor::compute( imgs[i], ahog, cv::Size(), cv::Size(), location );
    ahog_vec.push_back(ahog);
  }

  sthog_map_[pt_in_frames[0]] = ahog_vec;

}

/** Compuate STHOG descriptor by introducing additional images (imgs).
 *  It is assumed the rest are in the buffer, especially useful
 *  for the progressive mode */
void sthog_descriptor::compute( std::vector<cv::Mat>& imgs, std::vector<float>& descriptors,
                                cv::Size winStride, cv::Size padding,
                                const std::vector<cv::Point>& locations)
{
  std::vector<float> des(descriptors);
  std::vector<cv::Mat>::iterator it_img;

  pop_front_sthog_map( imgs.size() );

  for( it_img=imgs.begin(); it_img!=imgs.end(); it_img++ )
  {
    des.clear();
    sthog_descriptor::compute( *it_img, des, winStride, padding, locations );
  }

}

/** Detect object/event based on the STHOG descriptors at a fixed scale level */
void sthog_descriptor::detect(std::vector<cv::Mat>& imgs, std::vector<cv::Point>& hits,
                              double hitThreshold, cv::Size winStride, cv::Size padding,
                              const std::vector<cv::Point>& locations)
{
  std::vector<double> hitThresholds;

  while( hitThresholds.size() < std::max( multi_svm_ocv_.size(), multi_svm_linear_.size()) )
  {
    hitThresholds.push_back( hitThreshold );
  }

  this->multi_model_detection(imgs, hits, hitThresholds, winStride, padding, locations);
}

/** Detect object/event based on the STHOG descriptors at a fixed scale level using multiple SVM models */
void sthog_descriptor::multi_model_detection(std::vector<cv::Mat>& imgs, std::vector<cv::Point>& hits,
                                             std::vector<double> hitThresholds, cv::Size winStride,
                                             cv::Size padding, const std::vector<cv::Point>& locations)
{
  sthog_map_.clear();

  if( multi_svm_linear_.empty() && multi_svm_ocv_.empty())
  {
    LOG_ERROR( "No SVM model found" );
    return;
  }

  // compute sthog feature
  std::vector<float> descriptors;
  this->compute( imgs, descriptors, winStride, padding, locations);
  LOG_ASSERT( check_detector_size(), "detector size" );

  size_t dsize = get_sthog_descriptor_size();

  std::vector<float>::iterator it_svm;
  std::vector<float>::iterator it_des;
  STHOGVECTYPE::iterator it_vec;
  std::map<cv::Point, STHOGVECTYPE, ltPointCompare>::iterator it_map;

  for( it_map=sthog_map_.begin(); it_map!=sthog_map_.end(); it_map++ )
  {
    STHOGVECTYPE sthog_vec = it_map->second;
    double s;

    if( ! multi_svm_linear_.empty() )
    {
      // use linear SVM (dot product)
      for(unsigned m=0; m<multi_svm_linear_.size(); m++)
      {
        std::vector<float>& svm=multi_svm_linear_[m];
        s = svm.size() > dsize ? svm[dsize] : 0;
        it_svm = svm.begin();

        for( it_vec=sthog_vec.begin(); it_vec!=sthog_vec.end(); it_vec++)
        {
          for( it_des=it_vec->begin(); it_des!=it_vec->end(); it_des++, it_svm++ )
          {
            s += (*it_svm) * (*it_des);
          }
        }

        if( s >= hitThresholds[m] )
        {
          hits.push_back( it_map->first );
          hit_scores_.push_back( s );
          hit_models_.push_back( m );

          if( sthog_feature_ )
          {
            // write out feature that leads to the detection
            int index=1;
            LOG_DEBUG( "#%#" << s );
            for( it_vec=sthog_vec.begin(); it_vec!=sthog_vec.end(); it_vec++)
            {
              for( it_des=it_vec->begin(); it_des!=it_vec->end(); it_des++, index++ )
              {
                LOG_DEBUG( index << ":" << (*it_des) << " " );
              }
            }
            LOG_DEBUG( "\n" );
          }
        }
      }
    }
    else
    {
      // use OpenCV svm model svm_ocv_
      cv::Mat feature;
      feature = cv::Mat::zeros(1, dsize, CV_32FC1);

      std::vector<float> vec;
      float *value=feature.ptr<float>(0);
      int i=0;
      for( it_vec=sthog_vec.begin(); it_vec!=sthog_vec.end(); it_vec++)
      {
        for( it_des=it_vec->begin(); it_des!=it_vec->end(); it_des++, it_svm++ )
        {
          value[i++] = (*it_des);
        }
      }

      for(unsigned m=0; m<multi_svm_linear_.size(); m++)
      {
        s = multi_svm_ocv_[m]->predict( feature, vec, true);

        if( s >= hitThresholds[m] )
        {
          hits.push_back( it_map->first );
          hit_scores_.push_back( s );
          hit_models_.push_back( m );

          if( sthog_feature_ )
          {
            // write out feature that leads to the detection
            int index=1;
            LOG_DEBUG( "#%#" << s << " ");
            for( it_vec=sthog_vec.begin(); it_vec!=sthog_vec.end(); it_vec++)
            {
              for( it_des=it_vec->begin(); it_des!=it_vec->end(); it_des++, index++ )
              {
                LOG_DEBUG( index << ":" << (*it_des) << " " );
              }
            }
            LOG_DEBUG( "\n" );
          }
        }
      }
    }
  }
}

struct sthog_invoker
{
  sthog_invoker( sthog_descriptor* _sthog, const std::vector<cv::Mat>& _imgs,
                 std::vector<double> _hitThresholds, cv::Size _winStride, cv::Size _padding,
                 const double* _levelScale, cv::ConcurrentRectVector* _vec )
  {
    sthog = _sthog;
    imgs = _imgs;
    hitThresholds = _hitThresholds;
    winStride = _winStride;
    padding = _padding;
    levelScale = _levelScale;
    vec = _vec;
  }

  void operator()( const cv::BlockedRange& range ) const
  {
    int i, i1 = range.begin(), i2 = range.end();
    cv::Mat img = imgs.front();
    double minScale = i1 > 0 ? levelScale[i1] : i2 > 1 ? levelScale[i1+1] : std::max(img.cols, img.rows);
    cv::Size maxSz(cvCeil(img.cols/minScale), cvCeil(img.rows/minScale));
    cv::Mat smallerImgBuf(maxSz, img.type());
    std::vector<cv::Point> locations;

    for( i = i1; i < i2; i++ )
    {
      double scale = levelScale[i];
      cv::Size sz(cvRound(img.cols/scale), cvRound(img.rows/scale));

      std::vector<cv::Mat> smallerImgs;
      for( unsigned c=0; c<imgs.size(); c++ )
      {
        cv::Mat sImg(sz, img.type(), smallerImgBuf.data);
        if( sz == img.size() )
        {
          sImg = cv::Mat(sz, img.type(), img.data, img.step);
        }
        else
        {
          resize(img, sImg, sz);
        }
        smallerImgs.push_back( sImg );
      }

      sthog->sthog_descriptor::multi_model_detection(smallerImgs, locations, hitThresholds, winStride, padding);

      cv::Size scaledWinSize = cv::Size(cvRound(sthog->winSize.width*scale), cvRound(sthog->winSize.height*scale));

      for( size_t j = 0; j < locations.size(); j++ )
      {
        vec->push_back(cv::Rect(cvRound(locations[j].x*scale),
                            cvRound(locations[j].y*scale),
                            scaledWinSize.width, scaledWinSize.height));
      }
    }
  }

  sthog_descriptor* sthog;
  std::vector<cv::Mat> imgs;
  std::vector<double> hitThresholds;
  cv::Size winStride;
  cv::Size padding;
  const double* levelScale;
  cv::ConcurrentRectVector* vec;
};

/** Detect object/event based on the STHOG descriptors at multiple scale levels */
void
sthog_descriptor::detectMultiScale(
  std::vector<cv::Mat>& imgs, std::vector<cv::Rect>& foundLocations,
  double hitThreshold, cv::Size winStride, cv::Size padding,
  double scale0, int maxLevels, int groupThreshold)
{
  std::vector<double> hitThresholds;
  while( hitThresholds.size() < std::max( multi_svm_ocv_.size(), multi_svm_linear_.size()) )
  {
    hitThresholds.push_back( hitThreshold );
  }

  this->multi_model_scale_detection(imgs, foundLocations, hitThresholds, winStride, padding, scale0, maxLevels, groupThreshold);
}

/** Detect object/event based on the STHOG descriptors at multiple scale levels using multiple SVM models */
void
sthog_descriptor::multi_model_scale_detection(
  std::vector<cv::Mat>& imgs, std::vector<cv::Rect>& foundLocations,
  std::vector<double> hitThresholds, cv::Size winStride, cv::Size padding,
  double scale0, int maxLevels, int groupThreshold)
{
  // clear the scores for every run
  initialize_detector();

  double scale = 1.;
  int levels = 0;
  cv::Mat img = imgs.front();

  std::vector<double> levelScale;
  for( levels = 0; levels < maxLevels; levels++ )
  {
    levelScale.push_back(scale);
    if( cvRound(img.cols/scale) < winSize.width ||
        cvRound(img.rows/scale) < winSize.height ||
        scale0 <= 1 )
    {
      break;
    }
    scale *= scale0;
  }
  levels = std::max(levels, 1);
  levelScale.resize(levels);

  cv::ConcurrentRectVector allCandidates;

  parallel_for( cv::BlockedRange(0, static_cast<int>(levelScale.size())),
                sthog_invoker(this, imgs, hitThresholds, winStride, padding, &levelScale[0], &allCandidates)
    );

  foundLocations.resize(allCandidates.size());
  std::copy(allCandidates.begin(), allCandidates.end(), foundLocations.begin());

  if( hitThresholds[0] > -10 )
  {
    kwocv::group_rectangle_scores(foundLocations, hit_scores_, hit_models_, groupThreshold, 0.2, 0);
  }
  else
  {
    // heatmap entries
    for(unsigned j=0; j<hit_scores_.size(); j++)
    {
      cv::Rect rt=foundLocations[j];
      LOG_INFO( j << " " << hit_scores_[j] << " " << rt.x << " " << rt.y << " " << rt.width << " " << rt.height );
    }
  }
}

/** Get the computed STHOG descriptor at location pt */
void sthog_descriptor::get_sthog_descriptor(std::vector<float>& descriptor, cv::Point pt)
{
  descriptor.clear();

  if( sthog_map_.find(pt) == sthog_map_.end() )
  {
    LOG_ERROR( "descriptor is not available at [" << pt.x << "," << pt.y );
    return;
  }

  STHOGVECTYPE::iterator it_vec;
  HOGTYPE::iterator       it_des;
  STHOGVECTYPE sthog_vec = sthog_map_[pt];

  for( it_vec=sthog_vec.begin(); it_vec!=sthog_vec.end(); it_vec++ )
  {
    for( it_des=it_vec->begin(); it_des!=it_vec->end(); it_des++ )
    {
      descriptor.push_back( *it_des );
    }
  }
}

/** Pop the front element, i.e. pop out the HOG descriptor of the leading image */
void sthog_descriptor::pop_front_sthog_map(int n)
{
  std::map<cv::Point, STHOGVECTYPE, ltPointCompare>::iterator it_map;

  // pop_front n elements from each sthog feature
  for( it_map=sthog_map_.begin(); it_map!=sthog_map_.end(); it_map++ )
  {
    STHOGVECTYPE sthog_vec = it_map->second;

    for( int i=0; i<n; i++ )
    {
      if( sthog_vec.size() == 0 )
      {
        break;
      }
      sthog_vec.erase( sthog_vec.begin() );
    }
  }

  for( it_map=sthog_map_.begin(); it_map!=sthog_map_.end(); )
  {
    if( it_map->second.size() == 0 )
    {
      std::map<cv::Point, STHOGVECTYPE, ltPointCompare>::iterator next = it_map;
      next++;
      sthog_map_.erase(it_map);
      it_map = next;
    }
    else
    {
      it_map++;
    }
  }

}

/** Clear the cached STHOG descriptor */
void sthog_descriptor::clear_sthog_map()
{
  std::map<cv::Point, STHOGVECTYPE, ltPointCompare>::iterator it_map;

  for( it_map=sthog_map_.begin(); it_map!=sthog_map_.end(); it_map-- )
  {
    it_map->second.clear();
  }

  sthog_map_.clear();
}

/** Import SVM model */
void sthog_descriptor::set_svm_detector(const std::vector<float>& _svm_detector)
{
  svmDetector.clear();
  for(unsigned i=0; i<_svm_detector.size(); i++)
  {
    svmDetector.push_back( _svm_detector[i] );
  }
  LOG_ASSERT( check_detector_size(), "detector size" );
}

/** Load OpenCV SVM model from a model file */
void sthog_descriptor::load_svm_ocv_model(const std::string& fname)
{
  if( !fname.empty() )
  {
    svm_ocv_->load(fname.c_str());

    // turn on OpenCV svm and turn off svmDetector
    svmDetector.clear();
  }
}

/** Load linear SVM model from a model file */
void sthog_descriptor::load_svm_linear_model(const std::string& fname, bool verbose)
{
  if( !fname.empty() )
  {
    svm_ocv_->read_linear_model(fname.c_str(), get_sthog_descriptor_size()+1);

    svmDetector = svm_ocv_->get_linear_svm_coefficients();

    if(verbose)
    {
      for(unsigned i=0; i<svmDetector.size(); i++)
      {
        LOG_DEBUG( i << " " << svmDetector[i] );
      }
    }
  }
}

/** Set multiple OCV svm models */
void sthog_descriptor::svm_ocv_push_back()
{
  multi_svm_ocv_.push_back(svm_ocv_);
  svm_ocv_.reset(new svm_t());
}

/** Set multiple linear svm models */
void sthog_descriptor::svm_linear_push_back()
{
  multi_svm_linear_.push_back(svmDetector);
}

} // namespace vidtk
