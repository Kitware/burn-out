/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gen_world_units_per_pixel_process_h_
#define vidtk_gen_world_units_per_pixel_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/config_block.h>
#include <utilities/homography.h>
#include <vgl/vgl_point_2d.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>
#include <vcl_algorithm.h>

namespace vidtk
{

template <class PixType>
class gen_world_units_per_pixel_process
  : public process
{
public:
  typedef gen_world_units_per_pixel_process self_type;

  gen_world_units_per_pixel_process( vcl_string const& name );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  // Input src2wld homography used to compute GSD
  void set_source_homography( vgl_h_matrix_2d<double> const& H );

  VIDTK_INPUT_PORT( set_source_homography, vgl_h_matrix_2d<double> const& );

  void set_source_vidtk_homography( image_to_plane_homography const & H );
  VIDTK_INPUT_PORT( set_source_vidtk_homography, image_to_plane_homography const& );

  // Input image to extract the exact size.
  void set_image( vil_image_view<PixType> const& );

  VIDTK_INPUT_PORT( set_image, vil_image_view<PixType> const& );

  /// The output GSD port
  double world_units_per_pixel( ) const;

  VIDTK_OUTPUT_PORT( double, world_units_per_pixel );

private:
  config_block config_;

  vcl_pair<unsigned,unsigned> tl_xy;
  vcl_pair<unsigned,unsigned> br_xy;

  /// The source homography
  vgl_h_matrix_2d<double> const * im2wld_H_;

  /// Image only used to extract size
  vil_image_view<PixType> const* img_;
  
  double world_units_per_pixel_;
  bool disabled_;
  bool auto_size_extraction_;
};


} // end namespace vidtk


#endif // vidtk_gen_world_units_per_pixel_h_
