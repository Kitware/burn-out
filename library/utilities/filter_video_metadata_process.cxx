/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_video_metadata_process.h"

#include <string>
#include <sstream>
#include <iomanip>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_math.h>

#include <logger/logger.h>

VIDTK_LOGGER("filter_video_metadata_process_cxx");

namespace vidtk
{

filter_video_metadata_process
::filter_video_metadata_process( std::string const& _name )
: process( _name, "filter_video_metadata_process" ),
  disable_(true),
  allow_equal_time_(false),
  max_hfov_(0.0),
  max_alt_delta_(0.0),
  max_lat_delta_(0.0),
  max_lon_delta_(0.0),
  input_single_(NULL),
  input_multi_(NULL),
  previous_time_(0),
  previous_alt_(0.0),
  ts_()
{
  config_.add_parameter( "disabled",
    "false",
    "Whether or not this process is disabled, if so metadata will simply "
    "be passed along as-is." );

  config_.add_parameter( "max_hfov",
    "90",
    "Maximum horizontal field-of-view angle (in degrees) above which a "
    "metadata packet is marked as invalid. This field is checked "
    "independently on each image." );

  config_.add_parameter( "max_platform_altitude_change",
    "0.3",
    "Maximum change (fraction [0,1]) in platform altitude between two "
    "consecutive metadata packets." );

  config_.add_parameter( "max_lat_lon_change",
    "1.0 1.0",
    "Maximum change (in degrees) in [latitude longitude] values between"
    " two consecutive metadata packets. Currently it is reasonable to apply"
    " same threshold to all the position fields with latitude/longitude units." );

  config_.add_parameter( "allow_equal_time",
    "false",
    "When true, we consider metadata packets whose time is "
    "equal to the previous packet.  If the current packet "
    "is same as the previous packet, then it is filtered "
    "out.  Otherwise, it is passed on." );

  config_.add_optional( "scale_filename",
    "Do not compute GSD, but instead read precomputed values from a file. "
    "Set to 'help' for more info in LOG_INFO messages." );

  invalid_.is_valid( false );
}


filter_video_metadata_process
::~filter_video_metadata_process()
{
}


config_block
filter_video_metadata_process
::params() const
{
  return config_;
}


bool
filter_video_metadata_process
::set_params( config_block const& blk )
{
  try
  {
    disable_ = blk.get<bool>( "disabled" );

    if( !disable_ )
    {
      max_hfov_ = blk.get<double>( "max_hfov" );
      max_alt_delta_ = blk.get<double>( "max_platform_altitude_change" );
      vnl_double_2 max_lat_lon_delta = blk.get<vnl_double_2>( "max_lat_lon_change" );
      max_lat_delta_ = max_lat_lon_delta[0];
      max_lon_delta_ = max_lat_lon_delta[1];
      allow_equal_time_ = blk.get<bool>("allow_equal_time");
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
filter_video_metadata_process
::initialize()
{
  input_single_ = NULL;
  input_multi_ = NULL;
  previous_time_ = 0;
  previous_alt_ = 0.0;
  invalid_.is_valid(false);
  previous_packet_.is_valid(false);

  bool okay = true;
  if ( config_.has( "scale_filename" ))
  {
    std::string gsd_file_fn = config_.get<std::string>( "scale_filename" );
    if ( gsd_file_fn == "help" )
    {
      gsd_file_source_t::log_format_as_info();
      return false;
    }
    okay = gsd_file_source_.initialize( gsd_file_fn );
  }

  return okay;
}


// ----------------------------------------------------------------
/** Main processing step.
 *
 *
 */
bool
filter_video_metadata_process
::step()
{
  output_vector_.clear();       // clear outputs

  if( input_single_ && input_multi_ )
  {
    LOG_ERROR( this->name() << ": Cannot use both single and multi input ports." );
    return false;
  }

  if( disable_ )
  {
    LOG_INFO( this->name() << ": is disabled, passing the input along unchanged." );

    if( input_single_ )
    {
      output_vector_.push_back(*input_single_);
    }
    else if( input_multi_ )
    {
      output_vector_ = *input_multi_;
    }
    else
    {
      output_vector_.push_back(invalid_);
    }
    input_multi_ = NULL;
    input_single_ = NULL;
    return true;
  }

  //
  // If only single metadata object
  //
  if( !input_single_ && !input_multi_ )
  {
    LOG_ERROR( this->name() << ": Input data not provided." );
    return false;
  }

  //Test to see if it is valid
  if(input_single_)
  {
    video_metadata v = *input_single_;
    if(this->is_good(&v))
    {
      LOG_INFO( this->name() << " : Received valid metadata with ts=" << input_single_->timeUTC()
                             << ", valid corners: "<< input_single_->has_valid_corners() );

      output_vector_.push_back(v);
    }
  }

  //
  // Vector of metadata objects
  //
  if(input_multi_)
  {
    for( unsigned int i = 0; i < input_multi_->size(); ++i )
    {
      video_metadata v = (*input_multi_)[i];
      if( this->is_good(&v))
      {
         LOG_INFO( this->name() << " : Received valid metadata with ts=" << v.timeUTC()
                                <<", valid corners: "<< v.has_valid_corners() );
         output_vector_.push_back(v);
      }
    }
  }
  input_single_ = NULL;
  input_multi_ = NULL;
  return true;
}


// ----------------------------------------------------------------
/** Is metadata packet good
 *
 * This method test the supplied metadata packet to see if is good.
 * The packet must pass it's valid test (have 4 valid corners), It
 * must also have a valid time stamp. In addition:
 * - Its time must not be less than the last packet time seen. That is,
 *   all packets must have increasing timestamps;
 * - Horizontal field of view value, if non-zero, should not be
 *   unrealistically large;
 * - Platform altitude should not change drastically between
 *   two consecutive metadata packets.
 * - Any of the lat/lon location should not change drastically between
 *   two consecutive metadata packets.
 */
bool
filter_video_metadata_process
::is_good(video_metadata * in)
{
  if(!in)
  {
    LOG_ERROR( this->name() << ": Input is NULL" );
    return false;
  }

  if ( !in->is_valid() )
  {
    return false;
  }

  if ( !in->has_valid_time() )
  {
    return false;
  }

  vxl_uint_64 t = in->timeUTC();

  if(0 != t)
  {
    if( t < previous_time_ ||
        (t == previous_time_ && (!allow_equal_time_ || previous_packet_ == *in) ) )
    {
      return false;
    }
  }
  else
  {
    return false;
  }
  previous_time_ = t;
  previous_packet_ = *in;

  const double INVALID_DEFAULT = 0.0;

  // Horizontal field of view value, if non-zero, should not be
  // unrealistically large
  if( in->sensor_horiz_fov() != INVALID_DEFAULT &&
      in->sensor_horiz_fov() > max_hfov_ )
  {
    LOG_DEBUG( name() << ": Removing metadata packet "<< t
      <<" due to a high horizontal field-of-view value ("
      << in->sensor_horiz_fov() << ")." );
    return false;
  }

  // Platform altitude should not change drastically between
  // two consecutive metadata packets.
  if( in->platform_altitude() != INVALID_DEFAULT )
  {
    if( previous_alt_ != INVALID_DEFAULT )
    {
      // NOTE: Use of previous_alt_ in the denominator is intentional as it
      // relatively more trusted value than in->platform_altitude().
      double alt_delta = std::fabs( previous_alt_ - in->platform_altitude() ) / previous_alt_;

      if( alt_delta > max_alt_delta_ )
      {
        LOG_DEBUG( name() << ": Removing metadata packet "<< t <<" due to large ("
          << alt_delta <<") change in platform altitude value ( from "<< previous_alt_
          <<" to "<< in->platform_altitude() << ")." );
        return false;
      }
    }

    previous_alt_ = in->platform_altitude();
  }// if (in->platform_altitude() != INVALID_DEFAULT)

  if (!is_good_location( in->platform_location(), prev_platform_loc_ ) ||
      !is_good_location( in->frame_center(), prev_frame_center_ ) ||
      !is_good_location( in->corner_ul(), prev_corner_ul_ ) ||
      !is_good_location( in->corner_ur(), prev_corner_ur_ ) ||
      !is_good_location( in->corner_lr(), prev_corner_lr_ ) ||
      !is_good_location( in->corner_ll(), prev_corner_ll_ ))
  {
    LOG_DEBUG( name() << ": Removing metadata packet " << t);
    return false;
  }

  // Temporary location; should be cleaned up later.
  // TODO: Replace gsd with the world_image_width or equivalent.
  /*
    \todo Arslan's code review comments that need to be addressed.
    "This should be cleaned up, the file doesn't contain GSD anymore
    but the image width in meters."
   */
  if ( gsd_file_source_.is_valid() )
  {
    //
    // Compute a new Horizontal FOV angle field of the metadata based on a image width
    // in meters loaded from file and slant range from the metadata packet.
    //
    // "Retrieve" the GSD by asking gsd_file_source to look up the GSD
    // based on the timestamp.
    //

    if( !in->has_valid_slant_range() )
    {
      LOG_WARN( this->name() << ": Could not use the metadata packet " << in->timeUTC() << ", on frame with " << ts_ << " due to missing slant range." );
      return false;
    }

    if ( !ts_.is_valid() )
    {
      LOG_ERROR( "Stepped gen_world_units_per_pixel with gsd_file_source without setting timestamp?" );
      return false;
    }

    double scale = gsd_file_source_.get_gsd( ts_ );
    LOG_INFO( this->name() << ": Image width = " << scale << " (meters) via file lookup" );

    double slant = in->slant_range();

    // Now use the trignometry to come up with the horizontal FOV angle from scale and slant.
    double hfov = 2.0 * atan2( (scale / 2.0), slant ) * 180.0 / vnl_math::pi;

    LOG_TRACE( this->name() << ": replacing HFOV: from " << in->sensor_horiz_fov() << " to " << hfov );

    in->sensor_horiz_fov( hfov );
    // since we are over-riding the HFOV, we need to ensure that the unreliable VFOV is also removed.
    in->sensor_vert_fov( 0.0 );

    ts_ = timestamp();
  }

  return true;
}


void
filter_video_metadata_process
::set_timestamp( timestamp const & ts)
{
  ts_ = ts;
}

void
filter_video_metadata_process
::set_source_metadata( video_metadata const & in)
{
  input_single_ = &in;
}

void
filter_video_metadata_process
::set_source_metadata_vector( std::vector< video_metadata > const& in)
{
  input_multi_ = &in;
}

std::vector< video_metadata >
filter_video_metadata_process
::output_metadata_vector() const
{
  return output_vector_;
}

video_metadata
filter_video_metadata_process
::output_metadata() const
{
  if( ! output_vector_.empty())
  {
    return output_vector_[0];
  }
  return invalid_;
}

bool
filter_video_metadata_process
::is_good_location( geo_coord::geo_lat_lon const& new_loc,
                    geo_coord::geo_lat_lon & prev_loc ) const
{
  // Lat/Lon location should not change drastically between
  // two consecutive metadata packets.

  if( new_loc.is_valid() )
  {
    if( prev_loc.is_valid() )
    {
      double lat_delta = std::fabs( prev_loc.get_latitude() - new_loc.get_latitude() );
      double lon_delta = std::fabs( prev_loc.get_longitude() - new_loc.get_longitude() );

      if( lat_delta > max_lat_delta_ || lon_delta > max_lon_delta_)
      {
        std::stringstream strs;
        strs << std::setprecision( 16 )
             << "large change in location ( from "
             << prev_loc << " to "
             << new_loc << ").\n";
        LOG_ERROR(strs.str());
        return false;
      }
    }

    prev_loc = new_loc;
  }

  return true;
}

}// end namespace vidtk
