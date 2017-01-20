/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vidl_ffmpeg_metadata_frame_process_h_
#define vidtk_vidl_ffmpeg_metadata_frame_process_h_

#include <video_io/vidl_ffmpeg_frame_process.h>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Reads video from ffmpeg, metadata from file.
 *
 * This generates a video stream. The video is decoded, using ffmpeg,
 * from the supplied video file/stream. Anything supported by ffmepg
 * can be used.  The metadata is read from a separate file.
 */
class vidl_ffmpeg_metadata_frame_process
  : public vidl_ffmpeg_frame_process
{
public:
  vidl_ffmpeg_metadata_frame_process( std::string const& name );
  virtual ~vidl_ffmpeg_metadata_frame_process() { }

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  virtual bool seek( unsigned frame_number );

  virtual vidtk::timestamp timestamp() const;
  virtual vidtk::video_metadata metadata() const;

private:
  std::string metadata_filename_;
  std::vector<vidtk::timestamp> timestamps_;
  std::vector<vidtk::video_metadata> metadata_;
  int cur_idx_;
  int start_frame_;
};


} // end namespace vidtk


#endif // vidtk_vidl_ffmpeg_metadata_frame_process_h_
