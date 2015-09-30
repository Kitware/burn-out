/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_da_tracker_process_h_
#define vidtk_da_tracker_process_h_

#include <vcl_vector.h>
#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <utilities/timestamp.h>
#include <utilities/buffer.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/image_object.h>
#include <tracking/da_so_tracker.h>
#include <tracking/track.h>
#include <tracking/amhi.h>
#include <tracking/multiple_features_process.h>
#include <tracking/tracker_cost_function.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

class da_tracker_process
  : public process
{
public:
  typedef da_tracker_process self_type;

  da_tracker_process( vcl_string const& name );

  ~da_tracker_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// -------------------------- Input Ports ----------------------------

  /// Set of objects (MODs) detected on the current frame.
  void set_objects( vcl_vector< image_object_sptr > const& mods );
  VIDTK_OPTIONAL_INPUT_PORT( set_objects, vcl_vector< image_object_sptr > const& );

  void set_fg_objects( vcl_vector< image_object_sptr > const& fg_objs );
  VIDTK_OPTIONAL_INPUT_PORT( set_fg_objects, vcl_vector< image_object_sptr > const& );

  /// The timestamp for the current frame.
  void set_timestamp( timestamp const& ts );
  VIDTK_OPTIONAL_INPUT_PORT( set_timestamp, timestamp const& );

  /// Set of new initialized trackers on.
  void set_new_trackers( vcl_vector< da_so_tracker_sptr > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_new_trackers, vcl_vector< da_so_tracker_sptr > const& );

 /// Set of new initialized trackers on. - width height kalman
  void set_new_wh_trackers(  vcl_vector< da_so_tracker_sptr > const&  );
  VIDTK_OPTIONAL_INPUT_PORT( set_new_wh_trackers, vcl_vector< da_so_tracker_sptr > const& );



  // Added for updating the track foreground model (thru amhi), switch to
  // first frame only?
  void set_source_image( vil_image_view<vxl_byte> const& image_buf );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  void set_mf_params( multiple_features const& mf );

  //Added for fg_tracker.  
  void set_image2world_homography( vgl_h_matrix_2d<double> const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_image2world_homography, vgl_h_matrix_2d<double> const& );

  //Added for fg_tracker.  
  void set_world2image_homography( vgl_h_matrix_2d<double> const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_world2image_homography, vgl_h_matrix_2d<double> const& );

  void set_world_units_per_pixel( double val );
  VIDTK_OPTIONAL_INPUT_PORT( set_world_units_per_pixel, double );

  void set_active_tracks( vcl_vector< track_sptr > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_active_tracks, vcl_vector< track_sptr > const& );

  /// -------------------------- Output Ports ----------------------------

  /// List of MODs that weren't used by any of the active trackers.
  vcl_vector< image_object_sptr > const& unassigned_objects() const;
  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, unassigned_objects );

  /// Set of tracks that are actively being tracked.
  vcl_vector< track_sptr > const& active_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, active_tracks );

  /// Set of active trackers.
  vcl_vector< da_so_tracker_sptr > const& active_trackers() const;
  VIDTK_OUTPUT_PORT( vcl_vector< da_so_tracker_sptr > const&, active_trackers );

  /// Set of tracks that were terminated at the last step.
  vcl_vector< track_sptr > const& terminated_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, terminated_tracks );

  /// Set of tracks that were terminated at the last step.
  vcl_vector< da_so_tracker_sptr > const& terminated_trackers() const;
  VIDTK_OUTPUT_PORT( vcl_vector< da_so_tracker_sptr > const&, terminated_trackers );

  vcl_vector< image_object_sptr> updated_fg_objects() const;
  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr>, updated_fg_objects );


  double gate_sigma() const;

  // Terminate all tracks
  void terminate_all_tracks();

  // Clears the tracks, terminated tracks, and the trackers.
  void reset_trackers_and_tracks();

