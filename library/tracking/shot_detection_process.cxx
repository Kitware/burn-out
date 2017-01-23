/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "shot_detection_process.h"

#include <utilities/config_block.h>

#include <sstream>

#include <vnl/algo/vnl_svd_fixed.h>
#include <vnl/algo/vnl_determinant.h>
#include <vgl/vgl_vector_2d.h>

#include <logger/logger.h>

VIDTK_LOGGER("shot_detection_process");


namespace vidtk
{

class shot_detection_impl
{
public:
  unsigned last_active_tracks_size;

  timestamp src_ts;
  timestamp dst_ts;

  // configuration parameters
  config_block config;
  float max_frac_active_tracks;
  float max_frac_terminated_tracks;
  bool check_geometric_consistency_;
  double max_err_opp_sides_;
  double max_err_aspect_;

  // I/O data
  vgl_h_matrix_2d< double > const * H_src2ref;
  std::vector< klt_track_ptr >  const * active_tracks;
  std::vector< klt_track_ptr >  const * terminated_tracks;

  shot_detection_impl()
    : last_active_tracks_size( 0 ),
      max_frac_active_tracks( 0.2f ),
      max_frac_terminated_tracks( 0.4f ),
      check_geometric_consistency_( false ),
      max_err_opp_sides_( 0.4 ),
      max_err_aspect_( 0.4 )
  {
    reset_inputs();

    // Defining config parameters
    config.add_parameter( "max_frac_active_tracks",
      "0.2",
      "Maximum percentage (fraction) change in the number of activly tracked "
      "feature points (e.g. KLT) within one shot. A computed number above "
      " this threshold would contibute to declaring a new shot boundary.");

    config.add_parameter( "max_frac_terminated_tracks",
      "0.4",
      "Maximum percentage (fraction) of the number of tracked feature points "
      "(e.g. KLT) that are allowed to terminate within one shot. A computed number "
      " above this threshold would contibute to declaring a new shot boundary.");

    config.add_parameter( "check_geometric_consistency",
      "false",
      "For WAMI we need to detect bad homographies with large perspective"
      " , skew, etc." );

    config.add_parameter( "max_opp_sides_error",
      "0.4",
      "[0,1] interval. Applied only when check_geometric_consistency = true." );

    config.add_parameter( "max_aspect_error",
      "0.4",
      "[0,1] interval. Applied only when check_geometric_consistency = true." );

    // Setting default values
    max_frac_active_tracks = config.get<float>( "max_frac_active_tracks" );
    max_frac_terminated_tracks = config.get<float>( "max_frac_terminated_tracks" );

  } // shot_detection_impl()

  bool set_params()
  {
    // read user value(s)
    max_frac_active_tracks = config.get<float>( "max_frac_active_tracks" );
    max_frac_terminated_tracks = config.get<float>( "max_frac_terminated_tracks" );
    check_geometric_consistency_ = config.get<bool>( "check_geometric_consistency" );
    max_err_opp_sides_ = config.get<double>( "max_opp_sides_error" );
    max_err_aspect_ = config.get<double>( "max_aspect_error" );
    return true;
  } // set_params()

  void reset_inputs()
  {
    H_src2ref = NULL;
    active_tracks = NULL;
    terminated_tracks = NULL;
  } // void clear_inputs()

