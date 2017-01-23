/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "timestamp_generator_process.h"

#include <limits>
#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "timestamp_generator_process" );

timestamp_generator_process
::timestamp_generator_process( std::string const& _name )
  : process( _name, "timestamp_generator_process" )
{
}


timestamp_generator_process
::~timestamp_generator_process()
{
}


config_block
timestamp_generator_process
::params() const
{
  return generator_.params();
}


bool
timestamp_generator_process
::set_params( config_block const& blk )
{
  try
  {
    generator_.set_params( blk );
  }
  catch( const config_block_parse_error & e )
  {
    LOG_ERROR( name() << ": " << e.what() );
    return false;
  }

  return true;
}


bool
timestamp_generator_process
::initialize()
{
  return generator_.initialize();
}


process::step_status
timestamp_generator_process
::step2()
{
  ts_ = timestamp(); // reset output

  return generator_.provide_timestamp( ts_ );
}


// ----------------------------------------------------------------
// -- Outputs --

vidtk::timestamp
timestamp_generator_process
::timestamp() const
{
  return ts_;
}


} // end namespace vidtk
