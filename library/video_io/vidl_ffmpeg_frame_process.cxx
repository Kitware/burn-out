/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <algorithm>
#include <limits>

#include <video_io/vidl_ffmpeg_frame_process.h>

#include <vidl/vidl_convert.h>
#include <vidl/vidl_config.h>
#include <vil/vil_crop.h>
#include <logger/logger.h>

#include <klv/klv_key.h>
#include <klv/klv_0601.h>
#include <klv/klv_0601_traits.h>
#include <klv/klv_0104.h>
#include <klv/klv_parse.h>

#include <utilities/klv_to_metadata.h>

#include <boost/thread/locks.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread/thread.hpp>

using boost::int64_t;

namespace vidtk
{
VIDTK_LOGGER("vidl_ffmpeg_frame_process_cxx");
boost::mutex vidl_ffmpeg_frame_process::m_open_;

vidl_ffmpeg_frame_process
::vidl_ffmpeg_frame_process( std::string const& _name )
  : frame_process<vxl_byte>( _name, "vidl_ffmpeg_frame_process" ),
    read_ahead_( false ),
    ni_( 0 ),
    nj_( 0 ),
    nframes_( 0 ),
    start_frame_( -1 ),
    stop_after_frame_( -1 ),
    n_blank_frames_after_eoi_( -1 ),
    blank_frame_mode_(false),
    meta_ts_(0.0),
    ts_(0.0),
    klv_type_("none"),
    simulate_stream_(false),
    ts_scaling_factor_(1.0)
{
}


config_block
vidl_ffmpeg_frame_process
::params() const
{
  config_block blk;
  std::stringstream uintmax;
  uintmax << std::numeric_limits<unsigned int>::max();

  blk.add_parameter( "filename", "Name of video file" );
  blk.add_parameter( "start_at_frame", "-1", "-1 to start at beginning of video" );
  blk.add_parameter( "stop_after_frame", uintmax.str(), "Index of the last frame to read" );
  blk.add_parameter( "n_blank_frames_after_eoi", "0", "Number of blank frames after input to terminate active tracks" );
  blk.add_parameter( "klv_type", "none", "Type of klv metadata to use: [0601, 0104, none]" );
  blk.add_parameter( "time_type", "pts", "Source of timestamp to use: [misp, klv, pts]" );
  blk.add_parameter( "simulate_stream", "false", "Simulate a streaming source" );
  blk.add_parameter("ts_scaling_factor", "1.0", "Multiply each of the timestamp by this factor."
    " This is useful as the timestamps encoded in some videos (with MISP sources notably) might not be"
    " in the correct unit. In this case, the ts_scaling_factor can be used to scale the time to the"
    " correct unit.");

  return blk;
}


bool
vidl_ffmpeg_frame_process
::set_params( config_block const& blk )
{
  try
  {
    filename_ = blk.get<std::string>( "filename" );
    if( blk.has( "start_at_frame" ) )
    {
      // need to allow "-1" as a special parameter to avoid seeks to frame 0
      // which seem to disturb the metadata parsing (see initialize() below),
      // but some platforms throw exceptions when trying to parse "-1" as an
      // unsigned int
      start_frame_ =
        ( blk.get< std::string >( "start_at_frame" ) == "-1" )
        ? static_cast< unsigned >( -1 )
        : blk.get< unsigned >( "start_at_frame" );
    }

    if( blk.has( "stop_after_frame" ) )
    {
      stop_after_frame_ = blk.get<unsigned>( "stop_after_frame" );
    }

    if( blk.has( "n_blank_frames_after_eoi" ) )
    {
      n_blank_frames_after_eoi_ = blk.get<int>( "n_blank_frames_after_eoi" );
    }

    if (blk.has( "klv_type" ) )
    {
      klv_type_ = blk.get<std::string>( "klv_type" );
    }

    if (blk.has( "time_type" ) )
    {
      time_type_ = blk.get<std::string>( "time_type" );
    }

    if (blk.has( "simulate_stream" ) )
    {
      simulate_stream_ = blk.get<bool>( "simulate_stream" );
    }

    if (blk.has("ts_scaling_factor"))
    {
      ts_scaling_factor_ = blk.get<double>("ts_scaling_factor");
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  if (klv_type_ != "none" && klv_type_ != "0104" && klv_type_ != "0601")
  {
    LOG_ERROR(this->name() << " klv_type: " << klv_type_ << " is not 0104, 0601, or none.");
    return false;
  }

  if (time_type_ != "pts" && time_type_ != "klv" && time_type_ != "misp")
  {
    LOG_ERROR(this->name() << " time_type: " << time_type_ << " is not misp, klv, or pts.");
    return false;
  }

  if (klv_type_ == "none" && time_type_ == "klv")
  {
    LOG_ERROR(this->name() << " klv_type is none yet time_type is set to klv.");
    return false;
  }

  return true;
}


bool
vidl_ffmpeg_frame_process
::initialize()
{
#if ! VIDL_HAS_FFMPEG
  // An error message to make it easier to diagnose ffmpeg-related
  // failures.
  LOG_ERROR( "vxl's vidl doesn't have ffmpeg compiled in!" );
  return false;
#endif

  read_ahead_ = true;

  // If the open succeeds, it will already have read the first frame.
  // avcodec_open2 which is called by open is not thread safe so we need to lock
  {
    boost::lock_guard<boost::mutex> lock(m_open_);
    if( ! istr_.open( filename_ ) )
    {
      LOG_ERROR(this->name() << " failed to open video: " << filename_);
      return false;
    }
  }

  if( !istr_.has_metadata() )
  {
    if (klv_type_ != "none")
    {
      LOG_WARN(this->name() << " requested klv metadata but no metadata is in the video.");
      klv_type_ = std::string("none");
    }
  }

  nframes_ = static_cast< unsigned >( istr_.num_frames() );
  frame_rate_ = static_cast< double >( istr_.frame_rate() );

  metadata_.is_valid(false);

  //Since the metadata and the frames are not exactly synchronized we cannot be sure
  //exactly which frame the metadata packet really belongs to.  If we always take the
  //newest metadata time then we can have issues of the time going backwards when it resets to
  //the metadata time.  Therefore we use the first metadata packet to set the time and
  //use the pts to update that.  This does not work if the video pauses recording then
  //resumes later.

  if(!init_timestamp())  //will call advance()
  {
    LOG_ERROR(this->name() << " failed to initialize the timestamp for: " << filename_);
    return false;
  }

  ni_ = istr_.current_frame()->ni();
  nj_ = istr_.current_frame()->nj();

  //Seeking to frame 0 here seems to delay the metadata
  if( start_frame_ != unsigned(-1) && start_frame_ > 0)
  {
    istr_.seek_frame( start_frame_ );
  }

  return true;
}


bool
vidl_ffmpeg_frame_process
::step()
{
  LOG_ASSERT( istr_.is_open(), "Input video stream is not open" );

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

        LOG_ERROR( "Advance failed; returned blank frame " << (n_blank_frames_after_eoi_+1));

        img_ = vil_image_view<vxl_byte>( ni_, nj_, 3 );
        img_.fill(0);

        return true;
      }
      else
      {
        return false;
      }
    }
  }

