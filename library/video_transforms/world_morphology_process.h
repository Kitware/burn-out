/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_world_morphology_process_h_
#define vidtk_world_morphology_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <vil/vil_image_view.h>
#include <vil/algo/vil_structuring_element.h>

namespace vidtk
{

///
/// A class for performing morphology operations based on information relating
/// to the scale of different components of the "world".
///
/// There are 3 principle operating modes of this filter, which reflect how depth/scale
/// information is provided:
///
///   - CONSTANT_GSD, The input is a singular GSD value for the entire image
///   - GSD_IMAGE, The input is an image containing an approximate scale for each fg pixel
///   - HASHED_GSD_IMAGE, The input image contains some relative size (integer) of the
///        morphology operator to be performed on each pixel. The actual per pixel gsd
///        is defined as the integer divided by the hashed_gsd_scale_factor (a parameter).
///        This operating mode is more efficient when we have multiple world homography
///        processes using the same depth map.
///
/// The process takes 2 templated arguments, if the input pixel type is anything other
/// than a boolean operation than grayscale morphology will be performed. HashType is
/// the type of the GSD hash input image in the event that we wish to use that mode.
///
template < typename PixType = bool, typename HashType = unsigned >
class world_morphology_process
  : public process
{
public:
  typedef world_morphology_process self_type;

  world_morphology_process( std::string const& name );

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();
  virtual bool reset();

  /// Set the detected foreground image. Required.
  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// Set a constant GSD value for the image, if used.
  void set_world_units_per_pixel( double units_per_pix );
  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

  /// Set an input per-pixel approximate size image, if used.
  void set_gsd_image( vil_image_view<double> const& img );
  VIDTK_INPUT_PORT( set_gsd_image, vil_image_view<double> const& );

  /// Set an input per-pixel hashed gsd image, if used.
  void set_hashed_gsd_image( vil_image_view<HashType> const& img );
  VIDTK_INPUT_PORT( set_hashed_gsd_image, vil_image_view<HashType> const& );

  /// Should be market as false if we want to not process certain frames.
  void set_is_fg_good( bool val );
  VIDTK_INPUT_PORT( set_is_fg_good, bool );

  /// The output image.
  vil_image_view<PixType> image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image );

  /// Re-allocated output image.
  vil_image_view<PixType> copied_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, copied_image );

private:

  typedef std::vector< vil_structuring_element > structuring_element_vector;
  typedef std::vector< std::vector< std::ptrdiff_t > > offset_vector;

  // Operating modes of this filter
  enum{ CONSTANT_GSD, GSD_IMAGE, HASHED_GSD_IMAGE } mode_;

  // Internal parameter block
  config_block config_;

  // Possible inputs
  vil_image_view<PixType> src_img_;
  vil_image_view<double> gsd_img_;
  vil_image_view<HashType> gsd_hash_img_;
  double world_units_per_pixel_;
  bool is_fg_good_;

  // Parameters and settings
  double opening_radius_;
  double closing_radius_;

  // These parameters are added specifically for when objects may be very small
  double min_image_opening_radius_;
  double min_image_closing_radius_;
  double max_image_opening_radius_;
  double max_image_closing_radius_;

  // Information from last step for constant GSD mode
  double last_image_closing_radius_;
  double last_image_opening_radius_;
  double last_step_world_units_per_pixel_;

  // Decimation factor and follow-up filtering settings
  unsigned decimate_factor_;
  double extra_dilation_;
  unsigned dilation_size_threshold_;
  vil_structuring_element dilation_el_;
  vil_image_view< PixType > dilation_buffer_;

  // Scale factor of hashed image, if used
  double hashed_gsd_scale_factor_;

  // Structuring elements used for constant GSD mode
  vil_structuring_element opening_el_;
  vil_structuring_element closing_el_;

  // Structuring element containers and variables for variable GSD mode
  structuring_element_vector struct_elem_list_;
  offset_vector offset_list_;
  std::ptrdiff_t last_i_step_;
  std::ptrdiff_t last_j_step_;

  // Helper functions to simplify main step function
  bool gsd_image_step( const vil_image_view<double>& gsd_img );
  bool variable_size_step( const vil_image_view<HashType>& gsd_hash_img );
  bool constant_gsd_step();

  // Custom variable size binary/erosion helper functions
  HashType update_struct_element_lists( const vil_image_view<HashType>& gsd_hash,
                                        const double& radius_world_units,
                                        const double& min_radius_pixels,
                                        const double& max_radius_pixels );

  // Internal buffers used to prevent re-allocating temporary memory
  vil_image_view<PixType> buffer_;
  vil_image_view<HashType> interval_bins_;

  // Possible algorithm outputs
  vil_image_view<PixType> out_img_;
  mutable vil_image_view<PixType> copied_out_img_;
};


} // end namespace vidtk


#endif // vidtk_world_morphology_process_h_
