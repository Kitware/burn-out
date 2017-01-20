/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_downsampling_process_h_
#define vidtk_frame_downsampling_process_h_

// VIDTK Includes
#include <process_framework/process.h>
#include <utilities/downsampling_process.h>
#include <utilities/video_metadata.h>

// VXL Includes
#include <vil/vil_image_view.h>

namespace vidtk
{

/// An experimental process which serves to downsample the input image stream
/// in extreme circumstances where the tracker appears to be processing data
/// at a rate below that of the incoming data for an extended period of time.
template< typename PixType >
class frame_downsampling_process
  : public downsampling_process
{

public:

  typedef frame_downsampling_process self_type;

  frame_downsampling_process( std::string const& );
  virtual ~frame_downsampling_process() {}

  virtual vidtk::config_block params() const;
  virtual bool set_params( vidtk::config_block const& );

  VIDTK_PASS_THRU_PORT( image, vil_image_view< PixType > );
  VIDTK_PASS_THRU_PORT( metadata, vidtk::video_metadata );
  VIDTK_PASS_THRU_PORT( timestamp, vidtk::timestamp );

  virtual vidtk::timestamp current_timestamp();
  virtual bool is_frame_critical();

private:

  vidtk::config_block params_;
  bool retain_metadata_frames_;
};

}

#endif // vidtk_frame_downsampling_process_h_