  unsigned int frame_num = istr_.frame_number();
  if( stop_after_frame_ < frame_num )
  {
    return false;
  }

  // We succeed in the step if we can convert the frame to RGB.
  bool result = vidl_convert_to_view( *istr_.current_frame(),
                                      img_,
                                      VIDL_PIXEL_COLOR_RGB );


  // Read klv metadata
  if ( klv_type_ != "none" )
  {
    std::deque<vxl_byte> curr_md = istr_.current_metadata();
    klv_data klv_packet;
    metadata_.is_valid(false);
    while (klv_pop_next_packet( curr_md, klv_packet ))
    {
      if ( read_klv(klv_packet) )
      {
        break;
      }
    }
  }

  // Metadata packets may not exist for each frame, so use the diff in
  // presentation time stamps to foward the first metadata time stamp.
  double pts_diff = (istr_.current_pts() - pts_of_meta_ts_)*1e6;
  ts_.set_time(meta_ts_.time() + pts_diff);

  if( has_roi_ )
  {
    img_ = vil_crop(img_, roi_x_, roi_width_, roi_y_, roi_height_);
  }

  if( simulate_stream_ )
  {
    if( !start_source_time_.has_time() )
    {
      start_source_time_ = ts_;
      start_local_time_ = boost::posix_time::microsec_clock::local_time();
    }
    else
    {
      boost::posix_time::ptime current_time = boost::posix_time::microsec_clock::local_time();
      boost::posix_time::time_duration diff = current_time - start_local_time_;

      double local_elapsed_ms = static_cast<double>( diff.total_milliseconds() );
      double source_elapsed_ms = ( ts_.time() - start_source_time_.time() ) / 1000;

      if( local_elapsed_ms < source_elapsed_ms )
      {
        local_elapsed_ms = ( source_elapsed_ms - local_elapsed_ms );
        boost::this_thread::sleep( boost::posix_time::milliseconds( local_elapsed_ms ) );
      }
    }
  }

