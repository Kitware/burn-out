/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_filter_tracks_process_h_
#define vidtk_filter_tracks_process_h_

#include <vcl_vector.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/track.h>


namespace vidtk
{


/// Filter out tracks that don't match certain criteria.
///
/// The filter can apply multiple criteria in a single step, but it is
/// implementation defined as to the order in which the criteria are
/// applied.
class filter_tracks_process
  : public process
{
public:
  typedef filter_tracks_process self_type;

  filter_tracks_process( vcl_string const& name );

  ~filter_tracks_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_source_tracks( vcl_vector< track_sptr > const& objs );

  VIDTK_INPUT_PORT( set_source_tracks, vcl_vector< track_sptr > const& );

  vcl_vector< track_sptr > const& tracks() const;

  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, tracks );

private:
  bool filter_out_min_speed( track_sptr const& trk ) const;
  bool filter_out_max_speed( track_sptr const& trk ) const;


  config_block config_;

  double min_speed_sqr_;
  double max_speed_sqr_;

  // Input data
  vcl_vector< track_sptr > const* src_trks_;

  // Output data

  vcl_vector< track_sptr > out_trks_;

};


} // end namespace vidtk


#endif // vidtk_filter_tracks_process_h_
