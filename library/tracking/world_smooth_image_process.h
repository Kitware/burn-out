/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_world_smooth_image_process_h_
#define vidtk_world_smooth_image_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/config_block.h>
#include <tracking/image_object.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

template < class PixType >
class world_smooth_image_process
  : public process
{
public:
  typedef world_smooth_image_process self_type;

  world_smooth_image_process( vcl_string const& name );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// Set the detected foreground image.
  void set_source_image( vil_image_view<PixType> const& img );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// GSD input.
  void set_world_units_per_pixel( double units_per_pix );

  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

  /// The output image.
  vil_image_view<PixType> const& output_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, output_image );

  /// A clone of the output image.
  vil_image_view<PixType> const& copied_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, copied_image );

private:
  config_block config_;

  vil_image_view<PixType> const* src_img_;
  vil_image_view<PixType> out_img_;
  mutable vil_image_view<PixType> copied_out_img_;

  // Parameters
  double world_std_dev_;

  double max_image_std_dev_;

  int world_half_width_;

  double world_units_per_pixel_;

  unsigned max_image_half_width_;

  bool use_integer_based_smoothing_;

};


} // end namespace vidtk


#endif // vidtk_world_smooth_image_process_h_
