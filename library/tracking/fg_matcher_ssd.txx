/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "fg_matcher_ssd.h"

#include <vcl_vector.h>
#include <vcl_algorithm.h>


#include <vil/vil_image_view.h>
#include <vil/vil_math.h>
#include <vil/vil_crop.h>
#include <vil/vil_convert.h>

#include <vgl/vgl_point_2d.h>

#include <vcl_sstream.h>

#include <video/ssd.h>
#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <tracking/tracking_keys.h>

namespace vidtk
{

// Static member

template < class PixType >
fg_matcher_ssd_params fg_matcher_ssd< PixType >::params_;

/********************************************************************/

template < class PixType >
config_block
fg_matcher_ssd< PixType >
::params()
{
  return params_.config_;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_ssd< PixType >
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "max_dist", params_.max_dist_sqr_ );
    params_.max_dist_sqr_ *= params_.max_dist_sqr_;
    blk.get( "max_bbox_side_in_pixels", params_.max_bbox_side_in_pixels_ );
    blk.get( "max_frac_valley_width", params_.max_frac_valley_width_ );
    blk.get( "min_frac_valley_depth", params_.min_frac_valley_depth_ );
    blk.get( "padding_factor", params_.padding_factor_ );
    blk.get( "min_valley_depth", params_.min_valley_depth_ );
    blk.get( "use_weights", params_.use_weights_ );
    blk.get( "disable_multi_min_foreground", params_.disable_multi_min_fg_ );
    blk.get( "amhi_mode", params_.amhi_mode_ );
    blk.get( "max_update_accel", params_.max_update_accel_ );
    blk.get( "max_update_narea", params_.max_update_narea_ );
    vcl_string update_algo;
    blk.get( "update_algo", update_algo );
    if( update_algo == "always" )
    {
      params_.update_algo_ = fg_matcher_ssd_params::UPDATE_EVERY_FRAME;
    }
    else if( update_algo == "accel_area" )
    {
      params_.update_algo_ = fg_matcher_ssd_params::UPDATE_ACCEL_AREA;
    }
    else
    {
      log_error( "Invalid string specified for 'update_algo'\n" );
      return false;
    }
  }
  catch( unchecked_return_value & e )
  {
    log_error( "fg_matcher_ssd: set_params failed because, "
      << e.what() <<".\n" );
    return false;
  }

  params_.config_.update( blk );
  return true;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_ssd< PixType >
