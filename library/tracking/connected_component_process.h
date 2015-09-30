/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_connected_component_process_h_
#define vidtk_connected_component_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <tracking/image_object.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

class connected_component_process
  : public process
{
public:
  typedef connected_component_process self_type;

  connected_component_process( vcl_string const& name );

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

private:
  typedef image_object::float_type float_type;
  typedef image_object::float2d_type float2d_type;
  typedef image_object::float3d_type float3d_type;

  config_block config_;

  vil_image_view<bool> const* fg_img_;

  // Parameters
  float_type min_size_, max_size_;
  enum location_type { centroid, bottom };
  location_type loc_type_; // centroid or bottom
  vcl_vector< image_object_sptr > objs_;
};


} // end namespace vidtk


#endif // vidtk_connected_component_process_h_