  bool is_end_detected( std::stringstream & info_ss )
  {
    if( terminated_tracks != NULL && active_tracks != NULL )
    {
//#ifdef 1PRINT_DEBUG
      LOG_INFO( " shot_detection... active: " << active_tracks->size()
              << " last active: "  << last_active_tracks_size
              << " terminated: " << terminated_tracks->size()
              );
//#endif

      if( last_active_tracks_size != 0 )
      {
        float frac_active_tracks = (float(last_active_tracks_size) - active_tracks->size())
                                   / float( last_active_tracks_size );
        int prev_last_active_tracks_size = last_active_tracks_size;
        last_active_tracks_size = active_tracks->size();

        // Criterion # 1
        if( frac_active_tracks > max_frac_active_tracks )
        {
          info_ss << "Found end of shot due to active tracks fraction: "
                  << frac_active_tracks;
          return true;
        }

        // Criterion # 2
        // If there is a large number of lost KLT points, then this is due
        // to the end of the shot.
        float frac_term_tracks = 0.0;
        frac_term_tracks = float( terminated_tracks->size() )
                          / prev_last_active_tracks_size;

        if( frac_term_tracks > max_frac_terminated_tracks )
        {
          info_ss << "Found end of shot due to terminated tracks fraction: "
                  << frac_term_tracks;
          return true;
        }
      }
      else
      {
        last_active_tracks_size = active_tracks->size();
      }
    }

    // In the instance when the input homography is available.
    if( H_src2ref )
    {
      // Criterion # 3
      // If the homography matrix is singular and ill-conditioned.
      double det_epsilon = 1e-6;
      if( std::abs( vnl_determinant( H_src2ref->get_matrix() ) ) < det_epsilon )
      {
        info_ss << "Found end of shot due to singular homography (through "
                << "determinant "
                << std::abs( vnl_determinant( H_src2ref->get_matrix() ) ) << ").";
        return true;
      }

      // Criterion # 4
      // Test non-perspective geometric consistency.
      if(check_geometric_consistency_) // check_geometric_consistency_ )
      {
        const unsigned ni = 1000, nj = 1000; // Don't need the exact frame size.
        std::vector< vgl_homg_point_2d< double > >src_pts( 4 );
        src_pts[0].set(    0, 0    ); // tl
        src_pts[1].set( ni-1, 0    ); // tr
        src_pts[2].set( ni-1, nj-1 ); // br
        src_pts[3].set(    0, nj-1 ); // bl

        std::vector< vgl_point_2d< double > >dst_pts( 4 );
        for( size_t i = 0; i < src_pts.size(); i++ )
        {
          dst_pts[i] = *H_src2ref * src_pts[i];
        }

        // CW vectors
        vgl_vector_2d<double> top     = dst_pts[1] - dst_pts[0];
        vgl_vector_2d<double> right   = dst_pts[2] - dst_pts[1];
        vgl_vector_2d<double> bottom  = dst_pts[3] - dst_pts[2];
        vgl_vector_2d<double> left    = dst_pts[0] - dst_pts[3];

        // Cached values
        const double top_width = top.sqr_length();
        const double right_height = right.sqr_length();
        const double bottom_width = bottom.sqr_length();
        const double left_height = left.sqr_length();

        // Protecting against divide by 0 (and then some)
        if( left_height < 1 || bottom_width < 1 || top_width < 1 || right_height < 1)
        {
          info_ss << "Found end of shot due to geometric inconsistentcy (tiny side) on " << src_ts;
          return true;
        }

        // Check for non-parallel sides and aspect ratio
        double horizontal_ratio = right_height / left_height;
        double vertical_ratio   = top.sqr_length() / bottom_width;

        if( std::abs( horizontal_ratio - 1.0 ) > max_err_opp_sides_ ||
            std::abs( vertical_ratio - 1.0 )   > max_err_opp_sides_ )
        {
          info_ss << "Found end of shot due to geometric inconsistentcy (non-parallel sides) on " << src_ts;
          return true;
        }

        // Protecting against unexpected 'elongation'
        double aspect_ratio_rb = right_height / bottom_width;
        if( std::abs( aspect_ratio_rb - 1.0 ) > max_err_aspect_ )
        {
          info_ss << "Found end of shot due to geometric inconsistentcy (aspect_ratio_rb) on " << src_ts;
          return true;
        }
      }
    }

    // Criterion # 5
    // If the homography matrix is singular and ill-conditioned.
    // Tested through the condition number, which uses SVD and
    // could be computationally more expensive.
    // Note that vnl_svd::well_condition() returns the inverse of
    // the condition number, therefore the returned value will
    // be close to 0 in case of a singular matrix.
//     vnl_svd_fixed<double, 3, 3> H( H_src2ref->get_matrix() );
//     double cond_epsilon = 1e-5;
//     LOG_INFO( "Testing condition number H:\n" << H_src2ref->get_matrix() << "COND:," << std::abs( H.well_condition() ) );
//     if( std::abs( H.well_condition() ) < cond_epsilon )
//     {
//       info_ss << "Found end of shot due to singular homography (through "
//               << "condition number "
//               << std::abs( H.well_condition() ) << ").";
//       return true;
//     }


    return false;
  } // bool is_end_detected()
};

shot_detection_process
::shot_detection_process( std::string const& _name )
  : process( _name, "shot_detection_process" ),
    impl_( new shot_detection_impl ),
    is_end_of_shot_( false )
{
}


shot_detection_process
::~shot_detection_process()
{
  delete impl_;
}



config_block
shot_detection_process
::params() const
{
  return impl_->config;
}



bool
shot_detection_process
::set_params( config_block const& blk )
{
  impl_->config.update( blk );
  return impl_->set_params();
}



bool
shot_detection_process
::initialize()
{
  is_end_of_shot_ = false;
  return true;
}


bool
shot_detection_process
::reset()
{
  return this->initialize();
}


bool
shot_detection_process
::step()
{
  if( impl_->H_src2ref == NULL &&
      (impl_->active_tracks == NULL || impl_->terminated_tracks == NULL) )
  {
      // Parsable message for shotbreak visualization tool
      LOG_TRACE( "frametrace: shot_detection: no-input (" << this->name() << ")" );
      return false;
  }

  is_end_of_shot_ = false;

  std::stringstream info_ss;
  if( impl_->is_end_detected( info_ss ) )
  {
    LOG_INFO( this->name() << ": " << info_ss.str() );
    is_end_of_shot_ = true;
  }

  if( impl_->H_src2ref )
  {
    LOG_DEBUG( this->name() << ": " << info_ss.str() );
  }

  if( IS_TRACE_ENABLED() )
  {
    int n_active = (impl_->active_tracks == 0) ? -1 : impl_->active_tracks->size(),
        n_terminated = (impl_->terminated_tracks ==  0) ? -1 : impl_->terminated_tracks->size();
    LOG_TRACE( "frametrace: shot_detection: src: " << impl_->src_ts.frame_number()
               << " dst: " << impl_->dst_ts.frame_number()
               << " EOS: " << is_end_of_shot_
               << " nActive: " << n_active
               << " nTerminated: " << n_terminated );
  }

  return true;
}

//------------------ input port(s)---------------------------

void
shot_detection_process
::set_src_to_ref_homography( vgl_h_matrix_2d<double> const& H )
{
  impl_->H_src2ref = &H;
}


void
shot_detection_process
::set_active_tracks( std::vector< klt_track_ptr > const& trks )
{
  impl_->active_tracks = &trks;
}


void
shot_detection_process
::set_terminated_tracks( std::vector< klt_track_ptr > const& trks )
{
  impl_->terminated_tracks = &trks;
}

void
shot_detection_process
::set_src_to_ref_vidtk_homography( image_to_image_homography const& H)
{
  impl_->H_src2ref = &H.get_transform();
  impl_->src_ts = H.get_source_reference();
  impl_->dst_ts = H.get_dest_reference();
}

} // end namespace vidtk
