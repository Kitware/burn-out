/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "downsampling_process.h"

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

#include <limits>
#include <sstream>


namespace vidtk
{

VIDTK_LOGGER( "downsampling_process" );

downsampling_process
::downsampling_process( std::string const& _name )
  : process( _name, "downsampling_process" ),
    disabled_( false ),
    dropped_frame_output_( process::NO_OUTPUT ),
    frame_downsampler_enabled_( false ),
    queue_monitor_enabled_( false ),
    max_allowed_queue_len_( 7 ),
    pipeline_to_monitor_( NULL )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Should we disable this process and just pass our inputs?" );
  config_.add_parameter(
    "queue_monitor_enabled",
    "false",
    "Should we enable the pipeline queue monitor? This will monitor "
    "the queue length of different pipeline nodes. If the queue exceeds "
    "the provided threshold, frames will be dropped." );
  config_.add_parameter(
    "nodes_to_monitor",
    _name,
    "A list of pipeline nodes to monitor, seperated by spaces." );
  config_.add_parameter(
    "max_pipeline_queue_length",
    "7",
    "Maximum allowed pipeline queue length before we begin dropping frames" );
  config_.add_parameter(
    "dropped_frame_signal",
    "no_output",
    "When a frame is dropped via this process, should a NO_OUTPUT or SKIP "
    "signal be sent from this pipeline?" );

  config_.merge( frame_downsampler_settings().config() );
}

downsampling_process::
~downsampling_process()
{}

vidtk::config_block
downsampling_process
::params() const
{
  return config_;
}


bool
downsampling_process
::initialize()
{
  return true;
}


std::vector< std::string >
parse_params_list( const std::string& str )
{
  std::vector< std::string > str_list;

  std::stringstream parser( str );
  std::string entry;

  while( parser >> entry )
  {
    if( entry.size() > 0 )
    {
      str_list.push_back( entry );
    }
    if( parser.peek() == ' ' || parser.peek() == ',' )
    {
      parser.ignore();
    }
  }

  return str_list;
}


bool
downsampling_process
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get< bool >( "disabled" );

    if( ! disabled_ )
    {
      queue_monitor_enabled_ = blk.get< bool >( "queue_monitor_enabled" );

      if( queue_monitor_enabled_ )
      {
        max_allowed_queue_len_ = blk.get< unsigned >( "max_pipeline_queue_length" );

        queue_monitor_.reset( new pipeline_queue_monitor );

        std::string str_list = blk.get< std::string >( "nodes_to_monitor" );
        nodes_to_monitor_ = parse_params_list( str_list );
      }

      frame_downsampler_settings fd_settings;
      fd_settings.read_config( blk );

      frame_downsampler_enabled_ |= fd_settings.rate_limiter_enabled;
      frame_downsampler_enabled_ |= fd_settings.adaptive_downsampler_enabled;

      if( frame_downsampler_enabled_ )
      {
        frame_downsampler_.reset( new frame_downsampler() );

        if( !frame_downsampler_->configure( fd_settings ) )
        {
          throw config_block_parse_error( "Unable to configure frame downsampler." );
        }
      }

      if( !queue_monitor_enabled_ && !frame_downsampler_enabled_ )
      {
        disabled_ = true;
      }
      else
      {
        std::string input = blk.get< std::string >( "dropped_frame_signal" );

        if( input == "no_output" )
        {
          dropped_frame_output_ = process::NO_OUTPUT;
        }
        else if( input == "skip" )
        {
          dropped_frame_output_ = process::SKIP;
        }
        else
        {
          throw config_block_parse_error( "Invalid frame signal string: " + input );
        }
      }
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
downsampling_process
::set_pipeline_to_monitor( const vidtk::pipeline* p )
{
  bool ret_val = true;

  pipeline_to_monitor_ = p;

  if( nodes_to_monitor_.size() > 0 )
  {
    LOG_ASSERT( queue_monitor_, "Queue monitor invalid" );

    queue_monitor_->reset();

    for( unsigned i = 0; i < nodes_to_monitor_.size(); i++ )
    {
      if( !queue_monitor_->monitor_node( nodes_to_monitor_[i], p ) )
      {
        LOG_ERROR( "Unable to find node: " << nodes_to_monitor_[i] );
        ret_val = false;
      }
    }
  }

  return ret_val;
}


process::step_status
downsampling_process
::step2()
{
  if( disabled_ )
  {
    LOG_TRACE( "frametrace: downsample: " << current_timestamp().frame_number() << " : downsampler-disabled" );
    return process::SUCCESS;
  }

  if( frame_downsampler_enabled_ )
  {
    if( ! current_timestamp().has_time() )
    {
      LOG_ERROR( "Timestamp must have valid time." );
      return process::FAILURE;
    }

    if( frame_downsampler_->drop_current_frame( this->current_timestamp() ) &&
        !this->is_frame_critical() )
    {
      LOG_TRACE( "frametrace: downsample: " << current_timestamp().frame_number() << " : dropped" );
      return dropped_frame_output_;
    }
  }

  if( queue_monitor_enabled_ &&
      queue_monitor_->current_max_queue_length() > max_allowed_queue_len_ )
  {
    LOG_TRACE( "frametrace: downsample: " << current_timestamp().frame_number() << " : queue-overflow-dopped" );
    return dropped_frame_output_;
  }

  LOG_TRACE( "frametrace: downsample: " << current_timestamp().frame_number() << " : passed" );
  return process::SUCCESS;
}


vidtk::timestamp
downsampling_process
::current_timestamp()
{
  LOG_WARN( "Timestamp function accessor invalid." );
  return vidtk::timestamp();
}


bool
downsampling_process
::is_frame_critical()
{
  return false;
}


void
downsampling_process
::set_frame_rate( const double frame_rate )
{
  if( frame_downsampler_ )
  {
    frame_downsampler_->set_frame_rate( frame_rate );
  }
}


void
downsampling_process
::set_frame_count( const unsigned frame_count )
{
  if( frame_downsampler_ )
  {
    frame_downsampler_->set_frame_count( frame_count );
  }
}


double
downsampling_process
::output_frame_rate() const
{
  return ( frame_downsampler_ ? frame_downsampler_->output_frame_rate() : 0 );
}


unsigned
downsampling_process
::output_frame_count() const
{
  return ( frame_downsampler_ ? frame_downsampler_->output_frame_count() : 0 );
}


bool
downsampling_process
::in_burst_break() const
{
  return frame_downsampler_ && frame_downsampler_->in_burst_break();
}

}