  // Hack for bad data with bad timestamps
  while( result && ts_.time() < 0 )
  {
    result = this->step();
  }

  return result;
}

timestamp
vidl_ffmpeg_frame_process
::timestamp() const
{
  vidtk::timestamp ts = ts_;
  if ( blank_frame_mode_ )
  {
    last_frame_++;
  }
  else
  {
    last_frame_ = istr_.frame_number();
  }

  ts.set_frame_number( last_frame_ );
  return ts;
}


vil_image_view<vxl_byte>
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

bool
vidl_ffmpeg_frame_process::read_klv( const klv_data &klv)
{
  bool good = false;
  klv_uds_key uds_key( klv ); // create key from raw data

  if (klv_type_ == std::string("0601") && is_klv_0601_key(uds_key))
  {
    good = klv_0601_to_metadata(klv, metadata_);
  }
  else if (klv_type_ == std::string("0104") && klv_0104::is_key(uds_key))
  {
    good = klv_0104_to_metadata(klv, metadata_);
  }

  if (good)
  {
    double time_diff = fabs(static_cast<double>(metadata_.timeUTC()) - ts_.time());
    //warn if meta time and ts time differ by more than 10 seconds
    if (time_diff > 1e7)
    {
      LOG_WARN("Metadata timestamp large sync difference: " << time_diff / 1e6 << " seconds.");
    }
  }

  return good;
}



//Extract the time stamp from the buffer
bool convertMISPmicrosectime(const std::vector<unsigned char> &buf, int64_t &ts)
{
  enum MISP_time_code { MISPmicrosectime = 0,
                        FLAG = 16,
                        MSB_0,
                        MSB_1,
                        IGNORE_0,
                        MSB_2,
                        MSB_3,
                        IGNORE_1,
                        MSB_4,
                        MSB_5,
                        IGNORE_2,
                        MSB_6,
                        MSB_7,
                        IGNORE_3,
                        MISP_NUM_ELEMENTS
                      };

  //Check that the tag is the first thing in buf
  std::string misp("MISPmicrosectime");
  for (size_t i = 0; i < FLAG; i++)
  {
    if (buf[i] != misp[i])
      return false;
  }

  if (buf.size() >= MISP_NUM_ELEMENTS)
  {
    ts = 0;

    ts |= static_cast<int64_t>(buf[MSB_7]);
    ts |= static_cast<int64_t>(buf[MSB_6]) << 8;
    ts |= static_cast<int64_t>(buf[MSB_5]) << 16;
    ts |= static_cast<int64_t>(buf[MSB_4]) << 24;

    ts |= static_cast<int64_t>(buf[MSB_3]) << 32;
    ts |= static_cast<int64_t>(buf[MSB_2]) << 40;
    ts |= static_cast<int64_t>(buf[MSB_1]) << 48;
    ts |= static_cast<int64_t>(buf[MSB_0]) << 56;

    return true;
  }

  return false;
}

