/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "frame_to_frame_homography_process.h"
#include <cassert>
#include <fstream>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vgl/algo/vgl_h_matrix_2d_optimize_lmq.h>

#include <rrel/rrel_homography2d_est.h>
#include <rrel/rrel_lms_obj.h>
#include <rrel/rrel_lts_obj.h>
#include <rrel/rrel_ran_sam_search.h>
#include <rrel/rrel_irls.h>
#include <rrel/rrel_ransac_obj.h>
#include <rrel/rrel_trunc_quad_obj.h>
#include <rrel/rrel_mlesac_obj.h>
#include <rrel/rrel_muset_obj.h>

#include <logger/logger.h>

VIDTK_LOGGER("frame_to_frame_homography_process");


namespace vidtk
{


frame_to_frame_homography_process
::frame_to_frame_homography_process( std::string const& _name )
  : process( _name, "frame_to_frame_homography_process" ),
    refine_geometric_error_( false ),
    delta_( 1 ),
    disabled_( false ),
    homog_is_valid_( false )
{
  config_.add_parameter( "refine_with_geometric_error", "false", "UNDOCUMENTED" );

  // If delta is 1, the homography is from frame to frame.  If delta
  // is 2, the homography is from 2 frames ago to the current frame.
  // And so on.  It doesn't make sense for delta_ to be zero.
  config_.add_parameter( "delta", "1", "UNDOCUMENTED" );

  config_.add_parameter( "disabled", "false", "UNDOCUMENTED" );

  forw_H_.set_identity();
  back_H_.set_identity();
}


frame_to_frame_homography_process
::~frame_to_frame_homography_process()
{
}


config_block
frame_to_frame_homography_process
::params() const
{
  return config_;
}


bool
frame_to_frame_homography_process
::set_params( config_block const& blk )
{
  try
  {
    refine_geometric_error_ = blk.get<bool>( "refine_with_geometric_error" );
    delta_ = blk.get<unsigned>( "delta" );
    disabled_ = blk.get<bool>( "disabled" );
 }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
frame_to_frame_homography_process
::initialize()
{
  return true;
}


bool
frame_to_frame_homography_process
::step()
{
  if( disabled_ )
  {
    homog_is_valid_ = true;
    return true;
  }

  homog_is_valid_ = false;

  std::vector< vnl_vector<double> > from_pts;
  std::vector< vnl_vector<double> > to_pts;

  vnl_vector<double> p(3);
  p[2] = 1.0;
  state_map_type::const_iterator end = tracks_.end();
  for( state_map_type::const_iterator it = tracks_.begin();
       it != end; ++it )
  {
    if( it->second.good_ )
    {
      std::vector< track_state_sptr > const& hist = it->first->history();
      unsigned const N = hist.size();
      if( N > delta_ )
      {
        vnl_vector_fixed<double,3> const& prev = hist[N-delta_-1]->loc_;
        p[0] = prev[0]; p[1] = prev[1];
        from_pts.push_back( p );

        vnl_vector_fixed<double,3> const& curr = hist[N-1]->loc_;
        p[0] = curr[0]; p[1] = curr[1];
        to_pts.push_back( p );
      }
    }
  }

  if( from_pts.size() < 4 )
  {
    forw_H_.set_identity();
    back_H_.set_identity();
    return true;
  }

  rrel_homography2d_est hg( from_pts, to_pts );

  rrel_trunc_quad_obj* msac = new rrel_trunc_quad_obj();
  rrel_ran_sam_search * ransam = new rrel_ran_sam_search;
  ransam->set_trace_level(0);

  bool result = ransam->estimate( &hg, msac );

  if ( ! result )
  {
    LOG_ERROR( name() << ": MSAC failed!!");
    forw_H_.set_identity();
  }
  else
  {
    vnl_double_3x3 m;
    hg.params_to_homog( ransam->params(), m.as_ref().non_const() );
    forw_H_.set( m );
    homog_is_valid_ = true;
  }

  if( refine_geometric_error_ && result )
  {
    lmq_refine_homography();
  }

  // Update the inverse.
  //
  // The cost of this inverse is neligible compared to the cost above
  // of estimating the homograpy, so we might as well just compute it,
  // even if the user doesn't use it.
  back_H_ = forw_H_.get_inverse();


#if 0 // debugging output of correspondences
  {
  static unsigned cnt = -1;
  ++cnt;
  std::ostringstream fn;
  fn << "homog" << cnt << ".txt";
  std::ofstream debugout( fn.str().c_str() );

  state_map_type::const_iterator end = tracks_.end();
  for( state_map_type::const_iterator it = tracks_.begin();
       it != end; ++it )
  {
    if( it->second.good_ )
    {
      vnl_vector_fixed<double,3> const& img = it->first->last_state()->loc_;
      debugout << img[0] << " " << img[1] << "     ";

      vnl_vector_fixed<double,3> const& wld = it->second.world_loc_;
      debugout << wld[0] << " " << wld[1] << "     ";

      vgl_point_2d<double> p2 = world_to_img_H_ * vgl_homg_point_2d<double>( wld[0], wld[1] );
      debugout << p2.x() << " " << p2.y() << "   ";

      vgl_point_2d<double> p = img_to_world_H_ * vgl_homg_point_2d<double>( img[0], img[1] );
      debugout << p.x() << " " << p.y() << "   ";

      debugout << "\n";
    }
  }
  }
#endif // debugging output

  delete msac;
  delete ransam;

  // We return true even if the homography estimation failed, because
  // it is normal for the estimate to fail at the beginning.  (For
  // example, there aren't enough tracked features at the beginning.)
  // The user can check the status of the homography explicitly
  // through homography_is_valid().

  return true;
}


void
frame_to_frame_homography_process
::set_new_tracks( std::vector< track_sptr > const& trks )
{
  std::vector< track_sptr >::const_iterator it = trks.begin();
  std::vector< track_sptr >::const_iterator end = trks.end();
  for( ; it != end; ++it )
  {
    extra_info_type& ei = tracks_[ *it ];
    // be optimistic at the beginning
    ei.good_ = true;
  }
}


void
frame_to_frame_homography_process
::set_terminated_tracks( std::vector< track_sptr > const& trks )
{
  std::vector< track_sptr >::const_iterator it = trks.begin();
  std::vector< track_sptr >::const_iterator end = trks.end();
  for( ; it != end; ++it )
  {
    tracks_.erase( *it );
  }
}


bool
frame_to_frame_homography_process
::homography_is_valid() const
{
  return homog_is_valid_;
}


vgl_h_matrix_2d<double>
frame_to_frame_homography_process
::forward_homography() const
{
  return forw_H_;
}


vgl_h_matrix_2d<double>
frame_to_frame_homography_process
::backward_homography() const
{
  return back_H_;
}


void
frame_to_frame_homography_process
::lmq_refine_homography()
{
  // Gather the "good" points.  At this point, we expect that the
  // outliers have been eliminated.  Refine the homography based on
  // geometric error.

  std::vector<vgl_homg_point_2d<double> > from_pts;
  std::vector<vgl_homg_point_2d<double> > to_pts;

  state_map_type::const_iterator end = tracks_.end();
  for( state_map_type::const_iterator it = tracks_.begin();
       it != end; ++it )
  {
    if( it->second.good_ )
    {
      vnl_vector_fixed<double,3> const& img = it->first->last_state()->loc_;
      vnl_vector_fixed<double,3> const& wld = it->second.world_loc_;

      from_pts.push_back( vgl_homg_point_2d<double>( img[0], img[1] ) );
      to_pts.push_back( vgl_homg_point_2d<double>( wld[0], wld[1] ) );
    }
  }

  vgl_h_matrix_2d<double> refined_H;
  vgl_h_matrix_2d_optimize_lmq opt( forw_H_ );
  opt.set_verbose( false );
  if( opt.optimize( from_pts, to_pts, refined_H ) )
  {
    forw_H_ = refined_H;
  }
}


} // end namespace vidtk
