/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_greyscale_morphology_process_h_
#define vidtk_greyscale_morphology_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <tracking/image_object.h>
#include <vil/vil_image_view.h>
#include <vil/algo/vil_structuring_element.h>

namespace vidtk
{


class greyscale_morphology_process
  : public process
{
public:
  typedef greyscale_morphology_process self_type;

  greyscale_morphology_process( vcl_string const& name );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// Set the detected foreground image.
  void set_source_image( vil_image_view<vxl_byte> const& img );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  /// The output image.
  vil_image_view<vxl_byte> const& image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte> const&, image );

private:
  typedef image_object::float_type float_type;

  config_block config_;

  vil_image_view<vxl_byte> const* src_img_;

  // Parameters
  double opening_radius_;
  double closing_radius_;

  vil_structuring_element opening_el_;
  vil_structuring_element closing_el_;

  vil_image_view<vxl_byte> out_img_;
  vil_image_view<vxl_byte> buffer_;
};


} // end namespace vidtk


#endif // vidtk_greyscale_morphology_process_h_
