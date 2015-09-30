/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video/vidl_ffmpeg_frame_process.h>

#include <vidl/vidl_convert.h>
#include <vidl/vidl_config.h>

#include <vil/vil_crop.h>

#include <utilities/log.h>

namespace vidtk
{

vidl_ffmpeg_frame_process
::vidl_ffmpeg_frame_process( vcl_string const& name )
  : frame_process<vxl_byte>( name, "vidl_ffmpeg_frame_process" ),
    read_ahead_( false ),
    ni_( 0 ),
    nj_( 0 ),
    start_frame_( -1 ),
    stop_after_frame_( -1 ),
    n_blank_frames_after_eoi_( -1 ),
    blank_frame_mode_(false)
{
}


config_block
vidl_ffmpeg_frame_process
::params() const
{
  config_block blk;

  blk.add( "filename" );
  blk.add( "start_at_frame" );
  blk.add( "stop_after_frame" );
  blk.add( "n_blank_frames_after_eoi" );

  return blk;
}


bool
vidl_ffmpeg_frame_process
::set_params( config_block const& blk )
{
  bool good = true;

  good = good && blk.get( "filename", filename_ );
  if( blk.has( "start_at_frame" ) )
  {
    good = good && blk.get( "start_at_frame", start_frame_ );
  }

  if( blk.has( "stop_after_frame" ) )
  {
    good = good && blk.get( "stop_after_frame", stop_after_frame_ );
  }

  if( blk.has( "n_blank_frames_after_eoi" ) )
  {
    good = good && blk.get( "n_blank_frames_after_eoi", n_blank_frames_after_eoi_ );
  }

  log_debug( "vidl_ffmpeg_frame_process: filename = \"" << filename_ << "\"\n" );

  return good;
}


bool
vidl_ffmpeg_frame_process
::initialize()
{
#if ! VIDL_HAS_FFMPEG
  // An error message to make it easier to diagnose ffmpeg-related
  // failures.
  log_error( "vxl's vidl doesn't have ffmpeg compiled in!\n" );
#endif

  // If the open succeeds, it will already have read the first frame.
  read_ahead_ = true;
  if( ! istr_.open( filename_ ) )
  {
    return false;
  }

  // Newer versions of ffmpeg automatically read the first frame
  #if LIBAVFORMAT_BUILD < ((52<<16)+(2<<8)+0)  // before ver 52.2.0
  istr_.advance();
  #endif

  ni_ = istr_.current_frame()->ni();
  nj_ = istr_.current_frame()->nj();

  if( start_frame_ != unsigned(-1) )
  {
    istr_.seek_frame( start_frame_ );
  }

  return true;
}


bool
vidl_ffmpeg_frame_process
::step()
{
  log_assert( istr_.is_open(), "Input video stream is not open" );

  // First, try and read a frame.  We might not need to, if we already
  // have a frame in hand.

  if( read_ahead_ )
  {
    read_ahead_ = false;
  }
  else
  {
    if( ! istr_.advance() )
    {
      // optionally return blank frames after input, to hack a termination of active tracks
      if ( --n_blank_frames_after_eoi_ >= 0 )
      {
        if ( ! blank_frame_mode_ )
        {
          blank_frame_mode_ = true;
        }
        vcl_cerr << "Advance failed; returned blank frame " << (n_blank_frames_after_eoi_+1) << "\n";
        img_ = vil_image_view<vxl_byte>( ni_, nj_, 3 );
        for (unsigned i=0; i<ni_; ++i)
        {
          for (unsigned j=0; j<nj_; ++j)
          {
            img_(i,j,0) = 0;
            img_(i,j,1) = 0;
            img_(i,j,2) = 0;
          }
        }
        return true;
      }
      else
      {
        return false;
      }
    }
  }

  if( stop_after_frame_ < istr_.frame_number() )
  {
    return false;
  }

  // We succeed in the step if we can convert the frame to RGB.
  bool result = vidl_convert_to_view( *istr_.current_frame(),
                                      img_,
                                      VIDL_PIXEL_COLOR_RGB );
  if(has_roi_)
  {
    img_ = vil_crop(img_, roi_x_, roi_width_, roi_y_, roi_height_);
  }
  return result;
}

timestamp
vidl_ffmpeg_frame_process
::timestamp() const
{
  vidtk::timestamp ts;
  if ( blank_frame_mode_ )
  {
    ts.set_frame_number( last_timestamp_.frame_number() + 1 );
  }
  else
  {
    ts.set_frame_number( istr_.frame_number() );
  }
  last_timestamp_ = ts;
  return ts;
}


vil_image_view<vxl_byte> const&
vidl_ffmpeg_frame_process
::image() const
{
  return img_;
}


bool
vidl_ffmpeg_frame_process
::seek( unsigned frame_number )
{
  if( istr_.seek_frame( frame_number ) )
  {
    read_ahead_ = true;
    return true;
  }
  else
  {
    return false;
  }
}


} // end namespace vidtk
