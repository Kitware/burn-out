/*ckwg +5
 * Copyright 2011-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kwklt_tracking_process_impl_klt_h_
#define vidtk_kwklt_tracking_process_impl_klt_h_

#include "klt_tracking_process_impl.h"

#include <klt/klt.h>

#include <vil/vil_pyramid_image_view.h>

#include <utilities/config_block.h>
#include <utilities/timestamp.h>

#include <vector>

namespace vidtk
{

class klt_tracking_process_impl_klt
  : public klt_tracking_process_impl
{
public:
  klt_tracking_process_impl_klt();

  virtual ~klt_tracking_process_impl_klt();

  virtual bool initialize();

  virtual bool reinitialize();

  virtual bool is_ready();

  virtual int track_features();
  virtual void replace_features();
  virtual void select_features();

  virtual void post_step();

  virtual void set_image_pyramid(vil_pyramid_image_view<float> const&);
  virtual void set_image_pyramid_gradx(vil_pyramid_image_view<float> const&);
  virtual void set_image_pyramid_grady(vil_pyramid_image_view<float> const&);
  virtual void set_timestamp(timestamp const&);
  virtual void set_homog_predict(vgl_h_matrix_2d<double> const &);

protected:
  void find_new_features();
  int find_tracked_features();

  timestamp const* ts_;
  float homog_predict_[9];

  KLT_TrackingPyramidRec klt_cur_;
  KLT_TrackingPyramidRec klt_prev_;
  KLT_TrackingContext klt_tracking_context_;
  KLT_FeatureList klt_feature_list_;

  std::vector<klt_track_ptr> current_;
};


} // end namespace vidtk


#endif // vidtk_kwklt_tracking_process_impl_klt_h_
