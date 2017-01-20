/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vidl_ffmpeg_frame_process_h_
#define vidtk_vidl_ffmpeg_frame_process_h_

#include <video_io/frame_process.h>
#include <vidl/vidl_ffmpeg_istream.h>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <klv/klv_data.h>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Video source from ffmpeg.
 *
 * This class represnets a video source as read from ffmpeg.  Any
 * source supported by ffmpeg can be used. The metadata is supplied
 * from the embedded KLV data stream in the video source (if available).
 */
class vidl_ffmpeg_frame_process
  : public frame_process<vxl_byte>
{
public:
  vidl_ffmpeg_frame_process( std::string const& name );
  virtual ~vidl_ffmpeg_frame_process() { }

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  virtual bool seek( unsigned frame_number );
  virtual unsigned nframes() const { return nframes_; }
  virtual double frame_rate() const { return frame_rate_; }

  virtual vidtk::timestamp timestamp() const;
  virtual vil_image_view<vxl_byte> image() const;

  virtual unsigned ni() const { return ni_; }
  virtual unsigned nj() const { return nj_; }

protected:
  vidl_ffmpeg_istream istr_;

  bool read_klv( const klv_data &klv);

  bool init_timestamp();

private:
  bool read_ahead_;
  vil_image_view<vxl_byte> img_;

  unsigned ni_;
  unsigned nj_;

  std::string filename_;
  unsigned nframes_;
  double frame_rate_;
  unsigned start_frame_;
  unsigned stop_after_frame_;
  int n_blank_frames_after_eoi_;
  bool blank_frame_mode_;

  //Presentation timestamp for the last frame with a metadata time stamp
  vidtk::timestamp meta_ts_;
  vidtk::timestamp ts_;
  std::string klv_type_;

  mutable unsigned int last_frame_;  //for blank frame mode
  std::string time_type_;
  double pts_of_meta_ts_;
  bool first;
  static boost::mutex m_open_;

  bool simulate_stream_;
  boost::posix_time::ptime start_local_time_;
  vidtk::timestamp start_source_time_;

  double ts_scaling_factor_;
};


} // end namespace vidtk


#endif // vidtk_vidl_ffmpeg_frame_process_h_
