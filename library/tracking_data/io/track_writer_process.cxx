/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_writer_process.h"

#include <vul/vul_file.h>

#include <utilities/timestamp.h>
#include <plugin_loader/plugin_config_util.h>

#include <logger/logger.h>
#include <sstream>

VIDTK_LOGGER("track_writer_process");


namespace vidtk
{

track_writer_process
::track_writer_process( std::string const& _name )
  : process( _name, "track_writer_process" ),
    disabled_( true ),
    allow_overwrite_( false ),
    src_objs_( NULL ),
    src_trks_( NULL )
{
  std::stringstream format_help;
  format_help << "The format of the output (one of:\n "
              << writer_.get_writer_formats()
              << ')';

  config_.add_parameter( "disabled", "true", "Disables the writing process" );
  config_.add_parameter( "format", "columns_tracks", format_help.str() );
  config_.add_parameter( "filename", "", "The output file.  For format shape, this is treated as a directory. \n"
                         "For postegresql, this is used as the database name." );
  config_.add_parameter( "overwrite_existing", "false",
                         "Whether or not a file can be overwritten" );
  config_.add_parameter( "suppress_header", "false",
                         "Whether or not to write the header" );
  config_.add_parameter( "path_to_images","",
                         "Path to the images (used for writing mit and 4676 files)");
  config_.add_parameter( "frequency","0",
                         "The number of frames per second");
  config_.add_parameter( "x_offset", "0", "UNDOCUMENTED");
  config_.add_parameter( "y_offset", "0", "UNDOCUMENTED");
  config_.add_parameter( "write_lat_lon_for_world", "true",
                         "Write lat/lon values in the world coordinate fields");
  config_.add_parameter( "write_utm", "false",
                         "Write UTM values in the loc and vel fields in kw18");

  // Merge in the parameters from the loaded track writers.
  config_block const& writer_block = writer_.get_writer_config();
  config_.merge( writer_block );
}


track_writer_process
::~track_writer_process()
{
  if(writer_.is_open())
  {
    writer_.close();
  }
}


config_block
track_writer_process
::params() const
{
  return config_;
}

bool
track_writer_process
::set_params( config_block const& blk )
{
  try
  {
    this->disabled_ = blk.get<bool>( "disabled" );
    if(!this->disabled_)
    {
      std::string fmt_str = blk.get<std::string>( "format" );
      this->filename_ = blk.get<std::string>( "filename" );
      track_writer_options options;

#define SET_OPTIONS(N,T,I) if ( blk.has( #N )) { options.N ## _ = blk.get<T>( # N); }

      TRACK_WRITER_OPTION_LIST(SET_OPTIONS)

#undef SET_OPTIONS

      this->allow_overwrite_ = blk.get<bool>( "overwrite_existing" );

      if(!writer_.set_format(fmt_str))
      {
        throw config_block_parse_error(": unknown format");
      }
      writer_.set_options(options);
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
track_writer_process
::initialize()
{
  if( disabled_ )
  {
    return true;
  }

  if( fail_if_exists( filename_ ) )
  {
    LOG_ERROR( name() << ": File: "
                 << filename_ << " already exists. To overwrite set overwrite_existing flag" );
    return false;
  }

  if(!writer_.open(filename_))
  {
    LOG_ERROR(name() << ": Failed to open/find directory");
    return false;
  }
  return true;
}

void
track_writer_process
::clear()
{
  src_trks_ = NULL;
  src_objs_ = NULL;
  ts_ = timestamp();
}


process::step_status
track_writer_process
::step2()
{
  if( disabled_ )
  {
    //When writer is disabled, async pipelines buffers fill,  This forces them to clear out.
    if(src_trks_ == NULL)
    {
      clear();
      return process::FAILURE;
    }
    else
    {
      clear();
      return process::SUCCESS;
    }
  }

  if( src_trks_ == NULL )
  {
    clear();
    LOG_INFO( name() << ": source tracks not specified.");
    return process::FAILURE;
  }

  bool result =  writer_.write(*src_trks_, src_objs_, ts_);

  clear();

  if(result)
  {
    return process::SUCCESS;
  }

  return process::FAILURE;
}


void
track_writer_process
::set_image_objects( std::vector< image_object_sptr > const& objs )
{
  src_objs_ = &objs;
}


void
track_writer_process
::set_tracks( std::vector< track_sptr > const& trks )
{
  src_trks_ = &trks;
}


void
track_writer_process
::set_timestamp( timestamp const& ts )
{
  ts_ = ts;
}


bool
track_writer_process
::is_disabled() const
{
  return disabled_;
}


/// \internal
///
/// Returns \c true if the file \a filename exists and we are not
/// allowed to overwrite it (governed by allow_overwrite_).
bool
track_writer_process
::fail_if_exists( std::string const& filename )
{
  return (!allow_overwrite_) && vul_file::exists(filename);
}

} // end namespace vidtk
