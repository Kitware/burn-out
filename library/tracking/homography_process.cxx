/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_process.h"
#include <utilities/homography_util.h>
#include <cassert>
#include <fstream>
#include <algorithm>

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
::homography_process( std::string const& _name )
  : process( _name, "homography_process" ),
    refine_geometric_error_( false ),
    use_good_flag_( false ),
    good_thresh_sqr_( 3*3 ),
    current_ts_(NULL),
    ransam_( NULL ),
    initialized_( false ),
    unusable_frame_( false ),
    track_tail_length_( 0 )
{
  img0_to_world_H_.set_identity(false);
  config_.add_parameter( "refine_with_geometric_error", "false", "UNDOCUMENTED" );
  config_.add_parameter( "use_good_flag", "false", "UNDOCUMENTED" );

  config_.add_parameter( "good_threshold",
    "3",
    "Back-projection error (Euclidean distance)in pixels. KLT point in current "
    "frame is projected to the ref frame and error is measured against the known "
    "location there." );

  config_.add_parameter( "image0_to_world", "1 0 0 0 1 0 0 0 1", "UNDOCUMENTED" );

  config_.add_parameter( "trim_track_tail_size", "3",
    "Trim the KLT track tail to the specified number of "
    "elements. A value of 0 disables track tail trimming and lets the tails grow without bound. "
    "The reason to trim track tails is to limit process memory growth. Track tails grow by one "
    "element per frame in a shot. The tracks are reset/deleted when a stabilization break occurs. "
    "When the tails are not trimmed, process memory growth is unbounded and becomes a problem with "
    "extremely long shots. The only reason to keep track tails at all is to allow the GUI to "
    "display them." );
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
    refine_geometric_error_ = blk.get< bool > ( "refine_with_geometric_error" );
    use_good_flag_ = blk.get< bool > ( "use_good_flag" );
    double thr = blk.get< double > ( "good_threshold" );
    good_thresh_sqr_ = thr * thr;
    vnl_double_3x3 m = blk.get< vnl_double_3x3 > ( "image0_to_world" );
    this->track_tail_length_ = blk.get< int > ( "trim_track_tail_size" );
    if ( (this->track_tail_length_ > 0) && (this->track_tail_length_ < 3) ) // assure minimum length
    {
      this->track_tail_length_ = 3;
    }
    img0_to_world_H_.set_transform( homography::transform_t( m ) );
  }
  catch ( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
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
    if( current_ts_ )
    {
      LOG_TRACE( "frametrace: homography_process: " << current_ts_->frame_number() << " marked-unusable" );
    }
    else
    {
      LOG_TRACE( "frametrace: homography_process: mystery-frame marked-unusable" );
    }
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

  std::vector< vnl_vector<double> > from_pts;
  std::vector< vnl_vector<double> > to_pts;
  int drop_count(0);
  int total_count(0);

  vnl_vector<double> p(3);
  p[2] = 1.0;
  track_eit_iter end = tracks_.end();
  for( track_eit_iter it = tracks_.begin(); it != end; ++it )
  {
    if( it->good_ )
    {
      total_count++;
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
        drop_count++;           // count points dropped by masking
      }
      else if( it->have_img0_loc_ )
      {
        p[0] = pt.x;
        p[1] = pt.y;
        from_pts.push_back( p );

        vnl_vector_fixed<double,3> const& wld = it->img0_loc_;
        p[0] = wld[0];
        p[1] = wld[1];
        to_pts.push_back( p );
      }
    }
  }

  LOG_DEBUG ("Dropped " << drop_count << " points from "
             <<  total_count << " due to masking, saved "
             << from_pts.size() << " points.");

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

    if( IS_TRACE_ENABLED() )
    {
      int fn = (current_ts_ == 0) ? -1 : current_ts_->frame_number();
      LOG_TRACE( "frametrace: homography_process: " << fn << " new-reference" );
    }

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
    LOG_ERROR( name() << ": MSAC failed!!");
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
      LOG_WARN( name() << ": IRLS failed");
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
    LOG_INFO( name() << ": Non-finite values generated!!");
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

  if( IS_TRACE_ENABLED() )
  {
    int fn = (current_ts_ == 0) ? -1 : current_ts_->frame_number();
    LOG_TRACE( "frametrace: homography_process: " << fn << " carrying-on ; result " << result );
  }
  current_ts_ = NULL;

  img_to_img0_H_.normalize();
  img_to_world_H_.normalize();
  img0_to_world_H_.normalize();

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
::set_new_tracks( std::vector< klt_track_ptr > const& trks )
{
  new_tracks_ = trks;
}


void
homography_process
::set_updated_tracks( std::vector< klt_track_ptr > const& trks )
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


vgl_h_matrix_2d<double>
homography_process
::image_to_world_homography() const
{
  return img_to_world_H_.get_transform();
}


vgl_h_matrix_2d<double>
homography_process
::world_to_image_homography() const
{
  return world_to_img_H_.get_transform();
}

image_to_image_homography
homography_process
::image_to_world_vidtk_homography_image() const
{
  return img_to_world_H_;
}

image_to_image_homography
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

  std::vector< track_extra_info_type >::const_iterator it_teis = tracks_.begin();
  std::vector< track_extra_info_type >::const_iterator end_teis = tracks_.end();

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

  std::vector<vgl_homg_point_2d<double> > from_pts;
  std::vector<vgl_homg_point_2d<double> > to_pts;

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
  std::vector< klt_track_ptr >::const_iterator it = updated_tracks_.begin();
  std::vector< klt_track_ptr >::const_iterator t_end = updated_tracks_.end();

  std::vector< track_extra_info_type > live_tracks;

  // check updated tracks against our last set of tracks to see if we
  // have seen the track before.
  for( ; it != t_end; ++it )
  {
    track_eit_iter end_teis = std::find_if( tracks_.begin(), tracks_.end(),
            boost::bind( &is_same, _1, (*it)->tail() ));

    // If we already have an entry for this track.
    if (end_teis != tracks_.end())
    {
      live_tracks.push_back( *end_teis ); // add this track to the list
      live_tracks.rbegin()->track_ = *it; // update

      // Trim tracks here so they do not grow too long
      if ( this->track_tail_length_ > 0 ) // do not trim if length set to zero
      {
        (*it)->trim_track( this->track_tail_length_ );
      }
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

  // Replace list of tracks with list of currently active tracks.
  tracks_ = live_tracks;
}

} // end namespace vidtk
