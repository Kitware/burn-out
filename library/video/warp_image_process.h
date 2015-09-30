/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_warp_image_process_h_
#define vidtk_warp_image_process_h_

#include <vgl/algo/vgl_h_matrix_2d.h>

#include <vil/vil_image_view.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{


template <class PixType>
class warp_image_process
  : public process
{
public:
  typedef warp_image_process self_type;

  warp_image_process( vcl_string const& name );

  ~warp_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();
  
  virtual bool reset();

  virtual bool step();

  void set_destination_to_source_homography( vgl_h_matrix_2d<double> const& );

  VIDTK_INPUT_PORT( set_destination_to_source_homography, vgl_h_matrix_2d<double> const& );

  void set_source_image( vil_image_view<PixType> const& );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  vil_image_view<PixType> const& warped_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, warped_image );

private:
  config_block config_;

  /// See constructor for description of the input parameter.
  bool only_first_;
  bool disabled_;
  bool reset_internal_buffer_;

  unsigned frame_counter_;

  vgl_h_matrix_2d<double> homog_;
  vil_image_view<PixType> img_;
  vil_image_view<PixType> out_img_;
};


} // end namespace vidtk


#endif // vidtk_warp_image_process_h_
