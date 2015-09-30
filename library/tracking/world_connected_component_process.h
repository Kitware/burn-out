/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_world_connected_component_process_h_
#define vidtk_world_connected_component_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <tracking/image_object.h>
#include <vil/vil_image_view.h>
#include <vil/algo/vil_blob.h>
#include <utilities/homography.h>

namespace vidtk
{


class world_connected_component_process
  : public process
{
public:
  typedef world_connected_component_process self_type;

  world_connected_component_process( vcl_string const& name );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// Set the detected foreground image.
  void set_fg_image( vil_image_view<bool> const& img );

  VIDTK_INPUT_PORT( set_fg_image, vil_image_view<bool> const& );

  /// The connected components found in the image.
  vcl_vector< image_object_sptr > const& objects() const;

  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

  void set_world_units_per_pixel( double val );

  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

  void set_is_fg_good( bool val );

  VIDTK_INPUT_PORT( set_is_fg_good, bool );

  void set_src_2_wld_homography( image_to_plane_homography const& H );

  VIDTK_INPUT_PORT( set_src_2_wld_homography, image_to_plane_homography );

private:
  typedef image_object::float_type float_type;
  typedef image_object::float2d_type float2d_type;
  typedef image_object::float3d_type float3d_type;

  config_block config_;

  vil_image_view<bool> const* fg_img_;
  vcl_vector< image_object_sptr > objs_;
  double world_units_per_pixel_;
  bool is_fg_good_;
  image_to_plane_homography H_src2wld_;

  // Parameters
  float_type min_size_, max_size_;
  enum location_type { centroid, bottom };
  location_type loc_type_; // centroid or bottom
  vil_blob_connectivity blob_connectivity_;
};


} // end namespace vidtk


#endif // vidtk_world_connected_component_process_h_
