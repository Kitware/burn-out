/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_INTERPOLATE_GPS_TRACK_STATE_PROCESS_H
#define INCL_INTERPOLATE_GPS_TRACK_STATE_PROCESS_H

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/track.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/config_block.h>
#include <utilities/buffer.h>

#include <tracking/track.h>
#include <tracking/track_state.h>

namespace vidtk
{


class interpolate_gps_track_state_process
  : public process
{
public:

  typedef interpolate_gps_track_state_process self_type;

  interpolate_gps_track_state_process( const vcl_string& name );

  ~interpolate_gps_track_state_process();

  virtual config_block params() const;

  virtual bool reset();

  virtual bool set_params( const config_block& blk );

  virtual bool initialize();

  virtual bool step();

  // Process inputs
  void set_input_tracks( const vcl_vector<track_sptr> & tl );
  VIDTK_INPUT_PORT( set_input_tracks, const vcl_vector<track_sptr>& );

  // Process outputs
  const vcl_vector<track_sptr> & output_track_list() const;
  VIDTK_OUTPUT_PORT( const vcl_vector<track_sptr> &, output_track_list );


private:
  vcl_vector<track_sptr> const * tracks;
  vcl_vector<track_sptr> output_tracks;

}; // class

} // namespace

#endif

