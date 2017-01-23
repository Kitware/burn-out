/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/homography_to_scale_image_process.h>

#include <video_transforms/floating_point_image_hash_functions.h>

#include <utilities/compute_gsd.h>
#include <utilities/horizon_detection_functions.h>

#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_binary_erode.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/vil_math.h>
#include <vnl/vnl_math.h>
#include <vil/vil_border.h>
#include <vil/vil_crop.h>
#include <vgl/vgl_box_2d.h>

#include <algorithm>
#include <string>
#include <vector>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("homography_to_scale_image_process");


template<typename HashType>
homography_to_scale_image_process<HashType>
::homography_to_scale_image_process( std::string const& _name )
  : process( _name, "homography_to_scale_image_process" ),
    operating_mode_( HASHED_GSD_IMAGE ),
    algorithm_( GRID ),
    grid_sampling_height_( 8 ),
    grid_sampling_width_( 10 ),
    estimate_horizon_location_( true ),
    hashed_gsd_scale_factor_( 10 ),
    hashed_max_gsd_( 10 ),
    is_first_frame_( true ),
    process_frame_( true )
{
  config_.add_parameter( "mode",
                         "HASHED_GSD_IMAGE",
                         "Operating mode of the filter "
                         "(Acceptable values: GSD_IMAGE, HASHED_GSD_IMAGE). "
                         "Either a GSD image will be produced, outputting an image "
                         "with an estimated GSD for each pixel, or a hashed gsd image "
                         "with the estimated GSD mapped onto an integral value. "
                         "See class documentation for more information." );

  config_.add_parameter( "algorithm",
                         "GRID",
                         "Operating mode of the filter "
                         "(Acceptable values: GRID)."
                         "Algorithm grid will before a windowing across the image,"
                         "estimating the GSD for each windowed region. "
                         "See class documentation for more information. " );

  config_.add_parameter( "grid_sampling_height",
                         "18",
                         "Default height (resolution) of sampling grid if using the GRID "
                         "algorithm. The number of bins we want in the vertical direction "
                         "for every row in the sampling grid." );

  config_.add_parameter( "grid_sampling_width",
                         "25",
                         "Default width (resolution) of sampling grid if using the GRID "
                         "algorithm. The number of bins we want in the horizontal direction "
                         "for every col in the sampling grid." );

  config_.add_parameter( "estimate_horizon_position",
                         "true",
                         "If the inputted homography contains a vanishing line (horizon) "
                         "should we estimate its location, and then treat everything above it "
                         "as the maximum allowable GSD automatically? Set to true if so." );

  config_.add_parameter( "hashed_gsd_scale_factor",
                         "10.0",
                         "The scale factor between values in the integral hashed gsd output image, "
                         "and the actual gsd for those pixels, if in HASHED_GSD_IMAGE mode." );

  config_.add_parameter( "hashed_max_gsd",
                         "100",
                         "The maximum GSD to report, if in HASHED_GSD_IMAGE mode. Enter 0 for "
                         "infinity." );
}


template<typename HashType>
config_block
homography_to_scale_image_process<HashType>
::params() const
{
  return config_;
}


