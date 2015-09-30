/*ckwg +5
 * Copyright 2010-11 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_video_metadata_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vcl_cmath.h>
#include <vcl_sstream.h>
#include <vcl_iomanip.h>

namespace vidtk
{

filter_video_metadata_process
::filter_video_metadata_process( vcl_string const& name )
: process( name, "filter_video_metadata_process" ),
  disable_(true),
  max_hfov_(0.0),
  max_alt_delta_(0.0),
  max_lat_delta_(0.0),
  max_lon_delta_(0.0),
  input_single_(NULL),
  input_multi_(NULL),
  previous_time_(0),
  previous_alt_(0.0)
{
  config_.add( "disabled", "false" );

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

  invalid_.is_valid(false);
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
    blk.get( "disabled", disable_ );
    blk.get( "max_hfov", max_hfov_ );
    blk.get( "max_platform_altitude_change", max_alt_delta_ );
    double max_lat_lon_delta[2];
    blk.get( "max_lat_lon_change", max_lat_lon_delta );
    max_lat_delta_ = max_lat_lon_delta[0];
    max_lon_delta_ = max_lat_lon_delta[1];
  }
  catch( unchecked_return_value const& e )
  {
    log_error (name() << ": set_params() failed: "<< e.what() <<"\n");
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
  return true;
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

  if(input_single_ && input_multi_)
  {
    log_error( this->name() << ": Cannot use both single and multi input ports.\n" );
    return false;
  }

  if(disable_)
  {
    log_info( this->name() << ": is disabled, passing the input along unchanged.\n" );

    if(input_single_)
    {
      output_vector_.push_back(*input_single_);
    }
    else if( input_multi_)
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
    log_error( this->name() << ": Input data not provided.\n" );
    return false;
  }

  //Test to see if it is valid
  if(input_single_ && this->is_good(input_single_))
  {
    log_info( this->name() << " : Received valid metadata with ts=" << input_single_->timeUTC()
                           << ", valid corners: "<< input_single_->has_valid_corners() << "\n" );

    output_vector_.push_back(*input_single_);
  }

  //
  // Vector of metadata obejects
  //
  if(input_multi_)
  {
    for( unsigned int i = 0; i < input_multi_->size(); ++i )
    {
      video_metadata const * v = &(*input_multi_)[i];
      if( this->is_good(v))
      {
         log_info( this->name() << " : Received valid metadata with ts=" << v->timeUTC()
                                <<", valid corners: "<< v->has_valid_corners() << "\n" );
         output_vector_.push_back(*v);
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
::is_good(video_metadata const * in)
{
  if(!in)
  {
    log_error( this->name() << ": Input is NULL\n" );
    return false;
  }

  if ( !in->is_valid() )
  {
    //log_debug( "Invalid metadata packet\n" );
    return false;
  }

  if ( !in->has_valid_time() )
  {
    //log_debug( this->name() ": Does not have valid time.\n" );
    return false;
  }

  vxl_uint_64 t = in->timeUTC();

  //log_debug( "TIME is: " << (unsigned long)t << vcl_endl );

  if(0 != t)
  {
    if(t <= previous_time_)
    {
//      log_debug( this->name() << ": Current metadata timestamp is less or equal"
//                 << " to previous timestamp " << (unsigned long)t << " "
//                 << (unsigned long)previous_time_ << vcl_endl );
      return false;
    }
  }
  else
  {
    //log_debug( "Input does not have time\n" );
    return false;
  }
  //log_debug( "Time change: " << (unsigned long)previous_time_ << "  :to:  " << (unsigned long)t << vcl_endl);
  previous_time_ = t;

  const double INVALID_DEFAULT = 0.0;

  // Horizontal field of view value, if non-zero, should not be
  // unrealistically large
  if( in->sensor_horiz_fov() != INVALID_DEFAULT &&
      in->sensor_horiz_fov() > max_hfov_ )
  {
    log_debug( name() << ": Removing metadata packet "<< t
      <<" due to a high horizontal field-of-view value ("
      << in->sensor_horiz_fov() << ").\n");
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
      double alt_delta = vcl_fabs( previous_alt_ - in->platform_altitude() ) / previous_alt_;

      if( alt_delta > max_alt_delta_ )
      {
        log_debug( name() << ": Removing metadata packet "<< t <<" due to large ("
          << alt_delta <<") change in platform altitude value ( from "<< previous_alt_
          <<" to "<< in->platform_altitude() << ").\n");
        return false;
      }
    }

    previous_alt_ = in->platform_altitude();
  }// if (in->platform_altitude() != INVALID_DEFAULT)

  try
  {
    is_good_location( in->platform_location(), prev_platform_loc_ );
    is_good_location( in->frame_center(), prev_frame_center_ );
    is_good_location( in->corner_ul(), prev_corner_ul_ );
    is_good_location( in->corner_ur(), prev_corner_ur_ );
    is_good_location( in->corner_lr(), prev_corner_lr_ );
    is_good_location( in->corner_ll(), prev_corner_ll_ );
  }
  catch( unchecked_return_value const& e )
  {
    log_debug( name() << ": Removing metadata packet "<< t <<" due to: " << e.what() << "\n" );
    return false;
  }

  return true;
}


void
filter_video_metadata_process
::set_source_metadata( video_metadata const & in)
{
  input_single_ = &in;
}

void
filter_video_metadata_process
::set_source_metadata_vector( vcl_vector< video_metadata > const& in)
{
  input_multi_ = &in;
}

vcl_vector< video_metadata > const &
filter_video_metadata_process
::output_metadata_vector() const
{
  return output_vector_;
}

video_metadata const &
filter_video_metadata_process
::output_metadata() const
{
  if(output_vector_.size())
  {
    return output_vector_[0];
  }
  return invalid_;
}

checked_bool
filter_video_metadata_process
::is_good_location( video_metadata::lat_lon_t const& new_loc,
                    video_metadata::lat_lon_t & prev_loc ) const
{
  // Lat/Lon location should not change drastically between
  // two consecutive metadata packets.

  if( new_loc.is_valid() )
  {
    if( prev_loc.is_valid() )
    {
      double lat_delta = vcl_fabs( prev_loc.lat() - new_loc.lat() );
      double lon_delta = vcl_fabs( prev_loc.lon() - new_loc.lon() );

      if( lat_delta > max_lat_delta_ || lon_delta > max_lon_delta_)
      {
        vcl_stringstream strs;
        strs << vcl_setprecision( 16 );
        strs << "large change in location ( from ";
        prev_loc.ascii_serialize(strs) << " to ";
        new_loc.ascii_serialize(strs) << ").\n";
        return checked_bool( strs.str() );
      }
    }

    prev_loc = new_loc;
  }

  return true;
}

}// end namespace vidtk