protected:
  void update_trackers();
  void terminate_trackers();
  void append_state( da_so_tracker& tracker, track& trk, image_object_sptr obj );

  bool update_fg_tracks( track_sptr , da_so_tracker_sptr , image_object_sptr & );

  bool update_obj(vcl_vector< vgl_box_2d<unsigned> > const & bboxes,
                  image_object & obj,
                  da_so_tracker const & tracker,
                  vil_image_view<float> const & amhi_weight =
                    vil_image_view<float>() );

  void update_bbox_wh( double width, double height, image_object &obj );

  void flag_mod_overlaps( vcl_vector<bool> & assigned_mods, 
                          vcl_vector<unsigned> const & fg_matched,
                          vcl_vector<image_object_sptr> const & fg_new_objs);

  inline bool boxes_intersect( vgl_box_2d<unsigned> const& a,
                               vgl_box_2d<unsigned> const& b ) const;

  // ----- Parameters

  config_block config_;

  /// The covariance to be used for MODs.
  vnl_double_2x2 mod_cov_;

  /// The gate used to exclude assocations for noise reduction.
  double gate_sigma_sqr_;

  /// Time based termination criterion.
  unsigned term_missed_count_;
  double term_missed_secs_;

  tracker_cost_function_sptr tracker_cost_function_;

  enum assignment_alg_type { GREEDY, HUNGARIAN };
  assignment_alg_type assignment_alg_;

  // Flag indicating whether AMHI model for appearance modelling is 
  // being used or not.
  bool amhi_enabled_;

  // Whether to *track* amhi boxes vs. raw bboxes
  bool amhi_update_objects_;

  //  Fire up foreground tracking on the tracks tracks that did not 
  //  match in current iteration of correspondence. 
  bool fg_tracking_enabled_;

  // Number of miss detections on a track before the fg_tracker kicks in.
  unsigned fg_min_misses_;

  // Max time for which fg tracking can keep a track alive
  double fg_max_mod_lag_;
  double fg_padding_factor_;

  // Whether to use the prediction when searching for the foreground match
  // or not.
  bool fg_predict_forward_;

  enum location_type { centroid, bottom };
  location_type loc_type_; // centroid or bottom

  // [0,1] threshold to fit tighter bbox to the amhi weight matrix.
  // -1.0 is the "disable" value (default).  
  float amhi_tighter_bbox_frac_;

  // Up to this num of frames in history is used to compute area feature
  unsigned int area_window_size_;

  //The weight of each frame is this much less than the next frame
  //in history for computing area feature.
  double area_weight_decay_rate_;

  // Min speed for an object to be moving in order to use 
  // data-association (da) tracking. It would fall-back to perform foreground
  // (fg) tracking instead. 
  double min_speed_sqr_for_da_;

  // Min area similarity between an object and the tracked target
  double min_area_similarity_for_da_;

  // Min color similarity between an object and the tracked target
  double min_color_similarity_for_da_;

  // Whether to over-write incoming track ids or not?
  bool reassign_track_ids_;

  /// See add_parameter for details.
  double min_bbox_overlap_percent_;

  // ----- Cached input

  /// We assume that z=0 in the world_loc_ of all the MODs.
  vcl_vector< image_object_sptr > const* mods_;

  /// The current timestamp.
  timestamp const* cur_ts_;

  // Current image for computing appearance cost.
  vil_image_view<vxl_byte> const* image_;

  multiple_features const * mf_params_;

  // fg_tracking: Used to project predicted tracker pos to image. 
  vgl_h_matrix_2d<double> const* wld2img_H_;
  vgl_h_matrix_2d<double> const* img2wld_H_;

  vcl_vector< image_object_sptr > const* fg_objs_;
  vcl_vector< image_object_sptr > updated_fg_objs_;

  // Not using * for this one becasuse of the "reverse edge"
  vcl_vector< track_sptr > const * updated_active_tracks_;

  vcl_vector< da_so_tracker_sptr > const * new_trackers_;

  // ----- Internal state and output

  /// Set of active trackers.
  vcl_vector< da_so_tracker_sptr > trackers_;

  vcl_vector< da_so_tracker_sptr > wh_trackers_;
  vcl_vector< da_so_tracker_sptr > new_wh_trackers_;


  /// Set of tracks corresponding to the active trackers
  vcl_vector< track_sptr > tracks_;
  vcl_vector< track_sptr > new_tracks_;

  /// Set of tracks that were killed this round.
  vcl_vector< track_sptr > terminated_tracks_;

  /// Set of trackers that were killed this round.
  vcl_vector< da_so_tracker_sptr > terminated_trackers_;

  /// Set of trackers that were killed this round.
  vcl_vector< da_so_tracker_sptr > terminated_wh_trackers_;

  // See unassigned_objects().
  vcl_vector< image_object_sptr > unassigned_mods_;

  /// Id given to the last track.
  unsigned next_track_id_;

  /// Offset to add to the track id when reporting to the outside world.
  unsigned track_id_offset_;

  // Used for fg model update and fg tracking
  vbl_smart_ptr< amhi<vxl_byte> > amhi_;

  vcl_ofstream train_file_self_;

  double world_units_per_pixel_;

  ///Causes the algorithm to look for the forground between the previous
  ///location and the predicted location.
  bool search_both_locations_;

  const double INF_;

};


} // end namespace vidtk


#endif // vidtk_da_tracker_process_h_
