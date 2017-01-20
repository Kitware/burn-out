/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "video_io/vidl_ffmpeg_metadata_frame_process.h"

#include <utilities/video_metadata.h>
#include <utilities/video_metadata_util.h>

#include <fstream>

#include <logger/logger.h>

VIDTK_LOGGER("vidl_ffmpeg_metadata_frame_process");


namespace vidtk
{

vidl_ffmpeg_metadata_frame_process
::vidl_ffmpeg_metadata_frame_process( std::string const& _name )
  : vidl_ffmpeg_frame_process( _name ),
    cur_idx_( -1 ),
    start_frame_( -1 )
{
}


config_block
vidl_ffmpeg_metadata_frame_process
::params() const
{
  config_block blk = vidl_ffmpeg_frame_process::params();

  blk.add_parameter( "metadata_filename", "UNDOCUMENTED" );

  return blk;
}


bool
vidl_ffmpeg_metadata_frame_process
::set_params( config_block const& blk )
{
  try
  {
    metadata_filename_ = blk.get<std::string>( "metadata_filename" );
    if( blk.has( "start_at_frame" ) )
    {
      start_frame_ = blk.get<int>( "start_at_frame" );
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  LOG_DEBUG( "vidl_ffmpeg_metadata_frame_process: metadata_filename = \"" << metadata_filename_ << "\"" );

  return vidl_ffmpeg_frame_process::set_params( blk );
}


bool
vidl_ffmpeg_metadata_frame_process
::initialize()
{
  bool good = true;

  if( start_frame_ != -1 )
  {
    cur_idx_ = start_frame_ - 1;
  }

  std::ifstream fin;
  fin.open( metadata_filename_.c_str() );

  std::string full_line;
  while ( std::getline(fin, full_line) )
  {
    std::stringstream sstr( full_line );
    unsigned long frame_number;
    vxl_uint_64 timecode;
    video_metadata md;
    vidtk::timestamp ts;

    sstr >> frame_number;
    // The metadata timestamp
    sstr >> timecode;
    md.timeUTC( timecode );
    // The frame timestamp
    sstr >> timecode;
    ascii_deserialize( sstr, md );

    metadata_.push_back( md );

    ts.set_frame_number( frame_number );
    ts.set_time( static_cast<double>( timecode ) );

    timestamps_.push_back( ts );
  }

  fin.close();

  return good && vidl_ffmpeg_frame_process::initialize();
}


bool
vidl_ffmpeg_metadata_frame_process
::step()
{
  ++cur_idx_;

  if ( (cur_idx_ >= 0) && (metadata_.size() <= static_cast<size_t>(cur_idx_) ))
  {
    return false;
  }

  return vidl_ffmpeg_frame_process::step();
}


timestamp
vidl_ffmpeg_metadata_frame_process
::timestamp() const
{
  vidtk::timestamp ts = vidl_ffmpeg_frame_process::timestamp();

  if ( !ts.has_time() && timestamps_[cur_idx_].has_time() )
  {
    return timestamps_[cur_idx_];
  }

  return ts;
}


video_metadata
vidl_ffmpeg_metadata_frame_process
::metadata() const
{
  return metadata_[cur_idx_];
}


bool
vidl_ffmpeg_metadata_frame_process
::seek( unsigned frame_number )
{
  if ( frame_number < metadata_.size() )
  {
    cur_idx_ = frame_number;

    return true && vidl_ffmpeg_frame_process::seek( frame_number );
  }

  return false;
}


} // end namespace vidtk
