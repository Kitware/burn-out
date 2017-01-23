/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "dir_string_reader_impl.h"

#include <vul/vul_file_iterator.h>
#include <vul/vul_file.h>

#include <vpl/vpl.h>

#include <algorithm>

#include <boost/filesystem.hpp>

#ifdef USE_ANGEL_FIRE
#include <AFFileReader.h>
#endif

#include <logger/logger.h>
VIDTK_LOGGER("dir_string_reader_impl");


namespace vidtk {
namespace string_reader_impl {

// ----------------------------------------------------------------
dir_string_reader_impl
::dir_string_reader_impl( std::string const& _name )
  : string_reader_impl_base( _name )
{
  this->config_.add_parameter( "glob",
                "The set of images provided in glob form. e.g. images/image*.png" );
  this->config_.add_parameter( "live_monitor_mode", "false",
                "Boolean, set true if monitoring a directory. "
                "Currently used only for angel_fire.");
  this->config_.add_parameter( "buffer_size", "10", "Int, the size of the frame buffer.");
  this->config_.add_parameter( "live_monitor_timeout", "60",
                "Long, the number of seconds to wait for a timeout in live monitor mode.");
}


// ----------------------------------------------------------------
dir_string_reader_impl
::~dir_string_reader_impl( void )
{
}


// ----------------------------------------------------------------
bool dir_string_reader_impl
::set_params( config_block const& blk )
{
  this->glob_pattern = blk.get<std::string>( "glob" );
  this->monitor_mode = blk.get<bool> ( "live_monitor_mode");
  if( monitor_mode )
  {
    this->buffer_size = blk.get<unsigned int>( "buffer_size" );
    this->timeout = blk.get<long>( "live_monitor_timeout" );
  }

  // using a hack to give big images time to copy
  this->curtime = boost::posix_time::second_clock::universal_time() - boost::posix_time::seconds(5);
  this->lastfile = this->curtime;

  try
  {
    for( vul_file_iterator fn = glob_pattern; fn; ++fn )
    {
      char* filename = static_cast<char*> (malloc(strlen(fn()) + 1));
      strcpy(filename, fn());
      this->lastfile = boost::posix_time::second_clock::universal_time();
      filename_list.push_back( std::make_pair(0,filename) );

#ifdef USE_ANGEL_FIRE
      // if we're in monitor mode, get the time for sort
      if(monitor_mode)
      {
        this->all_files.insert(filename);
        filename_list.back().first = AFFile::readTime(filename);
      }
#endif
      free(filename);
    }

    std::sort( filename_list.begin(), filename_list.end() );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }

  return true;
 }


// ----------------------------------------------------------------
bool dir_string_reader_impl
::initialize()
{
  if( filename_list.empty() )
  {
    LOG_WARN( this->name() << ": no matching files in list" );
  }

  return true;
}


// ----------------------------------------------------------------
bool dir_string_reader_impl
::step()
{

// monitor mode doesn't work without a time sort, and right now we only have one set up
// for AngelFire. If we add other file types in the future, we can cut the preprocessor
// down to only encompass a couple of lines. This prevents mistakes in the meantime.
#ifdef USE_ANGEL_FIRE
  if(monitor_mode) // if we're using buffered mode, we have to check for new files
  {

    // loop until we either fill our buffer or we reach our timeout
    while( boost::posix_time::time_duration(this->curtime - this->lastfile).total_seconds() < this->timeout)
    {

      // using a hack to give big images time to copy
      this->curtime = boost::posix_time::second_clock::universal_time() - boost::posix_time::seconds(5);

      // traverse the directory, looking for new files
      for( vul_file_iterator fn = glob_pattern; fn; ++fn )
      {
        char* filename = static_cast<char*> (malloc(strlen(fn()) + 1));
        strcpy(filename, fn());
        boost::posix_time::ptime filetime = boost::posix_time::from_time_t(
                                            boost::filesystem::last_write_time( filename ) );

        if( this->all_files.find(filename) == this->all_files.end() ) // New file. Update timeout and file lists.
        {
          this->lastfile = boost::posix_time::second_clock::universal_time();
          filename_list.push_back( std::make_pair(AFFile::readTime(filename), filename) );
          this->all_files.insert(filename);
        }
        free(filename);
      }

      vpl_sleep(1);

      // we can stop looping if our buffer is full
      if (filename_list.size() > this->buffer_size)
      {
        break;
      }

    } // end while
  } // end if monitor_mode
#endif

  // if our buffer is empty, we can stop
  if (filename_list.empty())
  {
    return false;
  }

  // we've had a timeout, filled a buffer, or are not in monitor mode, so output our first file
  std::sort( filename_list.begin(), filename_list.end() );
  str_ = filename_list.front().second;
  filename_list.pop_front();
  return true;
}

} // end namespace string_reader_impl
} // end namespace vidtk
