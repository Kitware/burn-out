/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_initializer_process_h_
#define vidtk_track_initializer_process_h_

#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_2x2.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/image_object.h>
#include <tracking/track.h>
#include <tracking/multiple_features.h>

#include <utilities/timestamp.h>
#include <utilities/buffer.h>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>

// forward declare
template  < class COORD_T, class VALUE_T >
class rsdl_bins_2d;

//#define TEST_INIT

namespace vidtk
{

/// \brief Determines possible new tracks by analysing a sequence of object
/// detections for consistent motion.
///
/// Since the algorithm depends on temporal analysis, it requires a
/// pointer to a buffer of detections, and a buffer of corresponding
/// timestamps.
class track_initializer_process
  : public process
{
public:
  typedef track_initializer_process self_type;

  track_initializer_process( vcl_string const& name );

  ~track_initializer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_mf_params( multiple_features const& mf );

  /// \brief Set the buffer of MODs used to initialize the track.
  ///
  /// This process will maintain a pointer to \a buf, and thus the
  /// buffer object must persist for the lifetime of this process.
  void set_mod_buffer( buffer< vcl_vector<image_object_sptr> > const& buf );

  VIDTK_INPUT_PORT( set_mod_buffer, buffer< vcl_vector<image_object_sptr> > const& );

  /// \brief Set the buffer of timestamps corresponding to the MOD
  /// buffer.
  ///
  /// This process will maintain a pointer to \a buf, and thus the
  /// buffer object must persist for the lifetime of this process.
  void set_timestamp_buffer( buffer<timestamp> const& buf );

  VIDTK_INPUT_PORT( set_timestamp_buffer, buffer<timestamp>  const& );

  vcl_vector<track_sptr> const& new_tracks() const;

  VIDTK_OUTPUT_PORT( vcl_vector<track_sptr> const&,  new_tracks );

  void set_wld2img_homog_buffer( buffer< vgl_h_matrix_2d<double> > const& );
  VIDTK_INPUT_PORT( set_wld2img_homog_buffer, buffer< vgl_h_matrix_2d<double> >  const& );

private:

  // documentation in .cxx
  void add_new_state( track_sptr& trk,
                      image_object_sptr const& obj,
                      timestamp const& t );

  typedef rsdl_bins_2d<double, image_object_sptr> bin_type;

  // documentation in .cxx
  void closest_object( image_object::float3d_type const& src_loc,
                       image_object::float3d_type const& tgt_loc,
                       double src_time,
                       double full_interval,
                       unsigned offset,
                       vcl_vector<bin_type> const& bins,
                       vcl_vector< vcl_vector< image_object_sptr > > const & brut_force,
                       double& best_cost_out,
                       image_object_sptr& best_idx_out ) const;

  void closest_object2( image_object const& src_obj,
                        image_object const& tgt_obj,
                        double src_time,
                        double full_interval,
                        unsigned offset,
                        vcl_vector<bin_type> const& bins,
                        double& best_cost_out,
                        image_object_sptr& best_obj_out );

  bool find_close_objs( image_object_sptr const& obj) const;

protected:

  enum vel_init { USE_N_BACK_END_VEL, USE_ROBUST_SPEED_FIT, USE_ROBUST_POINTS_FIT };
  vel_init method_;

  config_block config_;

  /// Number of frames over which to compute MOD. Must be > 0.
  unsigned delta_;

  /// The distance maximum allowable distance in term of mahalanobis
  /// distance from an expected
  /// location to an actual location.  Beyond this threshold, it's
  /// considered not associated.
  double kinematics_sigma_gate_;

  /// The number of frames in which we may miss a detection.
  double allowed_miss_count_;

  /// The penality for missing a detection.  This should be much
  /// greater than max_dist_, because it'll be the basis for
  /// comparison.
  double miss_penalty_;

  /// Sigma-squared for tangential component of distance from expected
  /// point.
  double tang_sigma_sqr_;
  double one_over_tang_sigma_sqr_;

  /// Sigma-squared for normal component of distance from expected
  /// point.
  double norm_sigma_sqr_;
  double one_over_norm_sigma_sqr_;

  /// Use a bin structure to find the closest matching objects when
  /// computing the cost of a particular assignment.  Useful when then
  /// the number of objects is very large.
  bool use_bins_;
  double bin_lo_[2];
  double bin_hi_[2];
  double bin_size_[2];

  enum assignment_alg_type { GREEDY, HUNGARIAN };
  assignment_alg_type assignment_alg_;

  /// If the size of the cost matrix (num source x num targets)
  /// exceeds this number, this process will exit without trying to
  /// find any new tracks.  This is to prevent slowdown or crashes
  /// when noise causes a huge number of detections to appear on a few
  /// frames.
  unsigned max_assignment_size_;

  /// How often we should actually try to initialize
  double init_interval_;

  /// The last time we performed a track initialization.
  vidtk::timestamp last_time_;

  /// Don't initialize tracks whose speed is greater than this (-1 to disable)
  double init_max_speed_filter_;

  /// Don't initialize tracks whose speed is smaller than this (-1 to disable)
  double init_min_speed_filter_;


  buffer< vcl_vector< image_object_sptr > > const * mod_buf_;

  buffer< timestamp > const * timestamp_buf_;

  multiple_features const * mf_params_;

  vcl_vector<track_sptr> tracks_;

  vnl_double_2x2 spatial_cov_;
  image_object::float3d_type curr_loc_;
  vnl_double_3 line_dir_;

  // In case we want to only create tracks moving in a certain direction
  bool directional_prior_disabled_;
  vnl_double_2 directional_prior_;
  vnl_double_3 directional_prior_wld_;
  double min_directional_similarity_;

  double init_bt_norm_ris_val_;
  unsigned init_look_back_;

  double max_query_radius_;
  double min_area_similarity_;
  double min_color_similarity_;


#ifdef TEST_INIT
  unsigned next_track_id_;
#endif

  buffer< vgl_h_matrix_2d<double> > const * wld2img_H_buf_;
};

} // end namespace vidtk


#endif // vidtk_track_initializer_process_h_
