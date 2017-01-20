/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_to_scale_image_process_h_
#define vidtk_homography_to_scale_image_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <utilities/homography.h>
#include <tracking_data/image_object.h>
#include <vil/vil_image_view.h>
#include <utility>

namespace vidtk
{

///
/// A class for converting a planar homography into a depth map which shows
/// the estimated scale of each pixel in an image.
///
/// There are 2 principle operating modes of this filter, which reflect how
/// depth/scale information should be outputted:
///
///   - DEPTH_IMAGE, The output is an image containing a scale for each fg pixel
///   - HASHED_DEPTH_IMAGE, The output contains some relative size of the GSD
///        mapped unto an integral quantity produced by scaling the GSD image
///        by some scale factor.
///
/// There is currently only 1 algorithm provided which converts the homography
/// to the gsd image.
///
///   - GRID, samples a user specified number of rectangles in the image,
///        where each rectangle sampling zone is given the same GSD
///
template< typename HashType = unsigned >
class homography_to_scale_image_process
  : public process
{
public:

  typedef homography_to_scale_image_process self_type;
  typedef std::pair< unsigned, unsigned > resolution_t;

  homography_to_scale_image_process( std::string const& name );

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// Set a constant planar homography, if used.
  void set_image_to_world_homography( image_to_plane_homography const& H );
  VIDTK_INPUT_PORT( set_image_to_world_homography, image_to_plane_homography const& );

  /// The resolution of the output image
  void set_resolution( resolution_t const& res );
  VIDTK_INPUT_PORT( set_resolution, resolution_t const& );

  /// Should be marked as false if we want to not process a frame.
  void set_is_valid_flag( bool val );
  VIDTK_INPUT_PORT( set_is_valid_flag, bool );

  /// GSD Output Image (Not reallocated - For Sync Mode)
  vil_image_view<double> gsd_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<double>, gsd_image );

  /// GSD Output Image (Reallocated - For Async Mode)
  vil_image_view<double> copied_gsd_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<double>, copied_gsd_image );

  /// Hashed GSD Output Image (Not reallocated - For Sync Mode)
  vil_image_view<HashType> hashed_gsd_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<HashType>, hashed_gsd_image );

  /// Hashed GSD Output Image (Reallocated - For Async Mode)
  vil_image_view<HashType> copied_hashed_gsd_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<HashType>, copied_hashed_gsd_image );

private:

  // Operating modes of this filter
  enum{ GSD_IMAGE, HASHED_GSD_IMAGE } operating_mode_;
  enum{ GRID } algorithm_;

  // Internal parameter block
  config_block config_;
  unsigned grid_sampling_height_;
  unsigned grid_sampling_width_;
  bool estimate_horizon_location_;

  // Scale factor of hashed image, if used
  double hashed_gsd_scale_factor_;
  double hashed_max_gsd_;

  // Buffer last matrix so we know when to not recreate the depth-map
  image_to_plane_homography last_homog_;
  bool is_first_frame_;

  // Possible inputs
  image_to_plane_homography H_src2wld_;
  bool process_frame_;
  unsigned ni_;
  unsigned nj_;

  // Algorithm outputs
  vil_image_view<double> gsd_img_;
  vil_image_view<HashType> hashed_gsd_img_;
  mutable vil_image_view<double> copied_gsd_img_;
  mutable vil_image_view<HashType> copied_hashed_gsd_img_;
};


} // end namespace vidtk


#endif // vidtk_homography_to_scale_image_process_h_
