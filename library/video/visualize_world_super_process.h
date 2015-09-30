/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_visualize_world_super_process_h_
#define vidtk_visualize_world_super_process_h_

#include <pipeline/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>
#include <utilities/video_metadata.h>

namespace vidtk
{

template< class PixType >
class visualize_world_super_process_impl; 


/// \file A debugging sp for visualizing tracking world image. 

template< class PixType >
class visualize_world_super_process
  : public super_process
{
public:
  typedef visualize_world_super_process self_type;

  visualize_world_super_process( vcl_string const& name );

  ~visualize_world_super_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual process::step_status step2();

  // Input ports
  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_source_timestamp( timestamp const& );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_src_to_wld_homography(image_to_plane_homography const& );
  VIDTK_INPUT_PORT( set_src_to_wld_homography, image_to_plane_homography const& );

  void set_world_units_per_pixel(double );
  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

private:
  visualize_world_super_process_impl<PixType> * impl_; 
};

} // end namespace vidtk

#endif // vidtk_visualize_world_super_process_h_
