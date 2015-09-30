/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_mask_overlay_process_h_
#define vidtk_mask_overlay_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vnl/vnl_double_3.h>

namespace vidtk
{


/// \brief Draw a mask over an image for debugging/display purposes
class mask_overlay_process
  : public process
{
public:
  typedef mask_overlay_process self_type;

  mask_overlay_process( vcl_string const& name );

  ~mask_overlay_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<vxl_byte> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  /// \brief The input mask.
  void set_mask( vil_image_view<bool> const& mask );
  VIDTK_INPUT_PORT( set_mask, vil_image_view<bool> const& );

  /// \brief The output composite image.
  vil_image_view<vxl_byte> const& image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte> const&, image );


private:

  // Parameters
  config_block config_;

  double transparancy_;
  vnl_double_3 color_;

  vil_image_view<vxl_byte> image_;
  vil_image_view<bool> mask_;
  vil_image_view<vxl_byte> overlay_;

};


} // end namespace vidtk


#endif // vidtk_mask_overlay_process_h_