::create_model( track_sptr track,
                typename fg_matcher<PixType>::image_buffer_t const& img_buff,
                timestamp const & curr_ts )
{
  if( params_.amhi_mode_ )
  {
    // Do nothing. amhi_initializer_process was already doing the job.
  }
  else
  {
    // TODO: Implement functionality.

    // Simply use the last image chip as the model.
    timestamp const& ts = track->last_state()->time_;
    bool found;
    vil_image_view< PixType > const& im = img_buff.find_datum( ts, found, true );
    if( !found )
    {
      log_error( "Could not find image for last track state in the buffer.\n" );
      return false;
    }

    // Note that we're creating a shallow copy in the model.
    model_image_ = track->get_end_object()->templ( im );
    model_image_ts_ = ts;

  } // if !amhi_mode_

  return true;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_ssd< PixType >
::find_matches( track_sptr track,
                vgl_box_2d<unsigned> const & predicted_bbox,
                vil_image_view<PixType> const & curr_im,
                vcl_vector< vgl_box_2d<unsigned> > &out_bboxes ) const
{
  bool match_found = false;

  vil_image_view<PixType> const * templ_im;
  vil_image_view<PixType> templ_im_tmp;
  vil_image_view<float> const * templ_weight;
  vil_image_view<float> templ_weight_tmp;
  amhi_data const amhi_dat = track->amhi_datum();

  if( params_.amhi_mode_ )
  {
    if( !amhi_dat.image || !amhi_dat.weight )
    {
      log_error( "fg_matcher_ssd: Do not have valid AMHI model. "
        " Cannot use parameter value: amhi_mode = true\n" );
      return match_found;
    }

    templ_im = &amhi_dat.image;
    templ_weight = &amhi_dat.weight;
  }
  else
  {
    templ_im = &model_image_;
    if( params_.use_weights_ )
    {
      vil_image_view<bool> const& templ_mask = track->get_end_object()->mask_;
      templ_weight_tmp.set_size( templ_mask.ni(), templ_mask.nj() );
      vil_convert_cast( templ_mask, templ_weight_tmp ); // static_cast<float>(bool_im)
    }
    templ_weight = &templ_weight_tmp; // Will be an empty image if !use_weights_
  }

  log_assert( templ_im && templ_weight, "Expect to have valid pointers to "
    <<"image and weight.\n" );

  out_bboxes.clear();
  if( params_.disable_multi_min_fg_ )
  {
    vgl_box_2d<unsigned> out_bbox;
    find_single_match( predicted_bbox,
                       curr_im,
                       *templ_im,
                       *templ_weight,
                       out_bbox,
                       match_found );
    if( match_found )
    {
      out_bboxes.push_back(out_bbox);
    }
  }
  else
  {
    find_multi_matches( predicted_bbox,
                        curr_im,
                        *templ_im,
                        *templ_weight,
                        out_bboxes,
                        match_found );
  }

  return match_found;
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_ssd< PixType >
::update_model( track_sptr track,
                typename fg_matcher<PixType>::image_buffer_t const & img_buff,
                timestamp const & curr_ts  )
{
  if( params_.amhi_mode_ )
  {
    // Do nothing. amhi_initializer_process was already doing the job.
    return true;
  }
  else
  {
    if( track->last_state()->time_ == curr_ts )
    {
      if( params_.update_algo_ == fg_matcher_ssd_params::UPDATE_EVERY_FRAME )
      {
        bool found = false;
        vil_image_view< PixType > const& im = img_buff.find_datum( curr_ts, found, true );

        if( !found )
        {
          log_error( "fg_matcher_ssd: Could not find the current time in"
            " the image buffer.\n" );
          return false;
        }

        model_image_ = track->get_end_object()->templ( im );
        model_image_ts_ = curr_ts;
      } // if UPDATE_EVERY_FRAME
      else if( params_.update_algo_ == fg_matcher_ssd_params::UPDATE_ACCEL_AREA )
      {
        const unsigned REQ_LEN = 3;
        vcl_vector< track_state_sptr > const& hist = track->history();
        if( hist.size() < REQ_LEN )
        {
          log_error( "Track length (" << hist.size()
            <<") not expected. Required min length: "<< REQ_LEN <<"\n" );
          return false;
        }

        vcl_vector< image_object::float3d_type > vel, accel;
        vel.reserve( REQ_LEN - 1 );
        accel.reserve( REQ_LEN - 2 );
        image_object_sptr last_obj, obj;
        if( !hist[ hist.size() - REQ_LEN ]->image_object( last_obj ) )
        {
          log_error( "Could not extract image_object.\n" );
          return false;
        }

        // Iterating over last REQ_LEN states of the track_
        for( unsigned i = unsigned(hist.size() - REQ_LEN + 1); i < hist.size(); i++ )
        {
          if( !hist[i]->image_object( obj ) )
          {
            log_error( "Could not extract image_object.\n" );
            return false;
          }

          // Get non-smoothed position to compute *raw* velocity
          image_object::float3d_type last_D = obj->world_loc_ - last_obj->world_loc_;
          vel.push_back( last_D / hist[i]->time_.diff_in_secs( hist[i-1]->time_ ) );

          if( vel.size() > 1 ) // i.e. i > 1
          {
            accel.push_back( ( vel[ vel.size() - 1 ] - vel[ vel.size() - 2 ] )
                             / hist[i]->time_.diff_in_secs( hist[i-2]->time_ ) );
          }

          last_obj = obj;
        }

        double dAccel = accel.back().magnitude();

        if( !hist[ hist.size() - 2 ]->image_object( last_obj ) )
        {
          log_error( "Could not extract image_object.\n" );
          return false;
        }
        double ndArea =  vcl_abs(obj->area_ - last_obj->area_)/
                         vcl_max(obj->area_, last_obj->area_);

        if( dAccel < params_.max_update_accel_ && ndArea < params_.max_update_narea_ )
        {
          bool found = false;
          vil_image_view< PixType > const& im = img_buff.find_datum( curr_ts, found, true );

          if( !found )
          {
            log_error( "fg_matcher_ssd: Could not find the current time in"
              " the image buffer.\n" );
            return false;
          }

          model_image_ = track->get_end_object()->templ( im );
          model_image_ts_ = curr_ts;
        }

#ifdef PRINT_DEBUG_INFO
        log_debug( "fg_matcher_ssd::update_model() for " << *(track)
          << " model time: " << model_image_ts_  << " with dAccel: " << dAccel 
          << " ndArea: "<< ndArea << "\n" );
#endif
      } // if UPDATE_ACCELL_AREA
      else
      {
        log_error( "fg_matcher_ssd: Invalid model update algo.\n" );
        return false;
      }
    } // if( track_->last_state()->time_ == curr_ts )

    return true;
  } // ! amhi_mode_
}

/********************************************************************/

template < class PixType >
void
fg_matcher_ssd< PixType >
::find_single_match(vgl_box_2d<unsigned> const & predicted_bbox,
                    vil_image_view<PixType> const & curr_im,
                    vil_image_view<PixType> const & templ_im,
                    vil_image_view<float> const& templ_weight,
                    vgl_box_2d<unsigned> & out_bbox,
                    bool & match_found ) const
{
  match_found = false;

  vil_image_view< corr_type > corr_surf;

  assert( templ_im.ni() == templ_weight.ni() &&
          templ_im.nj() == templ_weight.nj() );

  if(  templ_im.ni() > params_.max_bbox_side_in_pixels_
    || templ_im.nj() > params_.max_bbox_side_in_pixels_
    || templ_im.ni() == 0
    || templ_im.nj() == 0 )
  {
    vcl_cout<<"fg_matcher_ssd::find_match: bailing out "
      << "for optimization."<<templ_im.ni()<<"x"<<templ_im.nj()
      <<" -> "<<predicted_bbox.width()<<"x"<<predicted_bbox.height()<<vcl_endl;
    return;
  }

  bool result;
  vgl_box_2d<unsigned> Rpad_bbox;

  result = this->warp_bbox( curr_im,
                            predicted_bbox,
                            templ_im,
                            corr_surf,
                            Rpad_bbox,
                            templ_weight );

  if( result )
  {
    match_found = ! detect_ghost( corr_surf,
                                  params_.max_bbox_side_in_pixels_,
                                  params_.max_frac_valley_width_,
                                  params_.min_frac_valley_depth_ );

    if( match_found )
    {
      vgl_box_2d<unsigned> const templ_wh( 0, templ_im.ni(),
                                           0, templ_im.nj() );
      fg_bbox_update( templ_wh,
                      predicted_bbox,
                      Rpad_bbox,
                      corr_surf,
                      out_bbox );
    }

    vil_image_view<const corr_type>::iterator iter;

    if( corr_surf.ni() < 2 || corr_surf.nj() < 2 )
    {
      return;
    }
    else
    {
      iter = vcl_min_element( corr_surf.begin(), corr_surf.end() );
    }

    match_found = match_found && (*iter < params_.max_dist_sqr_);
  }
}

/********************************************************************/

template < class PixType >
void
fg_matcher_ssd< PixType >
::find_multi_matches( vgl_box_2d<unsigned> const & predicted_bbox,
                      vil_image_view<PixType> const & curr_im,
                      vil_image_view<PixType> const & templ_im,
                      vil_image_view<float> const& templ_weight,
                      vcl_vector< vgl_box_2d<unsigned> > &out_bboxes,
                      bool & match_found ) const
{
  match_found = false;

  vil_image_view< corr_type > corr_surf;

  if(  templ_im.ni() > params_.max_bbox_side_in_pixels_
    || templ_im.nj() > params_.max_bbox_side_in_pixels_
    || templ_im.ni() == 0
    || templ_im.nj() == 0
    || predicted_bbox.width() > params_.max_bbox_side_in_pixels_
    || predicted_bbox.height() > params_.max_bbox_side_in_pixels_ )
  {
    vcl_cout<<"fg_matcher_ssd::find_multi_matches: bailing out for "
      <<"optimization."<<templ_im.ni()<<"x"<<templ_im.nj()<<" -> "
      <<predicted_bbox.width()<<"x"<<predicted_bbox.height()<<vcl_endl;
    return;
  }

  bool result;
  vgl_box_2d<unsigned> Rpad_bbox;

  result = this->warp_bbox( curr_im,
                            predicted_bbox,
                            templ_im,
                            corr_surf,
                            Rpad_bbox,
                            templ_weight );

  if( result )
  {
    vcl_vector< unsigned int > min_is, min_js;
    find_local_minima(corr_surf,
                      params_.max_dist_sqr_,
                      params_.min_valley_depth_,
                      templ_im.ni(),
                      templ_im.nj(),
                      min_is,
                      min_js );
    match_found = min_is.size() > 0;
    assert(min_is.size() == min_js.size());

    for( unsigned int i = 0; i < min_is.size(); ++i )
    {
      vgl_box_2d<unsigned> out_bbox;
      vgl_box_2d<unsigned> const templ_wh( 0, templ_im.ni(),
                                           0, templ_im.nj() );
      fg_bbox_update( templ_wh,
                      predicted_bbox,
                      Rpad_bbox,
                      corr_surf,
                      out_bbox );
      out_bboxes.push_back(out_bbox);
    }
  }
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_ssd< PixType >
::warp_bbox(vil_image_view<PixType> const& R_im,
            vgl_box_2d<unsigned> const& R_bbox,
            vil_image_view<PixType> const& T_chip,
            vil_image_view<corr_type> & corr_surf,
            vgl_box_2d<unsigned> & Rpad_bbox,
            vil_image_view<float> const& T_w ) const
{
  // If weight image is provided.
  if( !!T_w )
  {
    log_assert( T_chip.ni() == T_w.ni() && T_chip.nj() == T_w.nj(),
      "Size mismatch between the template and mask sizes.\n" );
  }

  // Prepare input image with R_bbox and 'extra' area around it
  // for correlation.
  unsigned padded_W = (unsigned) vcl_floor( T_chip.ni() * params_.padding_factor_ );
  unsigned padded_H = (unsigned) vcl_floor( T_chip.nj() * params_.padding_factor_ );

  Rpad_bbox.empty();
  vgl_point_2d<unsigned> tmp_pt;
  tmp_pt.set( static_cast<unsigned>(vcl_max(0, (int) (R_bbox.min_x() - padded_W ))),
              static_cast<unsigned>(vcl_max(0, (int) (R_bbox.min_y() - padded_H ))) );
  Rpad_bbox.set_min_point( tmp_pt );

  tmp_pt.set( vcl_min( R_im.ni()-1, R_bbox.max_x() + padded_W),
              vcl_min( R_im.nj()-1, R_bbox.max_y() + padded_H) );
  Rpad_bbox.set_max_point( tmp_pt );

  vil_image_view<PixType> Rpad_chip = vil_crop( R_im,
                                                 Rpad_bbox.min_x(),
                                                 Rpad_bbox.width(),
                                                 Rpad_bbox.min_y(),
                                                 Rpad_bbox.height() );

  // If weight image is not provided.
  if( !T_w )
    return get_ssd_surf(Rpad_chip, corr_surf, T_chip );
  else
    return get_ssd_surf(Rpad_chip, corr_surf, T_chip, T_w);
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_ssd< PixType >
::fg_bbox_update(  vgl_box_2d<unsigned> const & T_bbox,
                   vgl_box_2d<unsigned> const & R_bbox,
                   vgl_box_2d<unsigned> const & Rpad_bbox,
                   vil_image_view<corr_type> const & corr_surf,
                   vgl_box_2d<unsigned> & out_bbox ) const
{
  unsigned min_i, min_j;
  find_local_minimum(corr_surf, min_i, min_j);

  return fg_bbox_update( T_bbox, R_bbox, Rpad_bbox, min_i, min_j, out_bbox );
}

/********************************************************************/

template < class PixType >
bool
fg_matcher_ssd< PixType >
::fg_bbox_update( vgl_box_2d<unsigned> const & T_bbox,
                  vgl_box_2d<unsigned> const & R_bbox,
                  vgl_box_2d<unsigned> const & Rpad_bbox,
                  unsigned int min_i,
                  unsigned int min_j,
                  vgl_box_2d<unsigned> & out_bbox ) const
{
  //rel_* is in ref to Rpad_*
  //T_bbox to correlation peak.
  vgl_point_2d<unsigned> tmp_pt;
  vgl_box_2d<unsigned> rel_T_bbox = T_bbox;
  tmp_pt.set( min_i + T_bbox.width()/2 ,
              min_j + T_bbox.height()/2 );
  rel_T_bbox.set_centroid( tmp_pt );

  assert( R_bbox.min_x() >= Rpad_bbox.min_x() &&
          R_bbox.min_y() >= Rpad_bbox.min_y() );

  vgl_box_2d<unsigned> rel_R_bbox = R_bbox;
  tmp_pt.set( R_bbox.centroid_x() - Rpad_bbox.min_x(),
              R_bbox.centroid_y() - Rpad_bbox.min_y());
  rel_R_bbox.set_centroid( tmp_pt );

  //Storing aT_bbox for the sake of debugging. T_bbox aligned with R_bbox
  //in the image coordinates for displaying.
  out_bbox = T_bbox;
  out_bbox.set_centroid( R_bbox.centroid() +
                        ( rel_T_bbox.centroid() - rel_R_bbox.centroid() ) );
  return true;
}

/********************************************************************/

template < class PixType >
typename fg_matcher< PixType >::sptr_t
fg_matcher_ssd< PixType >
::deep_copy()
{
  typename fg_matcher< PixType >::sptr_t cp =
    typename fg_matcher< PixType >::sptr_t( new fg_matcher_ssd< PixType > );

  fg_matcher_ssd< PixType > * ssd_ptr = dynamic_cast< fg_matcher_ssd< PixType > * >( cp.get() );
  *ssd_ptr = *this;
  if( !params_.amhi_mode_ )
  {
    ssd_ptr->copy_model( this->model_image_ );
  }

  return cp;
}

/********************************************************************/

template < class PixType >
void
fg_matcher_ssd< PixType >
::copy_model( vil_image_view< PixType > const& im )
{
  model_image_.deep_copy( im );
}

/********************************************************************/

} // namespace vidtk
