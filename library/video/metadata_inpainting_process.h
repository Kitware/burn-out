/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_metadata_inpainting_process_h_
#define vidtk_metadata_inpainting_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <vil/vil_image_view.h>


namespace vidtk
{


class metadata_inpainting_process
  : public process
{
public:
  typedef metadata_inpainting_process self_type;

  metadata_inpainting_process( vcl_string const& name );

  ~metadata_inpainting_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<vxl_byte> const& img );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  /// \brief The input mask.
  void set_mask( vil_image_view<bool> const& img );

  VIDTK_INPUT_PORT( set_mask, vil_image_view<bool> const& );

  /// \brief The inpainted output image.
  vil_image_view<vxl_byte> const& inpainted_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte> const&, inpainted_image );

private:

  // Parameters
  config_block config_;

  bool disabled_;
  bool invalid_image_;

  int radius_;
  int method_;

  // Input
  vil_image_view<vxl_byte> input_image_;
  vil_image_view<bool> mask_;

  // Output
  vil_image_view<vxl_byte> inpainted_image_;
};


} // end namespace vidtk


#endif // vidtk_metadata_inpainting_process_h_
