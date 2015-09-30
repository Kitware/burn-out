/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vcl_cassert.h>
#include <vcl_fstream.h>
#include <vcl_algorithm.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vgl/algo/vgl_h_matrix_2d_optimize_lmq.h>

#include <vnl/vnl_math.h>

#include <rrel/rrel_homography2d_est.h>
#include <rrel/rrel_lms_obj.h>
#include <rrel/rrel_lts_obj.h>
#include <rrel/rrel_irls.h>
#include <rrel/rrel_ransac_obj.h>
#include <rrel/rrel_trunc_quad_obj.h>
#include <rrel/rrel_mlesac_obj.h>
#include <rrel/rrel_muset_obj.h>
#include <rrel/rrel_ran_sam_search.h>

#include <logger/logger.h>

///FIXME: "world" should not be an image. It should be a "plane". This process should
//        output homographies to both world and reference (img0) separately.

namespace vidtk
{

VIDTK_LOGGER ("homography_process");

//NOTE: Use initialize() for all variable initializations.
homography_process
::homography_process( vcl_string const& name )
  : process( name, "homography_process" ),
    refine_geometric_error_( false ),
    use_good_flag_( false ),
    good_thresh_sqr_( 3*3 ),
    current_ts_(NULL),
    ransam_( NULL ),
    initialized_( false ),
    unusable_frame_( false )
{
  img0_to_world_H_.set_identity(false);
  config_.add( "refine_with_geometric_error", "false" );
  config_.add( "use_good_flag", "false" );

  config_.add_parameter( "good_threshold",
    "3",
    "Back-projection error (Euclidean distance)in pixels. KLT point in current "
    "frame is projected to the ref frame and error is measured against the known "
    "location there." );

  config_.add( "image0_to_world", "1 0 0 0 1 0 0 0 1" );
}


homography_process
::~homography_process()
{
  delete ransam_;
}


config_block
homography_process
::params() const
{
  return config_;
}


bool
homography_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "refine_with_geometric_error", refine_geometric_error_ );
    blk.get( "use_good_flag", use_good_flag_ );
    double thr;
    blk.get( "good_threshold", thr );
    good_thresh_sqr_ = thr * thr;
    vnl_double_3x3 m;
    blk.get( "image0_to_world", m );
    img0_to_world_H_.set_transform( homography::transform_t(m));
  }
  catch( unchecked_return_value& )
  {
    // reset to old values
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
homography_process
::initialize()
{
  if( this->initialized_ == false )
  {
    img_to_img0_H_.set_identity(false);
    img_to_world_H_ = img0_to_world_H_;
    world_to_img_H_ = img_to_world_H_.get_inverse();

    img_to_world_H_.set_valid(false);
    world_to_img_H_.set_valid(false);

    tracks_.clear();
    delete ransam_;
    ransam_ = new rrel_ran_sam_search( 42 );

    this->initialized_ = true;
    current_ts_ = (NULL);
    reference_ts_ = timestamp();
  }
  return true;
}

bool
homography_process
::reinitialize()
{
  this->initialized_ = false;
  return this->initialize();
}

bool
homography_process
::reset()
{
  return this->reinitialize();
}

static bool is_same(homography_process::track_extra_info_type a, klt_track_ptr b)
{
  return a.track_ == b;
}


// ----------------------------------------------------------------
/** Main processing method.
 *
 *
 */
bool
homography_process
::step()
{
  if( unusable_frame_ )
  {
    LOG_WARN( this->name() << ": Not computing homography because frame is"
              << " marked as unusable at " << *current_ts_ );
    img_to_world_H_.set_identity( false );
    world_to_img_H_.set_identity( false );
    return true;
  }

  update_track_states();

  // Note that we estimate the homography from the current image to
  // the first image, and not directly to the world plane, because the
  // problem is better conditioned.  The first image -> world
  // homography is provided by the user, and can often make the
  // problem quite poorly conditioned.  This is especially true of
  // homographies that try to map from the image plane directly to the
  // world.


  // Step 1: gather up all the correspondences we have.  These are the
  // points that we have tracked, and whose (estimated) location on
  // the first image is known.

  vcl_vector< vnl_vector<double> > from_pts;
  vcl_vector< vnl_vector<double> > to_pts;

  vnl_vector<double> p(3);
  p[2] = 1.0;
  track_eit_iter end = tracks_.end();
  for( track_eit_iter it = tracks_.begin(); it != end; ++it )
  {
    if( it->good_ )
    {
      klt_track::point_t pt = it->track_->point();

      // If the tracked point is on the mask, then mark the track as a
      // bad track.
      //
      int img_i = vnl_math::rnd( pt.x );
      int img_j = vnl_math::rnd( pt.y );
      if( img_i >= 0 && unsigned(img_i) < mask_.ni() &&
          img_j >= 0 && unsigned(img_j) < mask_.nj() &&
          mask_( img_i, img_j ) == true )
      {
        it->good_ = false;
      }
      else if( it->have_img0_loc_ )
      {
        p[0] = pt.x; p[1] = pt.y;
        from_pts.push_back( p );

        vnl_vector_fixed<double,3> const& wld = it->img0_loc_;
        p[0] = wld[0]; p[1] = wld[1];
        to_pts.push_back( p );
      }
    }
  }


  // This will typically happen only on the first frame, because none
  // of the points will have img0 locations.  Of course, we set the
  // transform to identity, and then the points will gain their img0
  // locations.
  if( from_pts.size() < 4 )
  {
    img_to_img0_H_.set_identity(true);
    if(current_ts_)
    {
      LOG_INFO (this->name() << ": new reference (1) at " << *current_ts_ );

      reference_ts_ = *current_ts_;

      img_to_img0_H_.set_source_reference(*current_ts_)
        .set_dest_reference(reference_ts_)
        .set_new_reference(true);

      img0_to_world_H_.set_source_reference(*current_ts_)
        .set_dest_reference(reference_ts_)
        .set_valid(true)
        .set_new_reference(true);
    }

    img_to_world_H_ = img0_to_world_H_;

    world_to_img_H_ = img_to_world_H_.get_inverse();
    world_to_img_H_.set_new_reference (true);

    set_img0_locations();

    return true;
  }


  // Step 2: estimate the homography using sampling.  This will allow
  // a good rejection of outliers.
  rrel_homography2d_est hg( from_pts, to_pts );

  rrel_trunc_quad_obj msac;
  ransam_->set_trace_level(0);

  bool result = ransam_->estimate( &hg, &msac );

  if ( ! result )
  {
    vcl_cout << name() << ": MSAC failed!!\n";
  }
  else
  {
    // Step 3: refine the estimate using weighted least squares.  This
    // will allow us to estimate a homography that does not exactly
    // fit 4 points, which will be a better estimate.  The ransam
    // estimate from step 2 would have gotten us close enough to the
    // correct solution for IRLS to work.
    rrel_irls irls;
    irls.initialize_params( ransam_->params() );
    bool result2 = irls.estimate( &hg, &msac );
    if( ! result2 )
    {
      // if the IRLS fails, fall back to the ransam estimate.
      vcl_cout << name() << ": IRLS failed\n";
      vnl_double_3x3 m;
      hg.params_to_homog( ransam_->params(), m.as_ref().non_const() );
      img_to_img0_H_.set_transform( homography::transform_t(m) );
    }
    else
    {
      vnl_double_3x3 m;
      hg.params_to_homog( irls.params(), m.as_ref().non_const() );
      img_to_img0_H_.set_transform( homography::transform_t(m) );
    }
  }

  if( refine_geometric_error_ && result )
  {
    lmq_refine_homography();
  }

  if( current_ts_ )
  {
    img_to_img0_H_.set_source_reference(*current_ts_);

    if( !reference_ts_.is_valid() )
    {
      LOG_INFO (this->name() << ": new reference (2) at " << *current_ts_ );

      reference_ts_ = *current_ts_;
      img_to_img0_H_.set_new_reference( true );
      img_to_world_H_.set_new_reference( true );
      img0_to_world_H_.set_new_reference( true );
    }
    else
    {
      img_to_img0_H_.set_new_reference( false );
      img_to_world_H_.set_new_reference( false );
      img0_to_world_H_.set_new_reference( false );
    }
  }

  img_to_img0_H_.set_dest_reference( reference_ts_ );
  img_to_world_H_.set_dest_reference( reference_ts_ );
  img0_to_world_H_.set_dest_reference( reference_ts_ );

  // Update the inverse and add the user-provided world
  // transformation.
  //
  // The cost of this inverse is neligible compared to the cost above
  // of estimating the homograpy, so we might as well just compute it,
  // even if the user doesn't use it.
  img_to_world_H_ = img0_to_world_H_ * img_to_img0_H_;
  img_to_world_H_.set_valid(true);

  world_to_img_H_ = img_to_world_H_.get_inverse();
  world_to_img_H_.set_new_reference (true) // dest ref is always new
    .set_valid(true);

  // check that both the forward and backward homographies are finite.
  bool all_are_finite = true;
  for (unsigned i = 0; i < 3; ++i)
  {
    for (unsigned j = 0; j < 3; ++j)
    {
      all_are_finite =
        all_are_finite &&
        vnl_math::isfinite( img_to_world_H_.get_transform().get( i, j ) ) &&
        vnl_math::isfinite( world_to_img_H_.get_transform().get( i, j ) );
    }
  }
  if ( ! all_are_finite )
  {
    vcl_cout << name() << ": Non-finite values generated!!\n";
    result = false;
  }

  // If we think we have a reasonable homography, we can set the img0
  // location of points we started to track on this frame.
  if( result )
  {
    set_img0_locations();
    if( use_good_flag_ )
    {
      set_good_flags();
    }
  }

  current_ts_ = NULL;

  return result;
}


void
homography_process
::set_timestamp( timestamp const & ts )
{
  current_ts_ = &ts;
}


void
homography_process
::set_new_tracks( vcl_vector< klt_track_ptr > const& trks )
{
  new_tracks_ = trks;
}


void
homography_process
::set_updated_tracks( vcl_vector< klt_track_ptr > const& trks )
{
  updated_tracks_ = trks;
}


bool
homography_process
::is_same_track( track_extra_info_type const& tei,
                 klt_track_ptr const& trk ) const
{
  return *tei.track_ == *trk;
}


void
homography_process
::set_mask_image( vil_image_view<bool> const& mask )
{
  mask_ = mask;
}


vgl_h_matrix_2d<double> const&
homography_process
::image_to_world_homography() const
{
  return img_to_world_H_.get_transform();
}


vgl_h_matrix_2d<double> const&
homography_process
::world_to_image_homography() const
{
  return world_to_img_H_.get_transform();
}

image_to_image_homography const&
homography_process
::image_to_world_vidtk_homography_image() const
{
  return img_to_world_H_;
}

image_to_image_homography const&
homography_process
::world_to_image_vidtk_homography_image() const
{
  return world_to_img_H_;
}


vnl_vector_fixed<double,3>
homography_process
::project_to_world( vnl_vector_fixed<double,3> const& img_pt ) const
{
  assert( img_pt[2] == 0 );

  vgl_homg_point_2d<double> in( img_pt[0], img_pt[1] );

  vgl_point_2d<double> out = img_to_world_H_.get_transform() * in;

  vnl_vector_fixed<double,3> out_vec( out.x(),
                                      out.y(),
                                      0.0 );

  return out_vec;
}


vnl_vector_fixed<double,3>
homography_process
::project_to_image( vnl_vector_fixed<double,3> const& wld_pt ) const
{
  assert( wld_pt[2] == 0 );

  vgl_homg_point_2d<double> in( wld_pt[0], wld_pt[1] );

  vgl_point_2d<double> out = world_to_img_H_.get_transform() * in;

  vnl_vector_fixed<double,3> out_vec( out.x(),
                                      out.y(),
                                      0.0 );

  return out_vec;
}


bool
homography_process
::is_good( klt_track_ptr const& t ) const
{
  //state_map_type::const_iterator it = tracks_.find( t );
  //return it != tracks_.end() && it->second.good_;

  vcl_vector< track_extra_info_type >::const_iterator it_teis = tracks_.begin();
  vcl_vector< track_extra_info_type >::const_iterator end_teis = tracks_.end();

  for( ; it_teis != end_teis; ++it_teis )
  {
    if( is_same_track( *it_teis, t ) )
    {
      return it_teis->good_;
    }
  }

  // this is not a track we know about.
  return false;
}


void
homography_process
::lmq_refine_homography()
{
  // Gather the "good" points.  At this point, we expect that the
  // outliers have been eliminated.  Refine the homography based on
  // geometric error.

  vcl_vector<vgl_homg_point_2d<double> > from_pts;
  vcl_vector<vgl_homg_point_2d<double> > to_pts;

  track_eit_iter end = tracks_.end();
  for( track_eit_iter it = tracks_.begin();
       it != end; ++it )
  {
    if( it->good_ )
    {
      klt_track::point_t pt = it->track_->point();
      vnl_vector_fixed<double,3> const& wld = it->img0_loc_;

      from_pts.push_back( vgl_homg_point_2d<double>( pt.x, pt.y ) );
      to_pts.push_back( vgl_homg_point_2d<double>( wld[0], wld[1] ) );
    }
  }

  vgl_h_matrix_2d<double> refined_H;
  vgl_h_matrix_2d<double> initial_H( img_to_img0_H_.get_transform() );
  vgl_h_matrix_2d_optimize_lmq opt( initial_H );
  opt.set_verbose( false );
  if( opt.optimize( from_pts, to_pts, refined_H ) )
  {
    img_to_img0_H_.set_transform(refined_H.get_matrix());
  }
  else
  {
    LOG_WARN( this->name() << ": LMQ Refinement failed." );
  }
}


/// Update the img0 locations of all the tracks that don't have img0
/// locations.  This should be called after the homography for this
/// frame has been computed, because we need to know that to find out
/// what a reasonable estimate is for the img0 location of the image
/// point.
void
homography_process
::set_img0_locations()
{
  track_eit_iter end = tracks_.end();
  for( track_eit_iter it = tracks_.begin(); it != end; ++it )
  {
    if( ! it->have_img0_loc_ )
    {
      klt_track::point_t pt = it->track_->point();
      vgl_point_2d<double> p = img_to_img0_H_.get_transform() * vgl_homg_point_2d<double>( pt.x, pt.y );
      it->img0_loc_[0] = p.x();
      it->img0_loc_[1] = p.y();
      it->img0_loc_[2] = 0;
      it->have_img0_loc_ = true;
    }
  }
}


void
homography_process
::set_good_flags()
{
  unsigned good_counter = 0;
  track_eit_iter end = tracks_.end();
  for( track_eit_iter it = tracks_.begin(); it != end; ++it )
  {
    if( it->have_img0_loc_ &&
        it->good_ )
    {
      klt_track::point_t pt = it->track_->point();
      vgl_homg_point_2d<double> p( pt.x, pt.y );
      vgl_homg_point_2d<double> q = img_to_img0_H_.get_transform() * p;
      vnl_vector_fixed<double,3> const& img0 = it->img0_loc_;
      vgl_homg_point_2d<double> q0( img0[0], img0[1] );
      double sqr_err = (q-q0).sqr_length();
      if( sqr_err > good_thresh_sqr_ ) {
        it->good_ = false;
      }
      else
        good_counter++;
    }
  }

#ifdef PRINT_DEBUG_INFO
  LOG_DEBUG( this->name() << ": "<< good_counter << "/" << tracks_.size()
                          <<" KLT tracks are good." );
#endif

}

void
homography_process
::set_unusable_frame( bool flag )
{
  unusable_frame_ = flag;
}


// ----------------------------------------------------------------
/** Update track states.
 *
 * This method analyzes the updated_tracks_ vector.
 * The tracks_ vector is updated with the list of live tracks.
 */
void
homography_process
::update_track_states()
{
  vcl_vector< klt_track_ptr >::const_iterator it = updated_tracks_.begin();
  vcl_vector< klt_track_ptr >::const_iterator t_end = updated_tracks_.end();

  vcl_vector< track_extra_info_type > live_tracks;

  for( ; it != t_end; ++it )
  {
    track_eit_iter end_teis = vcl_find_if( tracks_.begin(), tracks_.end(),
            boost::bind( &is_same, _1, (*it)->tail() ));

    if (end_teis != tracks_.end())
    {
      live_tracks.push_back( *end_teis );
      live_tracks.rbegin()->track_ = *it;
    }
  }

  it = new_tracks_.begin();
  t_end = new_tracks_.end();

  for( ; it != t_end; ++it )
  {
    track_extra_info_type tei;
    tei.track_ = *it;
    tei.have_img0_loc_ = false;
    tei.good_ = true;

    live_tracks.push_back( tei );
  }

  tracks_ = live_tracks;
}

} // end namespace vidtk
