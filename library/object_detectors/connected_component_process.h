/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_connected_component_process_h_
#define vidtk_connected_component_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/config_block.h>
#include <tracking_data/image_object.h>

#include <vil/vil_image_view.h>
#include <vil/algo/vil_blob.h>

#include <vector>

namespace vidtk
{


class connected_component_process
  : public process
{
public:
  typedef connected_component_process self_type;

  connected_component_process( std::string const& name );
  virtual ~connected_component_process() {}

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// Set the detected foreground mask.
  void set_fg_image( vil_image_view< bool > const& img );
  VIDTK_INPUT_PORT( set_fg_image, vil_image_view< bool > const& );

  /// Set the detected foreground heatmap.
  void set_heatmap_image( vil_image_view< float > const& img );
  VIDTK_INPUT_PORT( set_heatmap_image, vil_image_view< float > const& );

  /// Set input GSD.
  void set_world_units_per_pixel( double val );
  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

  /// Is the FG image stable?
  void set_is_fg_good( bool val );
  VIDTK_INPUT_PORT( set_is_fg_good, bool );

  /// The connected components found in the image.
  std::vector< image_object_sptr > objects() const;
  VIDTK_OUTPUT_PORT( std::vector< image_object_sptr >, objects );

private:
  typedef image_object::float_type float_type;

  config_block config_;

  // Inputs
  vil_image_view< bool > fg_img_;
  vil_image_view< float > heatmap_img_;
  double world_units_per_pixel_;
  bool is_fg_good_;

  // Outputs
  std::vector< image_object_sptr > objs_;

  // Parameters
  bool disabled_;
  float_type min_size_, max_size_;
  enum { centroid, bottom } loc_type_;
  enum { none, average, min } confidence_method_;
  float min_confidence_, max_confidence_;
  vil_blob_connectivity blob_connectivity_;
};


} // end namespace vidtk


#endif // vidtk_connected_component_process_h_
