/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vidl_ffmpeg_frame_process
#define vidtk_vidl_ffmpeg_frame_process

#include <video/frame_process.h>
#include <vidl/vidl_ffmpeg_istream.h>

namespace vidtk
{

class vidl_ffmpeg_frame_process
  : public frame_process<vxl_byte>
{
public:
  vidl_ffmpeg_frame_process( vcl_string const& name );

  config_block params() const;

  bool set_params( config_block const& );

  bool initialize();

  bool step();

  bool seek( unsigned frame_number );

  vidtk::timestamp timestamp() const;

  vil_image_view<vxl_byte> const& image() const;

  unsigned ni() const { return ni_; }
  unsigned nj() const { return nj_; }

private:
  bool read_ahead_;
  vidl_ffmpeg_istream istr_;
  vil_image_view<vxl_byte> img_;

  unsigned ni_;
  unsigned nj_;

  vcl_string filename_;
  unsigned start_frame_;
  unsigned stop_after_frame_;
  int n_blank_frames_after_eoi_;
  mutable vidtk::timestamp last_timestamp_;
  bool blank_frame_mode_;
};


} // end namespace vidtk


#endif // vidtk_vidl_ffmpeg_frame_process
