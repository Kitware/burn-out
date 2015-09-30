/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_fg_matcher_csurf_h_
#define vidtk_fg_matcher_csurf_h_

/// This fg_matcher class wrap's CSURF foreground tracking algorithm currently
/// in Matlab.

#include <tracking/fg_matcher.h>
#include <tracking/track.h>
#include <process_framework/external_proxy_process.h>
#include <utilities/config_block.h>
#include <utilities/timestamp.h>

#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

struct matlab_config
{
  double dflag;      // debug flag. 1: print debug info, 0: silent (needed for display/debug)
  double show;       // window display flag. 1: show window, 0: don't show (needed for display/debug)
  double step,scale, matchratio, scenex, sceney, relx, rely, predictionlength, searchwx, searchwy;
  double lostnumdescriptors, lostlowthreshold, losthighthreshold, updatethreshold, stretch;
};

namespace vidtk
{

struct fg_matcher_csurf_params
{
  fg_matcher_csurf_params()
    : padding_factor_( 0.0 )
  {
    config_.add_parameter( "padding_factor", "1.0",
      "Search radius around the predicted box. 1.0 means 100% of the template"
      "box size." );

    config_.add_parameter( "dflag", "false",
      "print debug info" );

    config_.add_parameter( "show", "false",
      "window display flag. 1: show window, 0: don't show (needed for display/debug)" );

    // these are GOOD parameters for Argus, don't change them unless you
    // absolutely know what you are doing.
    config_.add_parameter( "step", "3", "no desc" );
    config_.add_parameter( "scale", "2", "no desc" );
    config_.add_parameter( "matchratio", "0.85", "no desc" );

    // TODO: Short-circuit this parameter to tracking_sp::track_termination_duration_frames
    config_.add_parameter( "predictionlength", "6", 
      "A lower bound on track termination (in number of frames) condition. Matlab "
      "code will never label a track as terminated less than this number but it "
      "could be larger than this number based on another condition." );

    config_.add_parameter( "lostnumdescriptors", "4", "no desc" );
    config_.add_parameter( "lostlowthreshold", "3", "no desc" );
    config_.add_parameter( "losthighthreshold", "4", "no desc" );
    config_.add_parameter( "updatethreshold", "2.5", "no desc" );
    config_.add_parameter( "stretch", "0.005", "no desc" );
  }

  matlab_config mc_;
  double padding_factor_; // Not used yet.
  config_block config_;

}; // struct fg_matcher_csurf_params

/********************************************************************/

template < class PixType >
class fg_matcher_csurf
  : public fg_matcher< PixType >
{
public:
  typedef struct csurf_model model_type;

  fg_matcher_csurf();

  ~fg_matcher_csurf();

  static config_block params();

  virtual bool initialize();

  virtual bool set_params( config_block const& blk );

  virtual bool create_model( track_sptr track,
                             typename fg_matcher<PixType>::image_buffer_t const& img_buff,
                             timestamp const & curr_ts );

  virtual bool find_matches( track_sptr track,
                             vgl_box_2d<unsigned> const & predicted_bbox,
                             vil_image_view<PixType> const & curr_im,
                             vcl_vector< vgl_box_2d<unsigned> > &out_bboxes ) const;

  virtual bool update_model( track_sptr track,
                             typename fg_matcher<PixType>::image_buffer_t const & img_buff,
                             timestamp const & curr_ts );

  virtual bool cleanup_model( track_sptr track );

  virtual typename fg_matcher<PixType>::sptr_t deep_copy();

private:
  static fg_matcher_csurf_params params_;

  static process_smart_pointer< external_proxy_process > bridge_proc_;

  // Produced as a result of "conflict resolution" among multiple matches
  // and used when calling the update function.
  unsigned matched_index_; 

}; // class fg_matcher_csurf

}// namespace vidtk

#endif //fg_detector_csurf_h_
