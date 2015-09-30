/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kwklt_tracking_process_impl_h_
#define vidtk_kwklt_tracking_process_impl_h_

#include "klt_track.h"

#include <vil/vil_pyramid_image_view.h>

#include <utilities/config_block.h>
#include <utilities/timestamp.h>

namespace vidtk
{

class klt_tracking_process_impl
{
public:
  klt_tracking_process_impl();

  virtual ~klt_tracking_process_impl();

  static config_block params();
  virtual bool set_params(config_block const&);

  virtual bool initialize();

  virtual bool reinitialize();

  virtual bool is_ready();

  virtual int track_features() = 0;
  virtual void replace_features() = 0;
  virtual void select_features() = 0;

  virtual void post_step();

  virtual void set_image_pyramid(vil_pyramid_image_view<float> const&) = 0;
  virtual void set_image_pyramid_gradx(vil_pyramid_image_view<float> const&) = 0;
  virtual void set_image_pyramid_grady(vil_pyramid_image_view<float> const&) = 0;
  virtual void set_timestamp(timestamp const&) = 0;

  bool disabled_;

  int feature_count_;
  int min_feature_count_;
  int window_width_;
  int window_height_;
  int min_distance_;
  int num_skipped_pixels_;
  int search_range_;

  vcl_vector<klt_track_ptr> active_;
  vcl_vector<klt_track_ptr> terminated_;
  vcl_vector<klt_track_ptr> created_;
};


} // end namespace vidtk


#endif // vidtk_kwklt_tracking_process_impl_h_
