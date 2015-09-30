/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "shot_detection_process.h"

#include <utilities/config_block.h>
#include <utilities/log.h>
#include <vcl_sstream.h>

#include <vnl/algo/vnl_svd_fixed.h>
#include <vnl/algo/vnl_determinant.h>

namespace vidtk
{

class shot_detection_impl
{
public:
  unsigned last_active_tracks_size;

  // configuration parameters
  config_block config;
  float max_frac_active_tracks;
  float max_frac_terminated_tracks;

  // I/O data
  vgl_h_matrix_2d< double > const * H_src2ref;
  vcl_vector< klt_track_ptr >  const * active_tracks;
  vcl_vector< klt_track_ptr >  const * terminated_tracks;
  
  shot_detection_impl()
    : last_active_tracks_size( 0 ),
      max_frac_active_tracks( 0.2 ),
      max_frac_terminated_tracks( 0.4 )
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

    // Setting default values
    config.get( "max_frac_active_tracks", max_frac_active_tracks );
    config.get( "max_frac_terminated_tracks", max_frac_terminated_tracks );

  } // shot_detection_impl()

  bool set_params()
  {
    // read user value(s)
    config.get( "max_frac_active_tracks", max_frac_active_tracks );
    config.get( "max_frac_terminated_tracks", max_frac_terminated_tracks );

    return true;
  } // set_params()

  void reset_inputs()
  {
    H_src2ref = NULL;
    active_tracks = NULL;
    terminated_tracks = NULL;
  } // void clear_inputs()

  bool is_end_detected( vcl_stringstream & info_ss )
  {
    // Criterion # 1
    if( terminated_tracks != NULL && active_tracks != NULL )
    {
//#ifdef 1PRINT_DEBUG
      vcl_cout<< " shot_detection... active: " << active_tracks->size() 
              << " last active: "  << last_active_tracks_size 
              << " terminated: " << terminated_tracks->size()
              << vcl_endl;
//#endif

      if( last_active_tracks_size != 0 ) 
      {
        float frac_active_tracks = (float(last_active_tracks_size) - active_tracks->size())
                                   / float( last_active_tracks_size );
        int prev_last_active_tracks_size = last_active_tracks_size;
        last_active_tracks_size = active_tracks->size();

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
    // Criterion # 3
    // If the homography matrix is singular and ill-conditioned. 
    if( H_src2ref ) 
    {
      double det_epsilon = 1e-6;
      if( vcl_abs( vnl_determinant( H_src2ref->get_matrix() ) ) < det_epsilon )
      {
        info_ss << "Found end of shot due to singular homography (through "
                << "determinant "
                << vcl_abs( vnl_determinant( H_src2ref->get_matrix() ) ) << ").";
        return true;
      }
    }

    // Criterion # 4
    // If the homography matrix is singular and ill-conditioned. 
    // Tested through the condition number, which uses SVD and 
    // could be computationally more expensive. 
    // Note that vnl_svd::well_condition() returns the inverse of 
    // the condition number, therefore the returned value will 
    // be close to 0 in case of a singular matrix.
//     vnl_svd_fixed<double, 3, 3> H( H_src2ref->get_matrix() );
//     double cond_epsilon = 1e-5;
//     vcl_cout << "Testing condition number H:\n" << H_src2ref->get_matrix() << "COND:," << vcl_abs( H.well_condition() ) << vcl_endl;
//     if( vcl_abs( H.well_condition() ) < cond_epsilon )
//     {
//       info_ss << "Found end of shot due to singular homography (through "
//               << "condition number "
//               << vcl_abs( H.well_condition() ) << ")."; 
//       return true;
//     }

    return false;
  } // bool is_end_detected()
};

shot_detection_process
::shot_detection_process( vcl_string const& name )
  : process( name, "shot_detection_process" ),
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
      log_info( this->name() << ": input data not available.\n" );
      return false;
  }

  is_end_of_shot_ = false;

  vcl_stringstream info_ss;
  if( impl_->is_end_detected( info_ss ) )
  {
    log_info( this->name() << ": " << info_ss.str() << "\n" );
    is_end_of_shot_ = true;
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
::set_active_tracks( vcl_vector< klt_track_ptr > const& trks )
{
  impl_->active_tracks = &trks;
}


void
shot_detection_process
::set_terminated_tracks( vcl_vector< klt_track_ptr > const& trks )
{
  impl_->terminated_tracks = &trks;
}

void
shot_detection_process
::set_src_to_ref_vidtk_homography( image_to_image_homography const& H)
{
  impl_->H_src2ref = &H.get_transform();
}

} // end namespace vidtk
