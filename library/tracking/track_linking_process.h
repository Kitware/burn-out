/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_linking_process_h_
#define vidtk_track_linking_process_h_

#include <vcl_vector.h>
#include <vcl_map.h>

#include <tracking/track.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/timestamp.h>

#include "track_linking.h"
#include "multi_track_linking_functors_functor.h"

namespace vidtk {

class track_linking_process : public process {
public:

  struct current_link
  {
    current_link(track_sptr self,
                 unsigned int next_id = static_cast<unsigned int>(-1),
                 unsigned int prev_id = static_cast<unsigned int>(-1) )
      :  self_(self), next_id_(next_id), prev_id_(prev_id),
         is_finalized_track_(false), is_fixed_(false)
    { }
    track_sptr self_;
    unsigned int next_id_;
    unsigned int prev_id_;
    bool is_finalized_track_;
    bool is_terminated() const
    { return next_id_ == static_cast<unsigned int>(-1); }

    bool is_start()
    { return prev_id_ == static_cast<unsigned int>(-1); }

    bool is_fixed_;
  };

  // Needed for port macros
  typedef track_linking_process self_type;

  track_linking_process( vcl_string const& name );

  void set_input_terminated_tracks( vcl_vector< track_sptr > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_input_terminated_tracks, vcl_vector< track_sptr > const& );

  void set_timestamp( timestamp const& ts );
  VIDTK_OPTIONAL_INPUT_PORT( set_timestamp, timestamp const& );

  void set_input_active_tracks(vcl_vector< track_sptr > const&);
  VIDTK_OPTIONAL_INPUT_PORT( set_input_active_tracks, vcl_vector< track_sptr > const& );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const &, get_output_tracks );
  vcl_vector< track_sptr > const & get_output_tracks() const;

  VIDTK_OUTPUT_PORT( vcl_vector< current_link > const &, get_output_link_info );
  vcl_vector< current_link > const & get_output_link_info() const;

  vcl_vector< track_sptr > get_current_linking();

protected:

  bool outside_time(track_sptr track);

  void update_output();

  track_linking<multi_track_linking_functors_functor> track_linker_;

  vcl_map< unsigned int, current_link > input_tracklets_to_link_;
  /// This is where the partial constructed track is stored.  
  /// The key is the most resent track id
  vcl_map< unsigned int, track_sptr > constructed_tracks_;
  vcl_vector< track_sptr > output_tracks_;
  vcl_vector< current_link > output_link_info_;
  

  unsigned int time_window_;
  timestamp const * timestamp_;
  config_block config_;

  ///to keep form doing unessisary linking when no new tracks were added since last linking
  bool tracks_were_added_since_last_time_;

  bool flush_;
  bool terminate_;
  bool disabled_;

  friend bool operator<( track_linking_process::current_link const & l,
                         track_linking_process::current_link const & r );

};

} // namespace vidtk

#endif // #ifndef vidtk_track_linking_process_h_

