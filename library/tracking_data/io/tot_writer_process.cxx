/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tot_writer_process.h"

#include <vul/vul_file.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_tot_writer_process_cxx__
VIDTK_LOGGER("tot_writer_process_cxx");


namespace vidtk
{
tot_writer_process
::tot_writer_process( std::string const& _name )
  : process( _name, "tot_writer_process" ),
    disabled_( true ),
    allow_overwrite_( false ),
    src_trks_set_( false )

{
  config_.add_parameter( "disabled",
                         "true",
                         "Disables the writing process" );
  config_.add_parameter( "filename",
                         "",
                         "The output file.  For format shape, this is treated as a directory" );
  config_.add_parameter( "overwrite_existing",
                         "true",
                         "Whether or not a file can be overwriten" );
}


tot_writer_process
::~tot_writer_process()
{
  if( writer_.is_open() )
  {
    writer_.close();
  }
}


config_block
tot_writer_process
::params() const
{
  return config_;
}

bool
tot_writer_process
::set_params( config_block const& blk )
{
  try
  {
    this->disabled_ = blk.get<bool>( "disabled" );

    if( !this->disabled_ )
    {
      this->filename_ = blk.get<std::string>( "filename" );
      this->allow_overwrite_ = blk.get<bool>( "overwrite_existing" );
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
tot_writer_process
::initialize()
{
  if( disabled_ )
  {
    return true;
  }

  if( fail_if_exists( filename_ ) )
  {
    LOG_ERROR( name() << " File: " << filename_
      << " already exists. To overwrite set overwrite_existing flag." );
    return false;
  }

  if( !writer_.open( filename_ ) )
  {
    LOG_ERROR(name() << "Failed to open/find directory");
    return false;
  }

  return true;
}

void
tot_writer_process
::clear()
{
  src_trks_.clear();
  src_trks_set_ = false;
}


process::step_status
tot_writer_process
::step2()
{
  if( disabled_ )
  {
    //When writer is disabled, async pipelines buffers fill,  This forces them to clear out.
    clear();

    if( !src_trks_set_ )
    {
      return process::FAILURE;
    }
    else
    {
      return process::SUCCESS;
    }
  }

  if( !src_trks_set_ )
  {
    clear();
    LOG_INFO( name() << ": source tracks not specified." );
    return process::FAILURE;
  }

  bool result = writer_.write( src_trks_ );

  clear();

  if( result )
  {
    return process::SUCCESS;
  }

  return process::FAILURE;
}


void
tot_writer_process
::set_tracks( std::vector< track_sptr > const& trks )
{
  src_trks_ = trks;
  src_trks_set_ = true;
}


bool
tot_writer_process
::is_disabled() const
{
  return disabled_;
}


/// \internal
///
/// Returns \c true if the file \a filename exists and we are not
/// allowed to overwrite it (governed by allow_overwrite_).
bool
tot_writer_process
::fail_if_exists( std::string const& filename )
{
  return ( !allow_overwrite_ ) && vul_file::exists( filename );
}

} // end namespace vidtk
