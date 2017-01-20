/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_reader_process_h_
#define vidtk_track_reader_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/track.h>
#include <tracking_data/io/track_reader.h>
#include <tracking_data/io/active_tracks_reader.h>
#include <utilities/timestamp.h>

namespace vidtk
{

// ----------------------------------------------------------------
/** Track reading process.
 *
 * This process reads tracks of the following formats:
 * kw18, mit/xml, viper/xgtf, vsl.
 *
 * In the default mode, the tracks from the configured file can be
 * returned based on a frame number counter.  The frame counter starts
 * at zero for the first step() call and increments thereafter.
 *
 * If batch mode is enabled (set_batch_mode()), all tracks from the
 * configured file are read on the first process step() call.
 *
 */
class track_reader_process
  : public process
{
public:
  typedef track_reader_process self_type;

  track_reader_process(std::string const& name);
  virtual ~track_reader_process();

  // process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_timestamp(timestamp const & ts);
  VIDTK_OPTIONAL_INPUT_PORT(set_timestamp, timestamp const &);

  // Port producing terminated tracks.
  //
  // Produces valid output in active (active_tracks_disable_==false) or
  // batch (active_tracks_disable_==true) mode. In active mode the
  // terminated tracks are published *after* the last state in the track.
  // This logic causes the process to run for one extra step after the
  // last frame of the video. Consumers of the active mode need to aware
  // of this extra step, i.e. expect terminated track from the last frame
  // *after* they have seen an invalid timestamp from timestamp_generator.
  std::vector<track_sptr> tracks() const;
  VIDTK_OUTPUT_PORT(std::vector <track_sptr>, tracks);

  std::vector<track_sptr> active_tracks() const;
  VIDTK_OUTPUT_PORT(std::vector <track_sptr>, active_tracks);

  ///Output the timestamp for the current active and terminated tracks.
  timestamp get_current_timestamp() const;
  VIDTK_OUTPUT_PORT(timestamp, get_current_timestamp);

  /** Return process disabled status.
   */
  bool is_disabled() const;

  /** Set batch reading mode. When batch mode is set (off by default),
   * all tracks in the input file are returned on the first process
   * cycle (step() call).
   */
  void set_batch_mode( bool batch_mode );
  bool is_batch_mode() const;


private:
  std::string filename_;
  config_block config_;
  bool disabled_;
  bool ignore_occlusions_;
  bool ignore_partial_occlusions_;
  std::string path_to_images_;   /*KW18 Only*/
  double frequency_;            /*MIT Only for now*/

  vidtk::ns_track_reader::track_reader_options reader_opt_;


  std::vector < track_sptr > tracks_;
  track_reader* reader_;

  /// Default false: read tracks in steps. True: read all tracks together.
  bool batch_mode_;
  bool batch_not_read_;

  // support for active tracks
  bool active_tracks_disable_;
  // read timestamps from the tracks instead of an external source
  bool timestamps_from_tracks_;
  // a frame counter used when getting timestamps from track files
  unsigned current_frame_;
  std::vector < track_sptr > active_tracks_;
  active_tracks_reader * active_tracks_reader_;

  // Timestamp of the current frame for which tracks need to be published.
  //
  // This parameter is only used when active_tracks_disable_ == false
  timestamp ts_;
  timestamp tracker_ts_;

  // Copy of the last step's timestamp, used for flushed tracks in active mode.
  timestamp last_ts_;

  // Flag to identify whether the one extra step has been performed at the EOV.
  bool terminated_flushed_;

  // Flag to determine whether to write to lat/lon or image_object::world_loc_.
  bool read_lat_lon_for_world_;

};

}

#endif
