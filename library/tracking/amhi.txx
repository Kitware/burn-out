/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "amhi.h"

#include <vcl_iostream.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_vector.h>

#include <vil/vil_image_view.h>
#include <vil/vil_math.h>
#include <vil/vil_crop.h>
#include <vil/algo/vil_correlate_2d.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_point_2d.h>

#include <tracking/image_object.h>
#include <tracking/tracking_keys.h>
#include <video/ssd.h>
#include <utilities/log.h>

#include <vil/vil_save.h>
#include <vil/algo/vil_threshold.h>

namespace vidtk
{

//TODO: Currently online updates to amhi are done through 
//      tracker. That uses this empty constructor. These 
//      parameter values needs to be loaded 
//      from the tracker config or a new process.

template < class PixType >
amhi<PixType>
::amhi()
: amhi_alpha_( 0.1 ),
  padding_factor_( 0.5 ),
  max_bbox_side_in_pixels_( 200 ),
  w_bbox_online_( 0.2 ),
  remove_zero_thr_( 0.001 ),
  max_frac_valley_width_( 0.3 ),
  min_frac_valley_depth_( 0.2 ),
  use_weights_( false )
{
}

template < class PixType >
amhi<PixType>
::amhi( double alpha,
        double padding, 
        double weight_bbox,
        double almost_zero,
        double max_frac_valley_width,
        double min_frac_valley_depth,
        bool use_weights,
        unsigned max_bbox_side )
: amhi_alpha_( alpha ),
  padding_factor_( padding ),
  max_bbox_side_in_pixels_( max_bbox_side ),
  w_bbox_online_( weight_bbox ),
  remove_zero_thr_( almost_zero ),
  max_frac_valley_width_( max_frac_valley_width ),
  min_frac_valley_depth_( min_frac_valley_depth ),
  use_weights_( use_weights )
{
}

/********************************************************************/

template < class PixType> 
void
amhi<PixType>
::gen_init_model( track_sptr trk, 
                  buffer< vil_image_view<PixType> > const* src_img_buff )
{
  vcl_vector< track_state_sptr > trkHist = trk->history();  
  image_object_sptr curr_obj = NULL;
  image_object_sptr prev_obj = NULL;
  vil_image_view< float > new_amhi;
  vgl_box_2d<unsigned> new_bbox;
  unsigned trkLen = trkHist.size();

  //Initialize amhi to latest frame. 
  curr_obj = trk->get_end_object();
  if( curr_obj == NULL )
  {
    log_error( "No MOD found in the current frame.\n" );
    return;
  }
  vgl_box_2d<unsigned> aT_bbox;
  vgl_box_2d<unsigned> R_in_prev;
  vgl_box_2d<unsigned> R_bbox = curr_obj->bbox_;
  new_amhi.set_size( curr_obj->mask_.ni(),curr_obj->mask_.nj() ); 

  vil_math_add_image_fraction( new_amhi, (float) 0, curr_obj->mask_, (float) 1);
  vil_image_view<PixType> const &currIm = src_img_buff->datum_at( 0 );
  vil_image_view< corr_type > corr_surf;

  vcl_vector<vgl_box_2d<unsigned> > Rs_in_prev(trkLen);

  unsigned ghost_count = 0;

  // For every observation in the track.
  for(unsigned i = 0; i < trkLen-1; i++ )
  {
    // 1. Access the bounding box     
    prev_obj = trk->get_object( i );
    if( prev_obj == NULL )
      continue;

    if(  R_bbox.width() > max_bbox_side_in_pixels_
      || R_bbox.height() > max_bbox_side_in_pixels_ 
      || prev_obj->bbox_.width() > max_bbox_side_in_pixels_ 
      || prev_obj->bbox_.height() > max_bbox_side_in_pixels_ ) 
    {
      vcl_cout<<"amhi model init:"
              <<" bailing out for optimization,"
              <<prev_obj->bbox_.width()<<"x"<<prev_obj->bbox_.height() 
              <<" -> "
              <<R_bbox.width()<<"x"<<R_bbox.height() 
              <<vcl_endl;
      continue;
    }

    vil_image_view<PixType> const &prevIm = src_img_buff->datum_at( 
      trkLen-1 - i);

    //TODO: Check for all 0's in new_amhi

    //R_bbox represents the bbox corresponding to the current 'new_amhi'
    //model.
    assert( R_bbox.width() == new_amhi.ni() &&
            R_bbox.height() == new_amhi.nj() );

    bool result; 
    //TODO: First push the 'template' of mask/weight through to ssd functions.
    //if( use_weights_ )
    //{
    //  result = warp_bbox( currIm, R_bbox, prevIm, prev_obj->mask_ , prev_obj->bbox_, corr_surf); 
    //}
    //else
    //{
      result = warp_bbox( currIm, 
                          R_bbox, 
                          prevIm, 
                          prev_obj->bbox_, 
                          corr_surf,
                          padding_factor_ );
    //}

    if( result )
    { 
      ghost_count += (unsigned) detect_ghost( corr_surf, 
                                              max_bbox_side_in_pixels_, 
                                              max_frac_valley_width_,
                                              min_frac_valley_depth_ );

      init_amhi_update( R_bbox, prev_obj, aT_bbox, corr_surf, new_amhi ); 
    }
    else
    {
      continue;
    }

    //For saving the model in the initial frames. 
    R_in_prev = R_bbox; //for copying width/height

    int new_cent_x = prev_obj->bbox_.centroid_x() + ( R_bbox.centroid_x() - aT_bbox.centroid_x());
    int new_cent_y = prev_obj->bbox_.centroid_y() + ( R_bbox.centroid_y() - aT_bbox.centroid_y());
    //Handle the image boundaries
    R_in_prev.set_min_x((unsigned) vcl_max(0.0, new_cent_x - vcl_floor( R_bbox.width()/2.0 ) ));
    R_in_prev.set_min_y((unsigned) vcl_max(0.0, new_cent_y - vcl_floor(R_bbox.height()/2.0) )); 
    R_in_prev.set_max_x((unsigned) vcl_min((double) currIm.ni(), new_cent_x + vcl_ceil(R_bbox.width()/2.0) ));
    R_in_prev.set_max_y((unsigned) vcl_min((double) currIm.nj(), new_cent_y + vcl_ceil(R_bbox.height()/2.0) ));

    Rs_in_prev[i] = R_in_prev; 
  }

  //current frame!
  Rs_in_prev[trkLen-1] = R_bbox; 

#ifdef PRINT_DEBUG_INFO
  vcl_cout<<"Computed AMHI for track length: "<<trkLen<<"\n";
#endif

  //Provide trk with both weight and image. Used for updating the track
  //appearance model. Appearance models other than histogram can be 
  //derived from this.
  amhi_data new_amhi_dat;

  new_amhi_dat.ghost_count = ghost_count;

  new_amhi_dat.weight = new_amhi;

  new_amhi_dat.image = vil_crop(  currIm, 
                                  R_bbox.min_x(), 
                                  R_bbox.width(), 
                                  R_bbox.min_y(), 
                                  R_bbox.height() ); 
  new_amhi_dat.bbox = R_bbox;

  //performs deep copy of the images and histgoram.
  trk->set_amhi_datum( new_amhi_dat );

  //propagate final new_amhi_dat.bbox size back into the init frames. 
  for(unsigned i = 0; i < trkLen; i++)
  {
    vgl_box_2d<unsigned> &bbox = Rs_in_prev[i];

    if( bbox.width() < 1 || bbox.height() < 1 )
      continue;

    //Using uniform size for all the bboxes. 

    //Handle the image boundaries
    bbox.set_min_x((unsigned) vcl_max(0.0, bbox.centroid_x() - vcl_floor(R_bbox.width()/2.0 ) ));
    bbox.set_min_y((unsigned) vcl_max(0.0, bbox.centroid_y() - vcl_floor(R_bbox.height()/2.0) )); 
    bbox.set_max_x((unsigned) vcl_min((double) currIm.ni(), bbox.centroid_x() + vcl_ceil(R_bbox.width()/2.0) ));
    bbox.set_max_y((unsigned) vcl_min((double) currIm.nj(), bbox.centroid_y() + vcl_ceil(R_bbox.height()/2.0) ));

    //bbox.set_width( R_bbox.width() );
    //bbox.set_height( R_bbox.height() );

    trk->set_init_amhi_bbox(i, bbox);
  }
}



/********************************************************************/
template<class PixType>
void 
amhi<PixType>
::init_amhi_update( vgl_box_2d<unsigned> & R_bbox, 
                    image_object_sptr const T_obj,
                    vgl_box_2d<unsigned> & aT_bbox,
                    vil_image_view<corr_type> const & corr_surf,
                    vil_image_view< float > & curr_amhi)
{
  unsigned min_i, min_j;
  find_local_minimum(corr_surf, min_i, min_j);
  vgl_point_2d<unsigned> tmp_pt;

  vgl_box_2d<unsigned> T_bbox = T_obj->bbox_; 

  //copy into new_amhi
  //new size of amhi by combining R_bbox and T_bbox offset after correl
  vgl_box_2d<unsigned> rel_T_bbox = T_bbox;
  tmp_pt.set( min_i + T_bbox.width()/2 , min_j + T_bbox.height()/2 );
  rel_T_bbox.set_centroid( tmp_pt );

  assert( R_bbox.min_x() >= Rpad_bbox_.min_x() &&
          R_bbox.min_y() >= Rpad_bbox_.min_y() );
  vgl_box_2d<unsigned> rel_R_bbox = R_bbox;
  tmp_pt.set( R_bbox.centroid_x() - Rpad_bbox_.min_x(),
              R_bbox.centroid_y() - Rpad_bbox_.min_y());
  rel_R_bbox.set_centroid( tmp_pt );

  //Storing aT_bbox for the sake of debugging. T_bbox aligned with R_bbox in the 
  //image coordinates for displaying. 
  //Also used for backprojecting amhi into source frames. 
  aT_bbox = T_bbox;
  aT_bbox.set_centroid( R_bbox.centroid() + ( rel_T_bbox.centroid() - 
                                              rel_R_bbox.centroid() ) );

  //After having translated T & R into Rpad, prepare the min bbox
  //used as the size of the new amhi size.
  vgl_box_2d<unsigned> new_bbox;
  new_bbox.add( rel_T_bbox );
  new_bbox.add( rel_R_bbox );

  //update rel_T & rel_R into min bbox frame of ref. 
  tmp_pt.set( rel_R_bbox.centroid_x() - new_bbox.min_x(),
              rel_R_bbox.centroid_y() - new_bbox.min_y() );
  rel_R_bbox.set_centroid( tmp_pt );

  tmp_pt.set( rel_T_bbox.centroid_x() - new_bbox.min_x(),
              rel_T_bbox.centroid_y() - new_bbox.min_y() );
  rel_T_bbox.set_centroid( tmp_pt );

  vil_image_view<float> new_amhi( new_bbox.width(), new_bbox.height() );
  new_amhi.fill( 0 );

  assert( curr_amhi.ni() == rel_R_bbox.width() && 
          curr_amhi.nj() == rel_R_bbox.height() );

  //Incorporate R (prev amhi) into new amhi
  vil_image_view<float> amhi_R_view = vil_crop( new_amhi, 
                                                rel_R_bbox.min_x(), 
                                                rel_R_bbox.width(), 
                                                rel_R_bbox.min_y(), 
                                                rel_R_bbox.height() );
  amhi_R_view.deep_copy( curr_amhi );

  //new_amhi =  "(1-alpha) * curr_amhi" + alpha * binary_mask
  vil_math_scale_values( new_amhi, 1 - amhi_alpha_ );

  assert( T_obj->mask_.ni() == rel_T_bbox.width() && 
          T_obj->mask_.nj() == rel_T_bbox.height() );

  //Incorporate T (new mask) into new amhi
  vil_image_view<float> amhi_T_view = vil_crop( new_amhi, 
                                                rel_T_bbox.min_x(),
                                                rel_T_bbox.width(), 
                                                rel_T_bbox.min_y(), 
                                                rel_T_bbox.height() );

  //new_amhi =  (1-alpha) * curr_amhi + "alpha * binary_mask"
  vil_math_add_image_fraction( amhi_T_view, (float) 1, T_obj->mask_, 
                                              (float) amhi_alpha_ );

  curr_amhi = new_amhi;

  tmp_pt.set( new_bbox.centroid_x() + Rpad_bbox_.min_x(),
              new_bbox.centroid_y() + Rpad_bbox_.min_y() );
  new_bbox.set_centroid( tmp_pt );

  R_bbox = new_bbox;
}
/********************************************************************/

//Assumption: Input R_bbox and curr_amhi are supposed to have the same size. 
template < class PixType >
bool
amhi<PixType>
::warp_bbox(vil_image_view<PixType> const& R_im, 
            vgl_box_2d<unsigned> const& R_bbox, 
            vil_image_view<PixType> const& T_im, 
            vgl_box_2d<unsigned> const & T_bbox,
            vil_image_view<corr_type> & corr_surf,
            double padding_factor )
{
  //Prepare 'kernel'/(T)emplate , which is taken as the size of the 
  //bbox in the previous frame.
  vil_image_view<PixType> T_chip = vil_crop( T_im,
                                             T_bbox.min_x(), 
                                             T_bbox.width(),
                                             T_bbox.min_y(),
                                             T_bbox.height() );

  //Prepare input image with R_bbox and 'extra' area around it for 
  //the sake of correlation.
  unsigned padded_W_ = (unsigned) vcl_floor( T_bbox.width()*padding_factor  );
  unsigned padded_H_ = (unsigned) vcl_floor( T_bbox.height()*padding_factor );

  Rpad_bbox_.empty();
  vgl_point_2d<unsigned> tmp_pt;
  tmp_pt.set( vcl_max(0, (int) (R_bbox.min_x() - padded_W_ )),
              vcl_max(0, (int) (R_bbox.min_y() - padded_H_ )) );
  Rpad_bbox_.set_min_point( tmp_pt );

  tmp_pt.set( vcl_min( R_im.ni()-1, R_bbox.max_x() + padded_W_),
              vcl_min( R_im.nj()-1, R_bbox.max_y() + padded_H_) );
  Rpad_bbox_.set_max_point( tmp_pt );

  vil_image_view<PixType> Rpad_chip = vil_crop( R_im, 
                                                Rpad_bbox_.min_x(),
                                                Rpad_bbox_.width(), 
                                                Rpad_bbox_.min_y(), 
                                                Rpad_bbox_.height() );

  //The output image containing the correlation values.   

  //  corr_type float_placeholder = (float) 0;
  //  vil_correlate_2d(Rpad_chip, corr_surf, T_chip, float_placeholder);

  return get_ssd_surf(Rpad_chip, corr_surf, T_chip);
}

/********************************************************************/

//Assumption: Input R_bbox and curr_amhi are supposed to have the same size. 
template < class PixType >
bool
amhi<PixType>
::warp_bbox(vil_image_view<PixType> const& R_im, 
            vgl_box_2d<unsigned> const& R_bbox, 
            vil_image_view<PixType> const& T_im, 
            vil_image_view<float> const& T_w, 
            vgl_box_2d<unsigned> const & T_bbox,
            vil_image_view<corr_type> & corr_surf,
            double padding_factor )
{
  //Prepare 'kernel'/(T)emplate , which is taken as the size of the 
  //bbox in the previous frame.
  vil_image_view<PixType> T_chip = vil_crop( T_im,
                                             T_bbox.min_x(), 
                                             T_bbox.width(),
                                             T_bbox.min_y(),
                                             T_bbox.height() );

  //Prepare input image with R_bbox and 'extra' area around it for 
  //the sake of correlation.
  unsigned padded_W_ = (unsigned) vcl_floor( T_bbox.width() * padding_factor );
  unsigned padded_H_ = (unsigned) vcl_floor( T_bbox.height() * padding_factor);

  Rpad_bbox_.empty();
  vgl_point_2d<unsigned> tmp_pt;
  tmp_pt.set( vcl_max(0, (int) (R_bbox.min_x() - padded_W_ )),
              vcl_max(0, (int) (R_bbox.min_y() - padded_H_ )) );
  Rpad_bbox_.set_min_point( tmp_pt );

  tmp_pt.set( vcl_min( R_im.ni()-1, R_bbox.max_x() + padded_W_),
              vcl_min( R_im.nj()-1, R_bbox.max_y() + padded_H_) );
  Rpad_bbox_.set_max_point( tmp_pt );

  vil_image_view<PixType> Rpad_chip = vil_crop( R_im, 
                                                Rpad_bbox_.min_x(),
                                                Rpad_bbox_.width(), 
                                                Rpad_bbox_.min_y(), 
                                                Rpad_bbox_.height() );

  //The output image containing the correlation values.   

  //  corr_type float_placeholder = (float) 0;
  //  vil_correlate_2d(Rpad_chip, corr_surf, T_chip, float_placeholder);

  return get_ssd_surf(Rpad_chip, corr_surf, T_chip, T_w);
}

/********************************************************************/

template <class PixType>
void 
amhi<PixType>
::update_track_model(image_object const & curr_obj, 
                    vil_image_view<vxl_byte> const & currIm, 
                    track & trk)
{
  vgl_box_2d<unsigned> curr_bbox = curr_obj.bbox_;
  amhi_data const& curr_dat = trk.amhi_datum();
  vgl_box_2d<unsigned> amhi_bbox(0, curr_dat.bbox.width(),
                                 0, curr_dat.bbox.height());

  vil_image_view< corr_type > corr_surf;
  amhi_data amhi_dat;
  vgl_box_2d<unsigned> &out_bbox = amhi_dat.bbox;

  if(  amhi_bbox.width() > max_bbox_side_in_pixels_
    || amhi_bbox.height() > max_bbox_side_in_pixels_ 
    || amhi_bbox.width() == 0
    || amhi_bbox.height() == 0 
    || curr_bbox.width() > max_bbox_side_in_pixels_ 
    || curr_bbox.height() > max_bbox_side_in_pixels_ ) 
  {
    vcl_cout<<"amhi model update: track : "<<trk.id()
            <<", bailing out for optimization."
            <<amhi_bbox.width()<<"x"<<amhi_bbox.height() 
            <<" -> "
            <<curr_bbox.width()<<"x"<<curr_bbox.height() 
            <<vcl_endl;
    return;
  }

  //NOTE: R & T are switched in this case as opposed to intialization. 
  //(T)emplate is the current amhi model and (R)eference image is the
  //detection object in the current image. 

  bool result;
  if( use_weights_ )
  {
    result = warp_bbox( currIm, 
                         curr_bbox, 
                         curr_dat.image, 
                         curr_dat.weight, 
                         amhi_bbox, 
                         corr_surf,
                         padding_factor_ );
  }
  else
  {
    result = warp_bbox( currIm, 
                         curr_bbox, 
                         curr_dat.image, 
                         amhi_bbox, 
                         corr_surf,
                         padding_factor_ );
  }

  if( result )
  {
    amhi_dat.ghost_count = (unsigned) detect_ghost( corr_surf, 
                                                     max_bbox_side_in_pixels_, 
                                                     max_frac_valley_width_,
                                                     min_frac_valley_depth_ );

    online_amhi_update( amhi_bbox, 
                        curr_obj, 
                        corr_surf, 
                        curr_dat.weight, 
                        out_bbox, 
                        amhi_dat.weight ); 

    //Provide trk with both weight and image. Used for updating the track
    //appearance model. Appearance models other than histogram can be 
    //derived from this.
    amhi_dat.image = vil_crop( currIm, 
                               out_bbox.min_x(), 
                               out_bbox.width(), 
                               out_bbox.min_y(), 
                               out_bbox.height() );

    trk.set_amhi_datum( amhi_dat );
  }    
}


/********************************************************************/
/*
Using a different version of intit_amhi_update function because of two reasons:
1. The direction of amhi->obj is reversed. 
2. We want to update (size and weight of) amhi conservatively in the online version. 
*/
template<class PixType>
void 
amhi<PixType>
::online_amhi_update( vgl_box_2d<unsigned> & T_bbox, 
                      image_object const & R_obj,
                      vil_image_view<corr_type> const & corr_surf,
                      vil_image_view< float > const & curr_amhi,
                      vgl_box_2d<unsigned> & new_bbox2,
                      vil_image_view<float> & new_amhi2 )
{
  unsigned min_i, min_j;
  find_local_minimum(corr_surf, min_i, min_j);
  vgl_point_2d<unsigned> tmp_pt;

  vgl_box_2d<unsigned> R_bbox = R_obj.bbox_; 

  //rel_* is in ref to Rpad_*
  //T_bbox to correlation peak. 
  vgl_box_2d<unsigned> rel_T_bbox = T_bbox;
  tmp_pt.set( min_i + T_bbox.width()/2 , 
              min_j + T_bbox.height()/2 );
  rel_T_bbox.set_centroid( tmp_pt );

  assert( R_bbox.min_x() >= Rpad_bbox_.min_x() &&
          R_bbox.min_y() >= Rpad_bbox_.min_y() );
  
  vgl_box_2d<unsigned> rel_R_bbox = R_bbox;
  tmp_pt.set( R_bbox.centroid_x() - Rpad_bbox_.min_x(),
              R_bbox.centroid_y() - Rpad_bbox_.min_y());
  rel_R_bbox.set_centroid( tmp_pt );

  //Storing aT_bbox for the sake of debugging. T_bbox aligned with R_bbox 
  //in the image coordinates for displaying. 
  vgl_box_2d<unsigned> aT_bbox = T_bbox;
  aT_bbox.set_centroid( R_bbox.centroid() + 
                      ( rel_T_bbox.centroid() - rel_R_bbox.centroid() ) );

  //After having translated T & R into Rpad, prepare the min bbox
  //used as the size of the new amhi size.
  vgl_box_2d<unsigned> new_bbox;
  new_bbox.add( rel_T_bbox );//original amhi
  new_bbox.add( rel_R_bbox );

  //update rel_T & rel_R into min bbox frame of ref. 
  tmp_pt.set( rel_R_bbox.centroid_x() - new_bbox.min_x(),
              rel_R_bbox.centroid_y() - new_bbox.min_y() );
  rel_R_bbox.set_centroid( tmp_pt );

  tmp_pt.set( rel_T_bbox.centroid_x() - new_bbox.min_x(),
              rel_T_bbox.centroid_y() - new_bbox.min_y() );
  rel_T_bbox.set_centroid( tmp_pt );

  vil_image_view<float> new_amhi( new_bbox.width(), new_bbox.height() );
  new_amhi.fill( 0 );

  assert( curr_amhi.ni() == rel_T_bbox.width() && 
          curr_amhi.nj() == rel_T_bbox.height() );

  //Incorporate T (current amhi) into new amhi
  vil_image_view<float> amhi_T_view = vil_crop( new_amhi, 
                                                rel_T_bbox.min_x(), 
                                                rel_T_bbox.width(), 
                                                rel_T_bbox.min_y(), 
                                                rel_T_bbox.height() );

  amhi_T_view.deep_copy( curr_amhi );

  vil_math_scale_values( new_amhi, 1 - amhi_alpha_ );

  assert( R_obj.mask_.ni() == rel_R_bbox.width() && 
          R_obj.mask_.nj() == rel_R_bbox.height() );

  //Incorporate T (new mask) into new amhi
  vil_image_view<float> amhi_R_view = vil_crop( new_amhi, 
                                                rel_R_bbox.min_x(), 
                                                rel_R_bbox.width(), 
                                                rel_R_bbox.min_y(), 
                                                rel_R_bbox.height() );

  //new_amhi = alpha * binary_mask + (1-alpha) * curr_amhi
  vil_math_add_image_fraction( amhi_R_view, (float) 1, R_obj.mask_, 
                                            (float) amhi_alpha_ );


  //Remove values ~ 0, for reduction in amhi size.
  remove_almost_zeros(new_amhi, rel_T_bbox);
    
  new_bbox2.empty();
  new_bbox2.add( rel_T_bbox );//original amhi

  //Taking a weighted mean of rel_T_bbox and rel_R_bbox. We want to discourage
  //the sudden 'jump' in the size of the bbox.   
  if(w_bbox_online_ > 0)//just for optimisation
  {
    tmp_pt.set( (unsigned)( rel_T_bbox.min_x() * (1-w_bbox_online_) +
                            rel_R_bbox.min_x() *    w_bbox_online_ ),
                (unsigned)( rel_T_bbox.min_y() * (1-w_bbox_online_) + 
                            rel_R_bbox.min_y() *    w_bbox_online_ ) );
    new_bbox2.add( tmp_pt );

    tmp_pt.set( (unsigned)(rel_T_bbox.max_x() * (1-w_bbox_online_) 
                         + rel_R_bbox.max_x() *    w_bbox_online_),
                (unsigned)(rel_T_bbox.max_y() * (1-w_bbox_online_) 
                         + rel_R_bbox.max_y() *    w_bbox_online_));
    new_bbox2.add( tmp_pt );
  }

  //output 1
  new_amhi2 = vil_crop( new_amhi, 
                        new_bbox2.min_x(), 
                        new_bbox2.width(),
                        new_bbox2.min_y(), 
                        new_bbox2.height() );

  //output 2
  //newbbox2 --> newbbox --> Rpad ref.
  tmp_pt.set( new_bbox2.centroid_x() + new_bbox.min_x() + Rpad_bbox_.min_x(),
              new_bbox2.centroid_y() + new_bbox.min_y() + Rpad_bbox_.min_y());
  new_bbox2.set_centroid( tmp_pt );
}

/********************************************************************/

template<class PixType>
void 
amhi<PixType>
::remove_almost_zeros(vil_image_view<float> &image, 
                      vgl_box_2d<unsigned> &box)
{
  vgl_box_2d<unsigned> orgbox = box;
  box.empty();
  
  for (unsigned j= orgbox.min_y() ; j < orgbox.max_y()  ; ++j )
  {
    for (unsigned i = orgbox.min_x() ; i < orgbox.max_x() ; ++i ) 
    {
      if( image(i,j) > remove_zero_thr_ )
        box.add( vgl_point_2d<unsigned>(i,j) );
    }
  }

  //because we always keep extra padding in bbox 
  //w.r.t to image. 
  box.set_max_x( box.max_x() + 1 );
  box.set_max_y( box.max_y() + 1 );
}

/********************************************************************/

template <class PixType>
void 
amhi<PixType>
::fg_match( vgl_box_2d<unsigned> const & curr_bbox, 
            vil_image_view<vxl_byte> const & currIm, 
            amhi_data const& curr_dat,
            vgl_box_2d<unsigned> &out_bbox,
            bool & match_found, 
            unsigned max_dist_sqr,
            double fg_padding_factor )
{
  match_found = false;

  vgl_box_2d<unsigned> amhi_bbox(0, curr_dat.bbox.width(),
                                 0, curr_dat.bbox.height());

  vil_image_view< corr_type > corr_surf;

  if(  amhi_bbox.width() > max_bbox_side_in_pixels_
    || amhi_bbox.height() > max_bbox_side_in_pixels_ 
    || amhi_bbox.width() == 0
    || amhi_bbox.height() == 0 
    || curr_bbox.width() > max_bbox_side_in_pixels_ 
    || curr_bbox.height() > max_bbox_side_in_pixels_ ) 
  {
    vcl_cout<<"fg_match: bailing out for optimization."
            <<amhi_bbox.width()<<"x"<<amhi_bbox.height() 
            <<" -> "
            <<curr_bbox.width()<<"x"<<curr_bbox.height() 
            <<vcl_endl;
    return;
  }

  //NOTE: R & T are switched in this case as opposed to intialization. 
  //(T)emplate is the current amhi model and (R)eference image is the
  //detection object in the current image. 

  bool result;
  if( use_weights_ )
  {
    result = warp_bbox( currIm, 
                         curr_bbox, 
                         curr_dat.image, 
                         curr_dat.weight,
                         amhi_bbox, 
                         corr_surf,
                         fg_padding_factor );
  }
  else
  {
    result = warp_bbox( currIm, 
                         curr_bbox, 
                         curr_dat.image, 
                         amhi_bbox, 
                         corr_surf, 
                         fg_padding_factor );
  }

  if( result )
  {
    match_found = ! detect_ghost(corr_surf, 
                                 max_bbox_side_in_pixels_, 
                                 max_frac_valley_width_,
                                 min_frac_valley_depth_ );

    if( match_found )
    {
      fg_bbox_update( amhi_bbox, 
                      curr_bbox, 
                      corr_surf, 
                      out_bbox );
    }

    typename vil_image_view<const corr_type>::iterator iter;

    if( corr_surf.ni() < 2 || corr_surf.nj() < 2 ) 
    {
      return;
    }
    else
    {
      iter = vcl_min_element( corr_surf.begin(), corr_surf.end() );
    }

    match_found = match_found && (*iter < max_dist_sqr);
#ifdef PRINT_DEBUG_INFO
    vcl_cout<<*iter<<vcl_endl;
#endif
  }
}

/********************************************************************/

template<class PixType>
void 
amhi<PixType>
::fg_bbox_update( vgl_box_2d<unsigned> const & T_bbox, 
                  vgl_box_2d<unsigned> const & R_bbox,
                  vil_image_view<corr_type> const & corr_surf,
                  vgl_box_2d<unsigned> & out_bbox )
{
  unsigned min_i, min_j;
  find_local_minimum(corr_surf, min_i, min_j);
  vgl_point_2d<unsigned> tmp_pt;

  //rel_* is in ref to Rpad_*
  //T_bbox to correlation peak. 
  vgl_box_2d<unsigned> rel_T_bbox = T_bbox;
  tmp_pt.set( min_i + T_bbox.width()/2 , 
              min_j + T_bbox.height()/2 );
  rel_T_bbox.set_centroid( tmp_pt );

  assert( R_bbox.min_x() >= Rpad_bbox_.min_x() &&
              R_bbox.min_y() >= Rpad_bbox_.min_y() );
  
  vgl_box_2d<unsigned> rel_R_bbox = R_bbox;
  tmp_pt.set( R_bbox.centroid_x() - Rpad_bbox_.min_x(),
              R_bbox.centroid_y() - Rpad_bbox_.min_y());
  rel_R_bbox.set_centroid( tmp_pt );

  //Storing aT_bbox for the sake of debugging. T_bbox aligned with R_bbox 
  //in the image coordinates for displaying. 
  out_bbox = T_bbox;
  out_bbox.set_centroid( R_bbox.centroid() + 
                       ( rel_T_bbox.centroid() - rel_R_bbox.centroid() ) );
}

/********************************************************************/
// TODO: Move this out to library/video OR vil/algorithm
//
// Finds a tighter version (bbox) of the original box (amhi_box_on_img)
// which is equal to the size of amhi weight matrix (M). 
template<class PixType>
void 
amhi<PixType>
::get_tighter_bbox( vil_image_view<float> M, 
                    float threshold,
                    vgl_box_2d<unsigned> & bbox, 
                    vgl_box_2d<unsigned> const & amhi_box_on_img )
{
  vil_image_view<bool> mask; 
  vil_threshold_above( M, mask, threshold );

  vgl_box_2d<unsigned> local_box;
  local_box.empty();
  for(unsigned i = 0; i < mask.ni(); i++ )
  {
    for(unsigned j = 0; j < mask.nj(); j++ )
    {
      if( mask(i,j) )
      {
        local_box.add( vgl_point_2d<unsigned>( i,j ) );
      }
    }
  }

  // There is atleast one pixel in amhi weight matrix above the threshold 
  if( !local_box.is_empty() )
  {
    bbox.empty();
    bbox.add( vgl_point_2d<unsigned>( amhi_box_on_img.min_x() + local_box.min_x(),
                                      amhi_box_on_img.min_y() + local_box.min_y() ) );
    bbox.add( vgl_point_2d<unsigned>( amhi_box_on_img.min_x() + local_box.max_x(),
                                      amhi_box_on_img.min_y() + local_box.max_y() ) );
  }
  else
  {
    bbox = amhi_box_on_img;
  }
}
/********************************************************************/

}//namespace 
