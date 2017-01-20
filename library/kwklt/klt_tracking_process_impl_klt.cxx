/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_tracking_process_impl_klt.h"

#include "klt_util.h"
#include <kwklt/klt_mutex.h>

#include <klt/pyramid.h>

#include <utilities/timestamp.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

namespace vidtk
{

klt_tracking_process_impl_klt::klt_tracking_process_impl_klt()
  : klt_tracking_process_impl()
{
  klt_feature_list_ = NULL;

  klt_tracking_context_ = NULL;

  klt_prev_.pyramid = NULL;
  klt_prev_.pyramid_gradx = NULL;
  klt_prev_.pyramid_grady = NULL;
}

klt_tracking_process_impl_klt::~klt_tracking_process_impl_klt()
{
  if (klt_tracking_context_)
  {
    post_step();
    KLTFreeTrackingContext(klt_tracking_context_);
  }
  KLTFreeFeatureList(klt_feature_list_);
}


bool klt_tracking_process_impl_klt::initialize()
{ // critical region
  boost::lock_guard<boost::mutex> lock(vidtk::klt_mutex::instance()->get_lock());

  klt_tracking_process_impl::initialize();

  if (klt_tracking_context_)
  {
    KLTFreeTrackingContext(klt_tracking_context_);
  }

  klt_tracking_context_ = KLTCreateTrackingContext();
  klt_tracking_context_->sequentialMode = TRUE;
  klt_tracking_context_->window_width = window_width_;
  klt_tracking_context_->window_height = window_height_;
  klt_tracking_context_->mindist = min_distance_;
  klt_tracking_context_->nSkippedPixels = num_skipped_pixels_;

  // The input pyramids are trusted, so search_range can be bogus here.
  KLTChangeTCPyramid(klt_tracking_context_, 0);
  KLTUpdateTCBorder(klt_tracking_context_);

  return true;
} // end critical region


bool klt_tracking_process_impl_klt::reinitialize()
{
  ts_ = NULL;

  current_.clear();
  current_.resize(feature_count_);

  _KLTFreePyramid( klt_prev_.pyramid );
  _KLTFreePyramid( klt_prev_.pyramid_gradx );
  _KLTFreePyramid( klt_prev_.pyramid_grady );

  klt_prev_.pyramid = NULL;
  klt_prev_.pyramid_gradx = NULL;
  klt_prev_.pyramid_grady = NULL;
  homog_predict_[0] = 1.0f;
  homog_predict_[1] = 0.0f;
  homog_predict_[2] = 0.0f;
  homog_predict_[3] = 0.0f;
  homog_predict_[4] = 1.0f;
  homog_predict_[5] = 0.0f;
  homog_predict_[6] = 0.0f;
  homog_predict_[7] = 0.0f;
  homog_predict_[8] = 1.0f;

  KLTFreeFeatureList(klt_feature_list_);
  klt_feature_list_ = KLTCreateFeatureList(feature_count_);

  return true;
}

bool klt_tracking_process_impl_klt::is_ready()
{
  if (!klt_cur_.pyramid || !klt_cur_.pyramid_gradx || !klt_cur_.pyramid_grady || !ts_)
  {
    return false;
  }

  return klt_tracking_process_impl::is_ready();
}

int klt_tracking_process_impl_klt::track_features()
{
  KLTTrackFeaturesPyramid(klt_tracking_context_, &klt_prev_, &klt_cur_, klt_cur_.pyramid->ncols[0], klt_cur_.pyramid->nrows[0], homog_predict_, klt_feature_list_);

  return find_tracked_features();
}

void klt_tracking_process_impl_klt::replace_features()
{
  _KLT_FloatImage image = klt_cur_.pyramid->img[0];
  _KLT_FloatImage image_gradx = klt_cur_.pyramid_gradx->img[0];
  _KLT_FloatImage image_grady = klt_cur_.pyramid_grady->img[0];

  KLTReplaceLostFeaturesRaw(klt_tracking_context_, image, image_gradx, image_grady, klt_feature_list_);

  find_new_features();
}

void klt_tracking_process_impl_klt::select_features()
{
  _KLT_FloatImage image = klt_cur_.pyramid->img[0];
  _KLT_FloatImage image_gradx = klt_cur_.pyramid_gradx->img[0];
  _KLT_FloatImage image_grady = klt_cur_.pyramid_grady->img[0];

  KLTSelectGoodFeaturesRaw(klt_tracking_context_, image, image_gradx, image_grady, klt_feature_list_);

  find_new_features();
}

void klt_tracking_process_impl_klt::post_step()
{
  _KLTFreePyramid(klt_prev_.pyramid);
  _KLTFreePyramid(klt_prev_.pyramid_gradx);
  _KLTFreePyramid(klt_prev_.pyramid_grady);

  klt_tracking_context_->pyramid_last = NULL;
  klt_tracking_context_->pyramid_last_gradx = NULL;
  klt_tracking_context_->pyramid_last_grady = NULL;

  klt_prev_.pyramid = klt_cur_.pyramid;
  klt_prev_.pyramid_gradx = klt_cur_.pyramid_gradx;
  klt_prev_.pyramid_grady = klt_cur_.pyramid_grady;

  klt_cur_.pyramid = NULL;
  klt_cur_.pyramid_gradx = NULL;
  klt_cur_.pyramid_grady = NULL;
  ts_ = NULL;
  homog_predict_[0] = 1.0f;
  homog_predict_[1] = 0.0f;
  homog_predict_[2] = 0.0f;
  homog_predict_[3] = 0.0f;
  homog_predict_[4] = 1.0f;
  homog_predict_[5] = 0.0f;
  homog_predict_[6] = 0.0f;
  homog_predict_[7] = 0.0f;
  homog_predict_[8] = 1.0f;

  klt_tracking_process_impl::post_step();
}

/// The image tracking of the input image.
void klt_tracking_process_impl_klt::set_image_pyramid(vil_pyramid_image_view<float> const& img)
{
  klt_cur_.pyramid = klt_pyramid_convert(const_cast<vil_pyramid_image_view<float>&>(img));
  klt_tracking_context_->nPyramidLevels = klt_cur_.pyramid->nLevels;
  klt_tracking_context_->subsampling = klt_cur_.pyramid->subsampling;
}

/// The image tracking of the x gradient.
void klt_tracking_process_impl_klt::set_image_pyramid_gradx(vil_pyramid_image_view<float> const& img)
{
  klt_cur_.pyramid_gradx = klt_pyramid_convert(const_cast<vil_pyramid_image_view<float>&>(img));
}

/// The image tracking of the y gradient.
void klt_tracking_process_impl_klt::set_image_pyramid_grady(vil_pyramid_image_view<float> const& img)
{
  klt_cur_.pyramid_grady = klt_pyramid_convert(const_cast<vil_pyramid_image_view<float>&>(img));
}

/// The timestamp for the current frame.
void klt_tracking_process_impl_klt::set_timestamp(vidtk::timestamp const& ts)
{
  ts_ = &ts;
}

void klt_tracking_process_impl_klt::set_homog_predict(vgl_h_matrix_2d<double> const &h)
{
  homog_predict_[0] = static_cast<float>(h.get(0,0));
  homog_predict_[1] = static_cast<float>(h.get(0,1));
  homog_predict_[2] = static_cast<float>(h.get(0,2));
  homog_predict_[3] = static_cast<float>(h.get(1,0));
  homog_predict_[4] = static_cast<float>(h.get(1,1));
  homog_predict_[5] = static_cast<float>(h.get(1,2));
  homog_predict_[6] = static_cast<float>(h.get(2,0));
  homog_predict_[7] = static_cast<float>(h.get(2,1));
  homog_predict_[8] = static_cast<float>(h.get(2,2));
}

void klt_tracking_process_impl_klt::find_new_features()
{
  for (int i = 0; i < klt_feature_list_->nFeatures; ++i)
  {
    if (klt_feature_list_->feature[i]->val > KLT_TRACKED)
    {
      klt_track::point_ point;
      point.x = klt_feature_list_->feature[i]->x;
      point.y = klt_feature_list_->feature[i]->y;
      point.frame = ts_->frame_number();
      klt_track_ptr new_track = klt_track::extend_track(point);

      current_[i] = new_track;
      active_.push_back(new_track);
      created_.push_back(new_track);
    }
  }
}

int klt_tracking_process_impl_klt::find_tracked_features()
{
  int count = 0;

  for (int i = 0; i < klt_feature_list_->nFeatures; ++i)
  {
    if (klt_feature_list_->feature[i]->val == KLT_TRACKED)
    {
      klt_track::point_ point;
      point.x = klt_feature_list_->feature[i]->x;
      point.y = klt_feature_list_->feature[i]->y;
      point.frame = ts_->frame_number();
      klt_track_ptr new_track = klt_track::extend_track(point, current_[i]);

      current_[i] = new_track;
      active_.push_back(new_track);

      ++count;
    }
    else if (klt_feature_list_->feature[i]->val < KLT_NOT_FOUND)
    {
      if (current_[i])
      {
        terminated_.push_back(current_[i]);
      }
      current_[i] = klt_track_ptr();
    }
  }

  return count;
}

} // end namespace vidtk
