/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_to_frame_homography_process_h_
#define vidtk_frame_to_frame_homography_process_h_

#include <vcl_vector.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking/track.h>


namespace vidtk
{


class frame_to_frame_homography_process
  : public process
{
public:
  typedef frame_to_frame_homography_process self_type;

  frame_to_frame_homography_process( vcl_string const& name );

  ~frame_to_frame_homography_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_new_tracks( vcl_vector< track_sptr > const& trks );

  VIDTK_INPUT_PORT( set_new_tracks, vcl_vector< track_sptr > const& );

  void set_terminated_tracks( vcl_vector< track_sptr > const& trks );

  VIDTK_INPUT_PORT( set_terminated_tracks, vcl_vector< track_sptr > const& );

  bool homography_is_valid() const;

  VIDTK_OUTPUT_PORT( bool, homography_is_valid );

  vgl_h_matrix_2d<double> const& forward_homography() const;

  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, forward_homography );

  vgl_h_matrix_2d<double> const& backward_homography() const;

  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, backward_homography );

public:
  struct extra_info_type
  {
    // The world location of the "start" of this track.
    vnl_vector_fixed<double,3> world_loc_;

    // Do we think that this is tracking a feature that is static and
    // on the ground plane, or tracking something "bad"?
    bool good_;
  };

private:
  /// Refine the estimate to minimize the geometric error.
  void lmq_refine_homography();

  config_block config_;

  bool refine_geometric_error_;
  unsigned delta_;
  bool disabled_;

  // state

  // Map from the tracked KLT features to extra stuff used for
  // homography estimation
  typedef vcl_map< track_sptr, extra_info_type > state_map_type;
  state_map_type tracks_;

  bool homog_is_valid_;
  vgl_h_matrix_2d<double> forw_H_;
  vgl_h_matrix_2d<double> back_H_;
};


} // end namespace vidtk


#endif // vidtk_frame_to_frame_homography_process_h_
