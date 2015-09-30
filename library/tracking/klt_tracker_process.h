/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_klt_tracker_process_h_
#define vidtk_klt_tracker_process_h_

#include <vcl_list.h>
#include <vcl_memory.h>
#include <utilities/timestamp.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/image_object.h>
#include <tracking/da_so_tracker.h>
#include <tracking/track.h>


namespace vidtk
{


/// \brief Track KLT features across images.
///
/// The tracked features are represented using vidtk::track
/// structures.
///
/// \note This KLT tracker will only work for contiguous greyscale
/// images.
class klt_tracker_process
  : public process
{
public:
  typedef klt_tracker_process self_type;

  klt_tracker_process( vcl_string const& name );

  ~klt_tracker_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  bool reinitialize();

  virtual bool step();

  virtual bool reset();

  // for the moment, we only allow a 8-bit image. If we want to handle
  // 16-bit image, we would need to extend the underlying KLT to do
  // that.  Then, we can handle 16-bit images by overloading set_image
  // with a 16-bit image version, instead of making the whole class
  // templated.

  /// Set the next image to process.
  void set_image( vil_image_view< vxl_byte > const& img );

  VIDTK_INPUT_PORT( set_image, vil_image_view< vxl_byte > const& );

  /// The timestamp for the current frame.
  void set_timestamp( timestamp const& ts );

  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );


  /// Set of tracks that are actively being tracked.
  vcl_list< track_sptr > const& active_tracks() const;

  VIDTK_OUTPUT_PORT( vcl_list< track_sptr > const&, active_tracks );

  /// Set of tracks that were terminated at the last step.
  vcl_vector< track_sptr > const& terminated_tracks() const;

  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, terminated_tracks );

  /// Set of new tracks that were created at the last step.
  vcl_vector< track_sptr > const& created_tracks() const;

  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, created_tracks );

protected:
  // documentation in .cxx
  void initialize_features();
  void create_new_tracks();
  void update_tracks();
  void terminate_tracks();
  void replace_lost_tracks();

  // ----- Parameters

  config_block config_;

  /// Number of features to (attempt to) track.
  unsigned num_feat_;

  /// Try to replace lost features?
  bool replace_lost_;

  /// Output KLT messages?
  int verbosity_;

  /// Feature window size.
  int window_width_;
  int window_height_;

  /// Maximum displacement allowed for a feature.
  unsigned search_range_;

  /// Don't actually compute anything.
  bool disabled_;

  // ----- Cached input

  /// The current timestamp.
  timestamp const* cur_ts_;

  /// The current frame.
  vil_image_view<vxl_byte> const* cur_img_;

  // ----- Internal state and output

  struct klt_state;
  klt_state * state_;

  /// Set of tracks corresponding to still active features
  vcl_list< track_sptr > active_tracks_;

  /// Map from the KLT feature id into the active_tracks_ list.
  vcl_vector< vcl_list< track_sptr >::iterator > track_map_;

  /// Set of tracks that were killed this round.
  vcl_vector< track_sptr > terminated_tracks_;

  /// Set of tracks that were created this round.
  vcl_vector< track_sptr > created_tracks_;

  // Id given to the last track
  unsigned next_track_id_;

  // If the process is retained across two calls on intialize(), then
  // we only want to "initialize" only the first time.
  bool initialized_;

  // See description in cxx file.
  unsigned min_distance_;

  // See description in cxx file.
  unsigned skipped_pixels_;
};


} // end namespace vidtk


#endif // vidtk_klt_tracker_process_h_