template<typename HashType>
bool
homography_to_scale_image_process<HashType>
::set_params( config_block const& blk )
{
  std::string mode_str, alg_str;

  try
  {

    mode_str = blk.get<std::string>( "mode" );

    if( mode_str == "GSD_IMAGE" )
    {
      operating_mode_ = GSD_IMAGE;
    }
    else if( mode_str == "HASHED_GSD_IMAGE" )
    {
      operating_mode_ = HASHED_GSD_IMAGE;

      hashed_gsd_scale_factor_ = blk.get<double>( "hashed_gsd_scale_factor" );
      hashed_max_gsd_ = blk.get<double>( "hashed_max_gsd" );

      if( hashed_max_gsd_ <= 0 )
      {
        hashed_max_gsd_ = std::numeric_limits<HashType>::max();
      }
    }
    else
    {
      throw config_block_parse_error( "Invalid mode specified!" );
    }

    alg_str = blk.get<std::string>( "algorithm" );

    if( alg_str == "GRID" )
    {
      algorithm_ = GRID;

      grid_sampling_height_ = blk.get<unsigned>( "grid_sampling_height" );
      grid_sampling_width_ = blk.get<unsigned>( "grid_sampling_width" );
    }
    else
    {
      throw config_block_parse_error( "Invalid algorithm type specified" );
    }

    estimate_horizon_location_ = blk.get<bool>( "estimate_horizon_position" );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template<typename HashType>
bool
homography_to_scale_image_process<HashType>
::initialize()
{
  return true;
}

// Fill in a rectangular region of the image with some value
template <typename PixType>
void fill_region( vil_image_view<PixType>& img,
                  const vgl_box_2d<unsigned>& region,
                  const PixType& value )
{
  assert( img.ni() >= region.max_x() );
  assert( img.nj() >= region.max_y() );

  vil_image_view< PixType > cropped = vil_crop( img,
                                                region.min_x(),
                                                region.width(),
                                                region.min_y(),
                                                region.height() );

  cropped.fill( value );
}


// Calculate the GSD for different portions of the image by gridding it
// up into rectangles, and approximating the GSD for each region. It may
// be more efficient to do a planar backprojection later, but this is good
// enough for now.
void create_grided_gsd_image( const unsigned& ni,
                              const unsigned& nj,
                              const homography::transform_t& homog,
                              const unsigned& grid_width,
                              const unsigned& grid_height,
                              bool fill_region_above_horizon,
                              vil_image_view<double>& dst )
{
  // Confirm output size and get required variables for iteration
  dst.set_size( ni, nj, 1 );

  // Confirm that simpling rates are smaller than the 0.5*image boundaries
  unsigned adj_grid_width = ( grid_width < ni/2 ? grid_width : ni/2 );
  unsigned adj_grid_height = ( grid_height < nj/2 ? grid_height : nj/2 );

  if( adj_grid_width < 1 )
  {
    adj_grid_width = 1;
  }

  if( adj_grid_height < 1 )
  {
    adj_grid_height = 1;
  }

  // Should we check each region to see if it is above the horizon?
  bool check_horizon = fill_region_above_horizon;
  const double max_gsd_value = std::numeric_limits<double>::max();

  // Check to see if the vanishing line exists in the given homog,
  // and to make sure it is not a vertical line
  check_horizon = ( contains_horizon_line(homog) ? check_horizon : false );

  // Check to see if the vanishing line intersects with the image plane
  check_horizon = ( check_horizon ? does_horizon_intersect(ni,nj,homog) : false );

  // Formulate grided regions
  std::vector< vgl_box_2d< unsigned > > regions;
  for( unsigned j = 0; j < adj_grid_height; j++ )
  {
    for( unsigned i = 0; i < adj_grid_width; i++ )
    {
      // Top left point for region
      unsigned ti = ( i * ni ) / adj_grid_width;
      unsigned tj = ( j * nj ) / adj_grid_height;

      // Bottom right
      unsigned bi = ( (i+1) * ni ) / adj_grid_width;
      unsigned bj = ( (j+1) * nj ) / adj_grid_height;

      // Formulate rect region
      vgl_box_2d<unsigned> region( ti, bi, tj, bj );

      if( check_horizon && is_region_above_horizon(region,homog) )
      {
        fill_region( dst, region, max_gsd_value );
        continue;
      }

      regions.push_back( region );
    }
  }

  // Process each region below the horizon
  for( unsigned int r = 0; r < regions.size(); r++ )
  {
    // Calculate estimated GSD for region
    double GSD_estimate = compute_gsd( regions[r], homog.get_matrix() );

    // Fill region
    fill_region( dst, regions[r], GSD_estimate );
  }
}

template<typename HashType>
bool
homography_to_scale_image_process<HashType>
::step()
{
  // Exit right away if we receive a don't process flag
  if( !process_frame_ )
  {
    return true;
  }

  // Check to see if this is a repeat homography
  bool is_repeat_homography = ( !is_first_frame_ && last_homog_ == H_src2wld_ );

  // Generate GSD image if we don't have a repeat homography
  if( !is_repeat_homography )
  {
    if( algorithm_ == GRID )
    {
      create_grided_gsd_image( ni_,
                               nj_,
                               H_src2wld_.get_transform(),
                               grid_sampling_width_,
                               grid_sampling_height_,
                               estimate_horizon_location_,
                               gsd_img_ );
    }
    else
    {
      LOG_ERROR( this->name() << ": unknown algorithm specified!\n" );
      return false;
    }
  }

  // Generate hashed GSD image if required
  if( !is_repeat_homography && operating_mode_ == HASHED_GSD_IMAGE )
  {
    scale_float_img_to_interval( gsd_img_,
                                 hashed_gsd_img_,
                                 hashed_gsd_scale_factor_,
                                 hashed_max_gsd_ );
  }

  // Reset inputs
  last_homog_ = H_src2wld_;
  process_frame_ = true;
  is_first_frame_ = false;
  return true;
}

// Input and output accessor functions
template<typename HashType>
void
homography_to_scale_image_process<HashType>
::set_image_to_world_homography( image_to_plane_homography const& H )
{
  H_src2wld_ = H;
}

template<typename HashType>
void
homography_to_scale_image_process<HashType>
::set_resolution( resolution_t const& res )
{
  ni_ = res.first;
  nj_ = res.second;
}


template<typename HashType>
void
homography_to_scale_image_process<HashType>
::set_is_valid_flag( bool val )
{
  process_frame_ = val;
}

template<typename HashType>
vil_image_view<double>
homography_to_scale_image_process<HashType>
::gsd_image() const
{
  return gsd_img_;
}

template<typename HashType>
vil_image_view<double>
homography_to_scale_image_process<HashType>
::copied_gsd_image() const
{
  copied_gsd_img_ = vil_image_view<double>();
  copied_gsd_img_.deep_copy( gsd_img_ );
  return copied_gsd_img_;
}

template<typename HashType>
vil_image_view<HashType>
homography_to_scale_image_process<HashType>
::hashed_gsd_image() const
{
  return hashed_gsd_img_;
}

template<typename HashType>
vil_image_view<HashType>
homography_to_scale_image_process<HashType>
::copied_hashed_gsd_image() const
{
  copied_hashed_gsd_img_ = vil_image_view<double>();
  copied_hashed_gsd_img_.deep_copy( hashed_gsd_img_ );
  return copied_hashed_gsd_img_;
}


} // end namespace vidtk
