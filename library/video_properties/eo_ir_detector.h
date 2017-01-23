/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_properties_eo_ir_detector_h_
#define vidtk_video_properties_eo_ir_detector_h_

#include <vil/vil_image_view.h>

#include <utilities/external_settings.h>

namespace vidtk
{

#define settings_macro( add_param ) \
  add_param( \
    eo_multiplier, \
    float, \
    4.0f, \
    "If there are more near gray pixels in the image compared with " \
    "the number of colored pixels multiplied with this scale factor " \
    "consider the image IR, otherwise EO."); \
  add_param( \
    scale_factor, \
    float, \
    0.8f, \
    "Only check pixels in the center of the image by scaling each " \
    "dimension by this percentage." ); \
  add_param( \
    eo_ir_sample_size, \
    float, \
    0.1f, \
    "Percentage of pixels to sample in the image or image region." ); \
  add_param( \
    rg_diff, \
    vxl_sint_32, \
    8, \
    "Difference threshold for red and green. Used when determining " \
    "eo or ir." ); \
  add_param( \
    rb_diff, \
    vxl_sint_32, \
    8, \
    "Difference threshold for red and blue. Used when determining " \
    "eo or ir." ); \
  add_param( \
    gb_diff, \
    vxl_sint_32, \
    8, \
    "Difference threshold for green and blue. Used when determining " \
    "eo or ir." ); \

init_external_settings( eo_ir_detector_settings, settings_macro );

#undef settings_macro


/// \brief Simple class to detect whether or not an image is EO or IR.
template < class PixType >
class eo_ir_detector
{
public:

  eo_ir_detector();
  eo_ir_detector( eo_ir_detector_settings const& settings );

  /// Configure internal settings of the detector.
  bool configure( eo_ir_detector_settings const& settings );

  /// Is the given image IR?
  bool is_ir( vil_image_view< PixType > const& img ) const;

  /// Is the given image EO?
  bool is_eo( vil_image_view< PixType > const& img ) const;

protected:

  /// Internal copy of settings.
  eo_ir_detector_settings settings_;

};

}

#endif
