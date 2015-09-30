/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_filter_tracks_process2_h_
#define vidtk_filter_tracks_process2_h_

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
class filter_tracks_process2
  : public process
{
public:
  typedef filter_tracks_process2 self_type;

  filter_tracks_process2( vcl_string const& name );

  ~filter_tracks_process2();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  // Useful for flushing track at end of input stream. 
  //
  // This function contains the main body of the step() function. Moved here 
  // because we needed to flush the tracks that are still 'alive' in tracker
  // and are not yet terminated but the input stream (.avi or homography stream) 
  // got reset. 
  //
  // Usage in the main track file: 
  // write_out_tracks( trk_filter2.filter_these( tracker.active_tracks() ) ); 
  //
  vcl_vector< track_sptr > filter_these( vcl_vector< track_sptr > const & in_trks );

  void set_source_tracks( vcl_vector< track_sptr > const& objs );

  VIDTK_INPUT_PORT( set_source_tracks, vcl_vector< track_sptr > const& );

  vcl_vector< track_sptr > const& tracks() const;

  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, tracks );

private:
  bool filter_out_multi_params( track_sptr const& trk ) const;
  bool filter_out_max_speed( track_sptr const& trk ) const;
  bool filter_out_min_arc_length( track_sptr const& trk ) const;


  config_block config_;

  //Configuration parameters
  double min_distance_covered_sqr_;
  double min_arc_length_sqr_;
  double max_speed_sqr_;
  double min_speed_sqr_;
  double min_track_length_;
  unsigned samples_;
  double max_median_area_;
  double max_median_area_amhi_;
  double max_frac_ghost_detections_;
  double max_aspect_ratio_amhi_;
  double min_occupied_bbox_amhi_;
  bool disabled_;
    
  // Input data
  vcl_vector< track_sptr > const* src_trks_;

  // Output data
  vcl_vector< track_sptr > out_trks_;

};


} // end namespace vidtk


#endif // vidtk_filter_tracks_process2_h_
