/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef video_metadata_writer_process_h_
#define video_metadata_writer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/video_metadata.h>
#include <utilities/timestamp.h>
#include <utilities/write_videometa_kml.h>

#include <boost/format.hpp>

namespace vidtk
{

class video_metadata_writer_process
  : public process
{
public:
  typedef video_metadata_writer_process self_type;

  video_metadata_writer_process( std::string const& name );
  virtual ~video_metadata_writer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  // Input ports
  void set_meta( video_metadata vm );
  VIDTK_INPUT_PORT( set_meta, video_metadata );

  void set_timestamp( timestamp ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp );


protected:
  void reset_variables();

  config_block config_;

  boost::format pattern_;

  std::string kml_fname_;
  write_videometa_kml kml_writer_;
  unsigned int kml_ni_;
  unsigned int kml_nj_;
  bool kml_write_given_corner_points_;

  video_metadata meta_;
  timestamp time_;

  bool disabled_;
  unsigned int step_count_;

  enum {MODE_VIDTK, MODE_KML} mode_;
};

} // end namespace vidtk

#endif // video_metadata_writer_process_h_
