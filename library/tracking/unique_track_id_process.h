/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_unique_track_id_process_h_
#define vidtk_unique_track_id_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/track.h>

#include <utilities/config_block.h>
#include <utilities/timestamp.h>

#include <vcl_fstream.h>
#include <vcl_map.h>
#include <vcl_utility.h>  // for vcl_pair

namespace vidtk
{

class unique_track_id_process
  : public process
{
public:
  typedef unique_track_id_process self_type;
  typedef vcl_pair<unsigned, unsigned> track_key_t;
  typedef vcl_map< track_key_t, unsigned> track_key_map_t;

  unique_track_id_process( vcl_string const& name );
  ~unique_track_id_process();

  // process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  // Input timestamp
  void input_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( input_timestamp, timestamp const& );

  // Input active tracks
  void input_active_tracks( track_vector_t const& active_tracks );
  VIDTK_INPUT_PORT( input_active_tracks, track_vector_t const& );

  // Input terminated tracks
  void input_terminated_tracks( track_vector_t const& terminated_tracks );
  VIDTK_INPUT_PORT( input_terminated_tracks, track_vector_t const& );

  // ----------------------------------------------------------------
  // Output timestamp
  timestamp const& output_timestamp() const;
  VIDTK_OUTPUT_PORT( timestamp const&, output_timestamp );

  // Output active tracks
  track_vector_t const& output_active_tracks() const;
  VIDTK_OUTPUT_PORT( track_vector_t const&, output_active_tracks );

  // Output terminated tracks
  track_vector_t const& output_terminated_tracks() const;
  VIDTK_OUTPUT_PORT( track_vector_t const&, output_terminated_tracks );

  track_key_map_t const& output_track_map() const;
  VIDTK_OUTPUT_PORT( track_key_map_t const&, output_track_map );

  vcl_vector< track_key_t > const& output_active_track_keys() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_key_t > const& , output_active_track_keys );


private:
  config_block config_;
  bool disabled_;
  vcl_string map_filename_;

  timestamp timestamp_;
  track_vector_t active_tracks_;
  track_vector_t terminated_tracks_;
  unsigned next_unique_track_id_;

  // Map tracker <subtype, input_id> to output_id
  track_key_map_t track_id_map_;
  vcl_vector < track_key_t > active_track_keys_;

  vcl_ofstream map_fstr_;  // for writing map file
};  // end class unique_track_id_process


}  // end namespace vidtk


#endif // vidtk_world_smooth_image_process_h_
