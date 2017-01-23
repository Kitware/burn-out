/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "world_morphology_process.h"

#include <video_transforms/variable_image_erode.h>
#include <video_transforms/variable_image_dilate.h>
#include <video_transforms/floating_point_image_hash_functions.h>

#include <tracking_data/image_object.h>

#include <vil/algo/vil_blob.h>
#include <vil/algo/vil_binary_erode.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_greyscale_erode.h>
#include <vil/algo/vil_greyscale_dilate.h>

#include <vil/vil_math.h>
#include <vil/vil_decimate.h>

#include <cassert>
#include <algorithm>
#include <string>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER ("world_morphology_process");


template<typename PixType,typename HashType>
world_morphology_process<PixType,HashType>
::world_morphology_process( std::string const& _name )
  : process( _name, "world_morphology_process" ),
    mode_( CONSTANT_GSD ),
    src_img_( NULL ),
    gsd_img_( NULL ),
    gsd_hash_img_( NULL ),
    world_units_per_pixel_( 1 ),
    is_fg_good_( true ),
    opening_radius_( 0 ),
    closing_radius_( 0 ),
    min_image_opening_radius_( 0 ),
    min_image_closing_radius_( 0 ),
    max_image_opening_radius_( 0 ),
    max_image_closing_radius_( 0 ),
    last_image_closing_radius_( 0 ),
    last_image_opening_radius_( 0 ),
    last_step_world_units_per_pixel_( 1 ),
    decimate_factor_( 1 ),
    hashed_gsd_scale_factor_( 10 ),
    last_i_step_( 0 ),
    last_j_step_( 0 )
{
  config_.add_parameter( "mode",
                         "CONSTANT_GSD",
                         "Operating mode of the filter "
                         "(Acceptable values: CONSTANT_GSD, GSD_IMAGE, or HASHED_GSD_IMAGE)."
                         "This process can either accept a single GSD value for the entire image, "
                         "a GSD image with a per-pixel GSD, or a GSD image hashed to integral values.");

  config_.add_parameter( "opening_radius",
                         "0",
                         "Opening radius per world unit." );

  config_.add_parameter( "min_image_opening_radius",
                         "0",
                         "Minimum opening radius in pixels." );

  config_.add_parameter( "max_image_opening_radius",
                         "4",
                         "Maximum opening radius in pixels." );

  config_.add_parameter( "closing_radius",
                         "0",
                         "Clasing radius per world unit." );

  config_.add_parameter( "min_image_closing_radius",
                         "0",
                         "Minimum closing radius in pixels." );

  config_.add_parameter( "max_image_closing_radius",
                         "4",
                         "Maximum clasing radius in pixels." );

  config_.add_parameter( "world_units_per_pixel",
                         "1",
                         "Default GSD for constand GSD mode if not provided." );

  config_.add_parameter( "decimate_factor",
                         "1",
                         "Run morphology on a decimated version of the input image." );

  config_.add_parameter( "extra_dilation",
                         "0",
                         "Perform a dilation operation on the morphology output." );

  config_.add_parameter( "dilation_size_threshold",
                         "0",
                         "Blob size requirements in order to perform dilation." );

  config_.add_parameter( "hashed_gsd_scale_factor",
                         "10.0",
                         "The scale factor between values in the integral hashed gsd output image, "
                         "and the actual gsd for those pixels." );
}


template<typename PixType,typename HashType>
config_block
world_morphology_process<PixType,HashType>
::params() const

{
  return config_;
}


