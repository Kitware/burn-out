/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_writer_process_h_
#define vidtk_track_writer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/image_object.h>
#include <tracking_data/track.h>
#include <tracking_data/io/track_writer.h>

namespace vidtk
{

  class timestamp;

/// Writes track entries to file.
///
/// The output can be of different formats.  The format is chosen by
/// the parameter \c format.  See the documentation of the various
/// write_formatX methods (e.g. write_format0) for descriptions of the
/// format.
///
/// Note that this process writes entire tracks, not partial.  This is
/// a suitable process for writing terminated tracks but innapropriate for
/// active tracks
///

class track_writer_process
  : public process
{
public:
  typedef track_writer_process self_type;

  track_writer_process( std::string const& name );

  virtual ~track_writer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  void set_image_objects( std::vector< image_object_sptr > const& objs );
  VIDTK_INPUT_PORT( set_image_objects, std::vector< image_object_sptr > const& );

  void set_tracks( std::vector< track_sptr > const& trks );
  VIDTK_INPUT_PORT( set_tracks, std::vector< track_sptr > const& );

  void set_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  bool is_disabled() const;

private:
  bool fail_if_exists( std::string const& filename );

  void clear();

    // Parameters

  config_block config_;

  bool disabled_;
  std::string filename_;
  bool allow_overwrite_;

  track_writer writer_;

  // Cached input
  std::vector<image_object_sptr> const* src_objs_;
  std::vector<track_sptr> const* src_trks_;
  timestamp ts_;
};


} // end namespace vidtk


#endif // vidtk_track_writer_process_h_
