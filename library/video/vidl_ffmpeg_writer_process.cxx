/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "vidl_ffmpeg_writer_process.h"
#include <vidl/vidl_frame.h>
#include <utilities/log.h>

namespace vidtk
{


vidl_ffmpeg_writer_process
::vidl_ffmpeg_writer_process( vcl_string const& name )
  : process( name, "vidl_ffmpeg_writer_process" )
{
  config_.add( "filename" );
  config_.add( "codec", "mpeg4" );
  config_.add( "bit_rate", "3000" );
  config_.add( "frame_rate", "30" );
}


vidl_ffmpeg_writer_process
::~vidl_ffmpeg_writer_process()
{
}


config_block
vidl_ffmpeg_writer_process
::params() const
{
  return config_;
}


bool
vidl_ffmpeg_writer_process
::set_params( config_block const& blk )
{
  typedef vidl_ffmpeg_ostream_params param_type;

  ostr_.set_filename( blk.get<vcl_string>( "filename" ) );

  vcl_string enc;
  blk.get( "codec", enc );
  if( enc == "mpeg4" )            param_.encoder_ = param_type::MPEG4;
  else if( enc == "msmpeg4v2" )   param_.encoder_ = param_type::MSMPEG4V2;
  else if( enc == "mpeg2video" )  param_.encoder_ = param_type::MPEG2VIDEO;
  else if( enc == "dvvideo" )     param_.encoder_ = param_type::DVVIDEO;
  else if( enc == "rawvideo" )    param_.encoder_ = param_type::RAWVIDEO;
  else if( enc == "huffyuv" )     param_.encoder_ = param_type::HUFFYUV;
  else
  {
    log_error( name() << ": unknown codec \"" << enc << "\"\n" );
    return false;
  }

  blk.get( "frame_rate", param_.frame_rate_ );
  blk.get( "bit_rate", param_.bit_rate_ );

  ostr_.set_params( param_ );

  config_.update( blk );
  return true;
}


bool
vidl_ffmpeg_writer_process
::initialize()
{
  // We delay the real initialization until we see the first frame
  // because we need to know the size of the image.

  return true;
}


bool
vidl_ffmpeg_writer_process
::step()
{
  if( ! src_frame_ )
  {
    return false;
  }

  return ostr_.write_frame( src_frame_ );
}


void
vidl_ffmpeg_writer_process
::set_frame( vil_image_view<vxl_byte> const& img )
{
  src_frame_ = new vidl_memory_chunk_frame( img );
  if( src_frame_->data() == NULL )
  {
    log_error( name() << ": couldn't form a output frame from input image\n" );
    src_frame_ = vidl_frame_sptr();
  }
}


} // end namespace vidtk
