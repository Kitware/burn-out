/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_amhi_h_
#define vidtk_amhi_h_

#include <vil/vil_image_view.h>
#include <vcl_vector.h>

#include <utilities/buffer.h>
#include <tracking/track.h>
#include <tracking/image_object.h>

#include <vbl/vbl_ref_count.h>

namespace vidtk
{

/// \brief Aligned Motion History Image (AMHI) for robust appearance 
/// modelling.
///
/// The main idea is to use a collection of partial observations from
/// an moving foreground object and "merge" them together to build a 
/// better appearance model which should be better than a mere union of
/// these observations. 

template < class PixType >
class amhi
  : public vbl_ref_count
{
  //  typedef float corr_type;
  typedef unsigned corr_type;

public:
  amhi();
  
  amhi( double alpha,
        double padding, 
        double weight_bbox,
        double almost_zero,
        double max_frac_valley_width,
        double max_frac_valley_depth,
        bool use_weights,
        unsigned max_bbox_side );

  ~amhi(){};

  /// \brief Generates an AMHI model for the intial set of observations
  /// in the track. 
  ///
  /// Creates a new smart pointer aMHI for the current track, the control
  /// for which is passed to track class.
  ///


  //Creates initial amhi model using a group of corresponding MODs. 
  void gen_init_model( vbl_smart_ptr<track> trk, 
                       buffer< vil_image_view<PixType> > const* src_img_buff);

  //Updates an existing amhi model. Perform ssd of an existing amhi_im 
  //around the obj in the current image. Overwrites amhi_weight, amhi_im,
  //and amhi_histogram.
  void update_track_model(  image_object const & obj, 
                            vil_image_view<vxl_byte> const & im, 
                            track & trk);  

  void fg_match( vgl_box_2d<unsigned> const & curr_bbox, 
                  vil_image_view<vxl_byte> const & currIm, 
                  amhi_data const& curr_dat,
                  vgl_box_2d<unsigned> &out_bbox,
                  bool & match_found, 
                  unsigned max_dist_sqr,
                  double fg_padding_factor );

  static void get_tighter_bbox( vil_image_view<float> M, 
                                float threshold,
                                vgl_box_2d<unsigned> & bbox, 
                                vgl_box_2d<unsigned> const & amhi_box );

private:
  bool warp_bbox( vil_image_view<PixType> const& R_im, 
                  vgl_box_2d<unsigned> const& R_bbox, 
                  vil_image_view<PixType> const& T_im, 
                  vgl_box_2d<unsigned> const & T_bbox,
                  vil_image_view<corr_type> & corr_surf,
                  double padding_factor );

  bool warp_bbox( vil_image_view<PixType> const& R_im, 
                  vgl_box_2d<unsigned> const& R_bbox, 
                  vil_image_view<PixType> const& T_im, 
                  vil_image_view<float> const& T_w, 
                  vgl_box_2d<unsigned> const & T_bbox,
                  vil_image_view<corr_type> & corr_surf,
                  double padding_factor );

  void init_amhi_update(  vgl_box_2d<unsigned> & R_bbox, 
                          image_object_sptr const T_obj,
                          vgl_box_2d<unsigned> & aT_bbox,
                          vil_image_view<corr_type> const & corr_surf,
                          vil_image_view<float> & curr_amhi);

  void online_amhi_update(  vgl_box_2d<unsigned> & T_bbox, 
                            image_object const & R_obj,
                            vil_image_view<corr_type> const & corr_surf,
                            vil_image_view< float > const & curr_amhi,
                            vgl_box_2d<unsigned> & new_bbox2,
                            vil_image_view<float> & new_amhi2);

  void remove_almost_zeros( vil_image_view<float> &im, 
                            vgl_box_2d<unsigned> &box);

  void fg_bbox_update(  vgl_box_2d<unsigned> const & T_bbox, 
                        vgl_box_2d<unsigned> const & R_bbox,
                        vil_image_view<corr_type> const & corr_surf,
                        vgl_box_2d<unsigned> & out_bbox );

  //Config parameters
  //init & online 
  double amhi_alpha_;
  double padding_factor_;
  unsigned max_bbox_side_in_pixels_;

  //online only 
  double w_bbox_online_;
  double remove_zero_thr_;
  double max_frac_valley_width_;
  double min_frac_valley_depth_;

  //online & fg_match only 
  bool use_weights_;


  //temp variables used for size transfer between local functions
  vgl_box_2d<unsigned> Rpad_bbox_;
  

  //Variables for debugging output into text file.
  timestamp ts_;

}; // class amhi

} // namespace vidtk

#endif //vidtk_amhi_h_
