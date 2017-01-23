/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vidl_ffmpeg_writer_process_h_
#define vidtk_vidl_ffmpeg_writer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <vidl/vidl_ffmpeg_ostream.h>
#include <vidl/vidl_ffmpeg_ostream_params.h>
#include <vidl/vidl_frame_sptr.h>
#include <vil/vil_image_view.h>


namespace vidtk
{


class vidl_ffmpeg_writer_process
  : public process
{
public:
  typedef vidl_ffmpeg_writer_process self_type;

  vidl_ffmpeg_writer_process( std::string const& name );

  ~vidl_ffmpeg_writer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_frame( vil_image_view<vxl_byte> const& img );

  VIDTK_INPUT_PORT( set_frame, vil_image_view<vxl_byte> const& );

protected:
  config_block config_;

  vidl_frame_sptr src_frame_;

  vidl_ffmpeg_ostream_params param_;
  vidl_ffmpeg_ostream ostr_;
};


} // end namespace vidtk


#endif // vidtk_vidl_ffmpeg_writer_process_h_