/// Find the first valid timestamp in the video if availible and cache it for updating
bool
vidl_ffmpeg_frame_process::
init_timestamp()
{
  meta_ts_.set_time(0.0);
  if(!istr_.advance())
  {
    return false;
  }

  if (time_type_ == "pts")
  {
    LOG_DEBUG(this->name() << " using pts for timestamp: " << istr_.current_pts() );
    pts_of_meta_ts_ = istr_.current_pts();
    ts_.set_time(meta_ts_.time());
    return true;
  }

  do
  {
    //Check for frame embeded time stamps, prefer this over klv time stamp since it is attached to the
    //frame itself and not in a separate stream.  We will still treat this timestamp the same as
    //klv by finding one then modifying it with pts.

    if (time_type_ == "misp")
    {
      std::vector<unsigned char> pkt_data = istr_.current_packet_data();
      std::string misp("MISPmicrosectime");

      //Check if the data packet has enough bytes for the MISPmicrosectime packet
      if (pkt_data.size() < misp.length() + 13)
      {
        continue;
      }

      bool found;
      size_t ts_location = std::string::npos;
      size_t last = pkt_data.size() - misp.size();
      for (size_t i = 0; i <= last; i++)
      {
        found = true;
        for (size_t j = 0; j < misp.size(); j++)
        {
          if (pkt_data[i+j] != misp[j])
          {
            found = false;
            break;
          }
        }

        if (found)
        {
          ts_location = i;
          break;
        }
      }

      if ((std::string::npos != ts_location) && ((ts_location + misp.length() + 13) < pkt_data.size()))
      {
        std::vector<unsigned char> MISPtime_buf(pkt_data.begin() + ts_location, pkt_data.begin() + ts_location + misp.length() + 13);
        int64_t ts = 0;
        convertMISPmicrosectime(MISPtime_buf, ts);
        meta_ts_.set_time(static_cast<double>(ts) * ts_scaling_factor_);
        LOG_DEBUG(this->name() << " found MISP timestamp:" << meta_ts_ );
      }
    }
    else if (time_type_ == "klv" && istr_.has_metadata() && klv_type_ != "none")
    {
      if (istr_.current_metadata().empty())
      {
        continue;
      }

      //It might be more accurate to get the second unique timestamp instead of the first
      std::deque<vxl_byte> curr_md = istr_.current_metadata();

      klv_data klv_packet;
      while (klv_pop_next_packet( curr_md, klv_packet ))
      {
        klv_uds_key uds_key( klv_packet );
        if (klv_type_ == std::string("0601") && is_klv_0601_key(uds_key) && klv_0601_checksum(klv_packet))
        {
          klv_lds_vector_t lds = parse_klv_lds( klv_packet );
          for ( klv_lds_vector_t::const_iterator itr = lds.begin(); itr != lds.end(); itr++ )
          {
            const klv_0601_tag tag (static_cast< klv_0601_tag > ( vxl_byte( itr->first ) ));
            if (tag == KLV_0601_UNIX_TIMESTAMP)
            {
              boost::any data = klv_0601_value( tag, &itr->second[0], itr->second.size() );
              meta_ts_.set_time(
                static_cast<double>(boost::any_cast<klv_0601_traits<KLV_0601_UNIX_TIMESTAMP>::type>(data))
                * ts_scaling_factor_);
              LOG_DEBUG(this->name() << " found initial klv 0601 timestamp: " << meta_ts_);
            }
          }
        }
        else if (klv_type_ == std::string("0104") && klv_0104::is_key(uds_key))
        {
          klv_uds_vector_t uds = parse_klv_uds( klv_packet );

          for ( klv_uds_vector_t::const_iterator itr = uds.begin(); itr != uds.end(); itr++ )
          {
            const klv_0104::tag tag = klv_0104::inst()->get_tag(itr->first);
            if (tag == klv_0104::UNIX_TIMESTAMP)
            {
              boost::any data = klv_0104::inst()->get_value(tag, &itr->second[0], itr->second.size());
              meta_ts_.set_time(
                static_cast<double>(klv_0104::inst()->get_value<vxl_uint_64>(tag, data))
                * ts_scaling_factor_);
              LOG_DEBUG(this->name() << " found initial klv 0104 timestamp: " << meta_ts_);
            }
          }
        }
      }
    }
  }
  while (meta_ts_.time() == 0.0 && istr_.advance());

  if (meta_ts_.time() != 0.0)
  {
    pts_of_meta_ts_ = istr_.current_pts();
    ts_.set_time(meta_ts_.time());
  }

  //Tried seeking to the beginning but some videos don't
  //want to seek, even to the start.  So reload the video.
  {
    boost::lock_guard<boost::mutex> lock(m_open_);
    istr_.open( filename_); //Calls close on current video
  }

  if(!istr_.advance())
  {
    return false;
  }

  //If we did not find a metadata timestamp then we use pts for time
  if (meta_ts_.time() == 0.0)
  {
    LOG_WARN(this->name() << " Did not find " << time_type_ << " time stamp, using pts.");
    pts_of_meta_ts_ = istr_.current_pts();
    ts_.set_time(0.0);
  }
  return true;
}

} // end namespace vidtk
