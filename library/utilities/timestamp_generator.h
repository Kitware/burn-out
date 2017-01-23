/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_timestamp_generator_h_
#define vidtk_timestamp_generator_h_

#include <utilities/oc_timestamp_hack.h>
#include <utilities/config_block.h>
#include <utilities/timestamp.h>
#include <utilities/checked_bool.h>
#include <process_framework/process.h>
#include <fstream>

namespace vidtk
{

class timestamp_generator
{
public:
  timestamp_generator();
  ~timestamp_generator();

  config_block params() const;
  checked_bool set_params( config_block const& );
  bool initialize();
  process::step_status provide_timestamp( timestamp & ts );

private:
  bool init_pair_file();
  bool parse_pair_file( timestamp & ts );

  // Use the manual_frame_rate_ to provide timestamp in secs.
  // NOTE: produces timestamp in MICRO seconds, this is what
  // timestamp class expects.
  bool apply_manual_framerate( timestamp & ts );

  config_block config_;

  // Config parameters

  /// \internal Which parsing method should we use?
  enum method_types { disabled, update_frame_number, ocean_city, fr_ts_pair_file };
  method_types method_;

  /// \internal A filename, if it is required by the parsing method.
  std::string filename_;

  /// \internal Used to generate timestamp in secs.
  // This should be replaced by the real frame_rate info from the
  // input stream.
  double manual_frame_rate_;

  /// \internal Absolute starting time of the source image sequence/video,
  /// used in case when the timestamp stream does not capture this
  /// information.
  double base_time_;

  unsigned skip_frames_, start_frame_, end_frame_;

  unsigned frame_number_;

  // parse helpers
  oc_timestamp_hack oc_timestamp_parser_;

  std::ifstream in_stream_;
};


} // end namespace vidtk


#endif // vidtk_timestamp_generator_h_