// A note on the settings for this function: ideally we should have a
// single boolean which controls whether or not we're closing or opening,
// not having a seperate radius for both and enforcing that only one of
// them can be set. This is only in here for legacy reasons, so config
// files don't have to change.
template<typename PixType,typename HashType>
bool
world_morphology_process<PixType,HashType>
::set_params( config_block const& blk )
{
  std::string mode_str;

  try
  {

    mode_str = blk.get<std::string>( "mode" );

    if( mode_str == "CONSTANT_GSD" )
    {
      mode_ = CONSTANT_GSD;
    }
    else if( mode_str == "GSD_IMAGE" )
    {
      mode_ = GSD_IMAGE;
    }
    else if( mode_str == "HASHED_GSD_IMAGE" )
    {
      mode_ = HASHED_GSD_IMAGE;
    }
    else
    {
      throw config_block_parse_error( "Invalid mode specified" );
    }

    // Get shared settings
    opening_radius_ = blk.get<double>( "opening_radius" );
    closing_radius_ = blk.get<double>( "closing_radius" );

    if( opening_radius_ > 0 && closing_radius_ > 0 )
    {
      throw config_block_parse_error( "Process can only open or close, not both" );
    }

    world_units_per_pixel_ = blk.get<double>( "world_units_per_pixel" );

    if ( fabs( world_units_per_pixel_ ) < 1e-6 )
    {
      LOG_WARN( this->name() << ": default world_units are extremely small!\n");
    }

    min_image_opening_radius_ = blk.get<double>( "min_image_opening_radius" );
    max_image_opening_radius_ = blk.get<double>( "max_image_opening_radius" );
    min_image_closing_radius_ = blk.get<double>( "min_image_closing_radius" );
    max_image_closing_radius_ = blk.get<double>( "max_image_closing_radius" );

    decimate_factor_ = blk.get<unsigned>( "decimate_factor" );
    extra_dilation_ = blk.get<double>( "extra_dilation" );
    dilation_size_threshold_ = blk.get< unsigned>( "dilation_size_threshold" );

    if( extra_dilation_ > 0.0 )
    {
      dilation_el_.set_to_disk( extra_dilation_ );
    }

    // Load modal specific settings
    if( mode_ == HASHED_GSD_IMAGE )
    {
      hashed_gsd_scale_factor_ = blk.get<double>( "hashed_gsd_scale_factor" );
    }
    else if( mode_ == GSD_IMAGE )
    {
      if( opening_radius_ > 0 )
      {
        hashed_gsd_scale_factor_ = 10 / opening_radius_;
      }
      else if( closing_radius_ > 0 )
      {
        hashed_gsd_scale_factor_ = 10 / closing_radius_;
      }
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}

template<typename PixType,typename HashType>
bool
world_morphology_process<PixType,HashType>
::initialize()
{
  return true;
}

template<typename PixType,typename HashType>
bool
world_morphology_process<PixType,HashType>
::reset()
{
  return this->initialize();
}

// Main step function
template<typename PixType,typename HashType>
bool
world_morphology_process<PixType,HashType>
::step()
{
  // Exit right away if fg image is marked as being not valid
  if( !is_fg_good_ )
  {
    return true;
  }

  // Validate the input fg image, as all modes use it
  if( !src_img_ )
  {
    LOG_ERROR( this->name() << ": invalid input image!\n" );
    return false;
  }

  bool is_okay = true;

  if( mode_ == CONSTANT_GSD )
  {
    is_okay = this->constant_gsd_step();
  }
  else if( mode_ == GSD_IMAGE )
  {
    // Validate the GSD Image Input
    if( !gsd_img_ )
    {
      LOG_ERROR( this->name() << ": invalid gsd image!\n" );
      return false;
    }
    // Run custom GSD mode step
    is_okay = this->gsd_image_step( gsd_img_ );
  }
  else if( mode_ == HASHED_GSD_IMAGE )
  {
    // Validate the hashed GSD Image Input
    if( !gsd_hash_img_ )
    {
      LOG_ERROR( this->name() << ": invalid scaled gsd image!\n" );
      return false;
    }
    // Run hashed GSD mode step
    is_okay = this->variable_size_step( gsd_hash_img_ );
  }
  else
  {
    LOG_ERROR( this->name() << ": invalid operating mode specified!\n" );
    return false;
  }

  // Reset inputs
  src_img_ = NULL;
  gsd_img_ = NULL;
  gsd_hash_img_ = NULL;

  return is_okay;
}


// Wrapper around vil morph functions for type-specific constant GSD operands
template< typename PixType >
void constant_gsd_dilate( const vil_image_view<PixType>& src_image,
                          vil_image_view<PixType>& dest_image,
                          const vil_structuring_element& element )
{
  vil_greyscale_dilate( src_image, dest_image, element );
}

template <>
void constant_gsd_dilate<bool>( const vil_image_view<bool>& src_image,
                                vil_image_view<bool>& dest_image,
                                const vil_structuring_element& element )
{
  vil_binary_dilate( src_image, dest_image, element );
}

template< typename PixType >
void constant_gsd_erode( const vil_image_view<PixType>& src_image,
                          vil_image_view<PixType>& dest_image,
                          const vil_structuring_element& element )
{
  vil_greyscale_erode( src_image, dest_image, element );
}

template <>
void constant_gsd_erode<bool>( const vil_image_view<bool>& src_image,
                               vil_image_view<bool>& dest_image,
                               const vil_structuring_element& element )
{
  vil_binary_erode( src_image, dest_image, element );
}


// Step function for constant GSD mode
template< typename PixType, typename HashType >
bool
world_morphology_process<PixType,HashType>
::constant_gsd_step()
{
  assert( src_img_ != NULL );

  vil_image_view<PixType> input_img;
  vil_image_view<PixType> morph_img;

  // Resize image if set
  if( decimate_factor_ > 1 )
  {
    input_img = vil_decimate( src_img_, decimate_factor_ );
  }
  else
  {
    input_img = src_img_;
  }

  // Filter size
  if( dilation_size_threshold_ > 0 )
  {
    vil_image_view<unsigned> labels_whole;
    std::vector<vil_blob_region> blobs;
    std::vector<vil_blob_region>::iterator it_blobs;

    vil_blob_labels( input_img, vil_blob_4_conn, labels_whole );
    vil_blob_labels_to_regions( labels_whole, blobs );

    for( it_blobs = blobs.begin(); it_blobs < blobs.end(); ++it_blobs )
    {
      std::vector< vgl_point_2d< image_object::float_type > > pts;
      vil_blob_region::iterator it_chords;

      if( it_blobs->size() >= dilation_size_threshold_ )
      {
        continue;
      }

      for( it_chords = it_blobs->begin(); it_chords < it_blobs->end(); ++it_chords )
      {
        for( unsigned i = it_chords->ilo; i <= it_chords->ihi; ++i )
        {
          input_img( i, it_chords->j) = false;
        }
      }
    }
  }

  // We need to update the structuring element if the set_world_units_per_pixel
  // changed since last run.

  // Structuring element will take a double for the radius.
  double image_opening_radius = opening_radius_ / world_units_per_pixel_;
  double image_closing_radius = closing_radius_ / world_units_per_pixel_;

  if ( last_image_opening_radius_ != image_opening_radius &&
        opening_radius_ > 0 )
  {
    double rad = std::max( min_image_opening_radius_, image_opening_radius );
    rad = std::min( max_image_opening_radius_, rad );
    opening_el_.set_to_disk( rad );
    last_image_opening_radius_ = image_opening_radius;
  }
  if ( last_image_closing_radius_ != image_closing_radius &&
        closing_radius_ > 0 )
  {
    double rad = std::max( min_image_closing_radius_, image_closing_radius );
    rad = std::min( max_image_closing_radius_, rad );
    closing_el_.set_to_disk( rad );
    last_image_closing_radius_ = image_closing_radius;
  }

  // We call erode and dilate directly instead of calling open and
  // close because we want to use our own intermediate buffer to avoid
  // reallocating it at every frame.
  if( world_units_per_pixel_ * opening_radius_ > 0 )
  {
    constant_gsd_erode( input_img, buffer_, opening_el_ );
    constant_gsd_dilate( buffer_, morph_img, opening_el_ );
  }
  else if( world_units_per_pixel_ * closing_radius_ > 0 )
  {
    constant_gsd_dilate( input_img, buffer_, closing_el_ );
    constant_gsd_erode( buffer_, morph_img, closing_el_ );
  }
  else
  {
    morph_img = input_img;
  }

  // Perform extra dilation
  if( extra_dilation_ > 0.0 && src_img_.size() > 500000 )
  {
    constant_gsd_dilate( morph_img, dilation_buffer_, dilation_el_ );
    morph_img = dilation_buffer_;
  }

  // Upscale image if required
  if( decimate_factor_ > 1 )
  {
    out_img_ = src_img_;

    const std::ptrdiff_t istep = out_img_.istep();

    for( unsigned j = 0; j < out_img_.nj(); ++j )
    {
      PixType* ptr = &out_img_( 0, j );

      for( unsigned i = 0; i < out_img_.ni(); ++i, ptr+=istep )
      {
        *ptr = morph_img( i / decimate_factor_, j / decimate_factor_ );
      }
    }
  }
  else
  {
    out_img_ = morph_img;
  }

  return true;
}


// Helper function which maintains the internal list of structuring elements
// such that the code is not repeated for both variable dilation and erosion.
// Returns the maximum GSD hash in the current image. Used to store structuring
// elements across frames so no recomputation is required. Returns the minimum
// value in the hash_image (smallest GSD bin) for possible later use.
template<typename PixType,typename HashType>
HashType
world_morphology_process<PixType,HashType>
::update_struct_element_lists( const vil_image_view<HashType>& gsd_hash,
                               const double& radius_world_units,
                               const double& min_radius_pixels,
                               const double& max_radius_pixels )
{
  // Get required input properties
  std::ptrdiff_t s_istep = src_img_.istep(), s_jstep = src_img_.jstep();

  // Make sure the istep and jstep hasn't changed, if it has flush our
  // known, precomputed offset and structuring element lists
  if( last_i_step_ != s_istep || last_j_step_ != s_jstep )
  {
    struct_elem_list_.clear();
    offset_list_.clear();
    last_i_step_ = s_istep;
    last_j_step_ = s_jstep;
  }

  // Precompute any structuring elements we may possibly require
  HashType min_value, max_value;
  vil_math_value_range( gsd_hash, min_value, max_value );

  if( max_value >= struct_elem_list_.size() )
  {
    // Record any new structuring elements we might need, and update
    // the corresponding offset list
    for( unsigned i = struct_elem_list_.size(); i <= max_value; i++ )
    {
      vil_structuring_element new_elem;

      double radius;

      if( i != 0 )
      {
        radius = radius_world_units /
         convert_interval_scale_to_float( i, hashed_gsd_scale_factor_ );
      }
      else
      {
        radius = max_radius_pixels;
      }

      // Check radius against bounds
      if( radius < min_radius_pixels )
      {
        radius = min_radius_pixels;
      }
      if( radius > max_radius_pixels )
      {
        radius = max_radius_pixels;
      }

      // If radius rounds to 0, cause a 1x1 kernel to be created resulting in no-op
      // when that kernel is called for a particular pixel
      if( radius == 0 )
      {
        radius = 0.1;
      }

      new_elem.set_to_disk( radius );
      struct_elem_list_.push_back( new_elem );
      offset_list_.push_back( std::vector< std::ptrdiff_t >() );
      vil_compute_offsets( offset_list_[i], new_elem, s_istep, s_jstep );
    }
  }

  return min_value;
}

// Step function for morphology size map mode
template<typename PixType,typename HashType>
bool
world_morphology_process<PixType,HashType>
::variable_size_step( const vil_image_view<HashType>& gsd_hash_img )
{
  // Update the list of any strucuring elements we may need and get
  // simultaneously get the maximum value in the image
  HashType min_value = 0;

  if ( closing_radius_ > 0 )
  {
    min_value = update_struct_element_lists( gsd_hash_img,
                                             closing_radius_,
                                             min_image_closing_radius_,
                                             max_image_closing_radius_ );
  }
  else if( opening_radius_ > 0 )
  {
    min_value = update_struct_element_lists( gsd_hash_img,
                                             opening_radius_,
                                             min_image_opening_radius_,
                                             max_image_opening_radius_ );
  }

  // We call erode and dilate directly instead of calling open and
  // close because we want to use our own intermediate buffer to avoid
  // reallocating it at every frame.
  if( opening_radius_ > 0 )
  {
    variable_image_erode( src_img_,
                          gsd_hash_img,
                          struct_elem_list_,
                          offset_list_,
                          min_value,
                          buffer_ );

    variable_image_dilate( buffer_,
                           gsd_hash_img,
                           struct_elem_list_,
                           offset_list_,
                           min_value,
                           out_img_ );
  }
  else if( closing_radius_ > 0 )
  {
    variable_image_dilate( src_img_,
                           gsd_hash_img,
                           struct_elem_list_,
                           offset_list_,
                           min_value,
                           buffer_ );

    variable_image_erode( buffer_,
                          gsd_hash_img,
                          struct_elem_list_,
                          offset_list_,
                          min_value,
                          out_img_ );
  }
  else
  {
    out_img_ = src_img_;
  }

  return true;
}

// Step function for GSD image mode
template<typename PixType,typename HashType>
bool
world_morphology_process<PixType,HashType>
::gsd_image_step( const vil_image_view<double>& gsd_img )
{
  assert( gsd_img.nplanes() == 1 );

  // Bin the input image such that every possible gsd in the image
  // gets associated with some descritized id corresponding to some
  // gsd range interval

  if( opening_radius_ > 0 )
  {
    scale_float_img_to_interval( gsd_img,
                                interval_bins_,
                                hashed_gsd_scale_factor_,
                                opening_radius_ );
  }
  else if( closing_radius_ > 0 )
  {
    scale_float_img_to_interval( gsd_img,
                                interval_bins_,
                                hashed_gsd_scale_factor_,
                                closing_radius_ );
  }
  else
  {
    out_img_ = src_img_;
  }

  // Call prior step function
  return variable_size_step( interval_bins_ );
}


// Input and output accessor functions
template<typename PixType,typename HashType>
void
world_morphology_process<PixType,HashType>
::set_source_image( vil_image_view<PixType> const& img )
{
  src_img_ = img;
}

template<typename PixType,typename HashType>
void
world_morphology_process<PixType,HashType>
::set_world_units_per_pixel( double units_per_pix )
{
  world_units_per_pixel_ = units_per_pix;
}

template<typename PixType,typename HashType>
void
world_morphology_process<PixType,HashType>
::set_gsd_image( vil_image_view<double> const& img )
{
  gsd_img_ = img;
}

template<typename PixType,typename HashType>
void
world_morphology_process<PixType,HashType>
::set_hashed_gsd_image( vil_image_view<HashType> const& img )
{
  gsd_hash_img_ = img;
}

template<typename PixType,typename HashType>
void
world_morphology_process<PixType,HashType>
::set_is_fg_good( bool val )
{
  is_fg_good_ = val;
}

template<typename PixType,typename HashType>
vil_image_view<PixType>
world_morphology_process<PixType,HashType>
::image() const
{
  return out_img_;
}


template<typename PixType,typename HashType>
vil_image_view<PixType>
world_morphology_process<PixType,HashType>
::copied_image() const
{
  copied_out_img_ = vil_image_view<PixType>();
  copied_out_img_.deep_copy( out_img_ );
  return copied_out_img_;
}

} // end namespace vidtk
