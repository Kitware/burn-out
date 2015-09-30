/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_world_morphology_process_h_
#define vidtk_world_morphology_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <tracking/image_object.h>
#include <vil/vil_image_view.h>
#include <vil/algo/vil_structuring_element.h>

namespace vidtk
{


class world_morphology_process
  : public process
{
public:
  typedef world_morphology_process self_type;

  world_morphology_process( vcl_string const& name );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// Set the detected foreground image.
  void set_source_image( vil_image_view<bool> const& img );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<bool> const& );

  /// The output image.
  vil_image_view<bool> const& image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<bool> const&, image );

  void set_world_units_per_pixel( double units_per_pix );

  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

  void set_is_fg_good( bool val );

  VIDTK_INPUT_PORT( set_is_fg_good, bool );

private:
  typedef image_object::float_type float_type;

  config_block config_;

  vil_image_view<bool> const* src_img_;
  bool is_fg_good_;

  // Parameters
  double opening_radius_;
  double closing_radius_;

  // These parameters are added specifically for the medium zoom 
  // level where object are very tiny.
  double min_image_opening_radius_;
  double min_image_closing_radius_;
  double max_image_opening_radius_;
  double max_image_closing_radius_;

  double last_image_closing_radius_;
  double last_image_opening_radius_;

  double last_step_world_units_per_pixel_;
  double world_units_per_pixel_;

  vil_structuring_element opening_el_;
  vil_structuring_element closing_el_;

  vil_image_view<bool> out_img_;
  vil_image_view<bool> buffer_;
};


} // end namespace vidtk


#endif // vidtk_world_morphology_process_h_
