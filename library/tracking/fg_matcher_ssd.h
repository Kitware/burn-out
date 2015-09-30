/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_fg_matcher_ssd_h_
#define vidtk_fg_matcher_ssd_h_

///To detect foreground objects.  The is an attempt to seperate foreground
///tracking from AMHI so one can do forground tracking with out doing AMHI.
///This could also be useful in track linking.

#include <tracking/amhi_data.h>
#include <tracking/fg_matcher.h>
#include <tracking/track.h>
#include <utilities/config_block.h>
#include <utilities/timestamp.h>
#include <utilities/paired_buffer.h>

#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

namespace vidtk
{

struct fg_matcher_ssd_params
{
public:
  fg_matcher_ssd_params()
  : use_weights_( false ),
    max_bbox_side_in_pixels_( 0 ),
    max_frac_valley_width_( 0.0 ),
    min_frac_valley_depth_( 0.0 ),
    min_valley_depth_( 2000.),
    padding_factor_( 0.0 ),
    max_dist_sqr_( 625 ),
    disable_multi_min_fg_( false ),
    amhi_mode_( false ),
    update_algo_( UPDATE_INVALID ),
    max_update_accel_( 0.0 ),
    max_update_narea_( 0.0 )
  {
    config_.add_parameter( "max_dist", "25",
      "Threshold value to decide fg match. [0,255]." );

    config_.add_parameter( "padding_factor", "1.0",
      "Search radius around the predicted box. 1.0 means 100% of the template"
      "box size." );

    config_.add_parameter( "use_weights", "true",
      "Uses weights when calculating visual match. If amhi is enabled, amhi"
      "weight will be used." );

    config_.add_parameter( "max_bbox_side_in_pixels", "200",
      "The maximum size of the bounding box used." );

    config_.add_parameter( "max_frac_valley_width", "0.4",
      "e.g. 0.4 value will say that there will be at the most 40% of the"
      " pixels of the ssd surface which will form a valley. 0.4 percentile"
      "in the spatial range." );

    config_.add_parameter( "min_frac_valley_depth", "0.2",
      "e.g. 0.2 percentile in the dynamic range." );

    config_.add_parameter( "min_valley_depth", "2000",
      "The minimum depth of the ssd valley calculated by the min point and"
      " the average of the four corners." );

    config_.add_parameter( "disable_multi_min_foreground", "false",
      "Disables the dectection of several minima in the ssd image." );

    config_.add_parameter( "amhi_mode", "false",
      "Whether to use image and weight patch from AMHI model or previous "
      "box in the track. " );

    config_.add_parameter( "max_update_accel", "7",
      "Update the foreground model only if the acceleration magnitude"
      " is lower than this threshold.");

    config_.add_parameter( "max_update_narea", "0.5",
      "Update the foreground model only if the normalized change in area is"
      " lower than this threshold.");

    config_.add_parameter( "update_algo", "always",
      "Update the foreground model based on the specified algo: always"
      " or accel_area.");
  } // fg_matcher_ssd_params()

  config_block config_;

  bool amhi_mode_;

  bool use_weights_;   ///Use weights when applying ssd

  ///TODO: NEED COMMENT
  unsigned max_bbox_side_in_pixels_;
  double max_frac_valley_width_;
  double min_frac_valley_depth_;
  double min_valley_depth_;

  // A factor of bbox dimension by which the padding is added around
  // the given search (test) bbox
  double padding_factor_;

  // Threshold value to decide fg match. [0,255]^2.
  double max_dist_sqr_;

  bool disable_multi_min_fg_;

  enum update_algo{ UPDATE_INVALID = 0, UPDATE_EVERY_FRAME, UPDATE_ACCEL_AREA };
  update_algo update_algo_;

  double max_update_accel_;
  double max_update_narea_;
}; // struct fg_matcher_ssd_params

/********************************************************************/

template < class PixType >
class fg_matcher_ssd
  : public fg_matcher< PixType >
{
public:
  fg_matcher_ssd(){};

  static config_block params();

  virtual bool set_params( config_block const& blk );

  virtual bool initialize(){ return true; }

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

  virtual typename fg_matcher< PixType >::sptr_t deep_copy();

  void copy_model( vil_image_view< PixType > const& );

private:
  typedef unsigned corr_type;

  bool warp_bbox( vil_image_view<PixType> const& R_im,
                  vgl_box_2d<unsigned> const& R_bbox,
                  vil_image_view<PixType> const& T_im,
                  vil_image_view<corr_type> & corr_surf,
                  vgl_box_2d<unsigned> & Rpad_bbox,
                  vil_image_view<float> const& T_w = vil_image_view<float>() ) const;

  bool fg_bbox_update( vgl_box_2d<unsigned> const & T_bbox,
                       vgl_box_2d<unsigned> const & R_bbox,
                       vgl_box_2d<unsigned> const & Rpad_bbox,
                       vil_image_view<corr_type> const & corr_surf,
                       vgl_box_2d<unsigned> & out_bbox ) const;

  bool fg_bbox_update( vgl_box_2d<unsigned> const & T_bbox,
                       vgl_box_2d<unsigned> const & R_bbox,
                       vgl_box_2d<unsigned> const & Rpad_bbox,
                       unsigned int min_x,
                       unsigned int min_y,
                       vgl_box_2d<unsigned> & out_bbox ) const;

  void find_single_match( vgl_box_2d<unsigned> const & predicted_bbox,
                          vil_image_view<PixType> const & currIm,
                          vil_image_view<PixType> const & templ_im,
                          vil_image_view<float> const& templ_weight,
                          vgl_box_2d<unsigned> &out_bbox,
                          bool & match_found ) const;

  void find_multi_matches( vgl_box_2d<unsigned> const & predicted_bbox,
                           vil_image_view<PixType> const & currIm,
                           vil_image_view<PixType> const & templ_im,
                           vil_image_view<float> const& templ_weight,
                           vcl_vector< vgl_box_2d<unsigned> > &out_bboxes,
                           bool & match_found ) const;

  static fg_matcher_ssd_params params_;

  vil_image_view<PixType> model_image_;

  timestamp model_image_ts_;

}; // class fg_matcher_ssd

}// namespace vidtk

#endif //fg_detector_h_
