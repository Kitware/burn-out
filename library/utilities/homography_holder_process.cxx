/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_holder_process.h"

#include <vcl_vector.h>
#include <vnl/vnl_double_3x3.h>
#include <utilities/timestamp.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{

homography_holder_process
::homography_holder_process( vcl_string const& name )
 : process( name, "homography_holder_process" )
{
  config_.add_parameter( "matrix", 
    "1 0 0  0 1 0  0 0 1",
    "Full 3x3 homography." );

  config_.add_parameter( "scale", 
    "1.0",
    "Scale (same for both x and y) applied to the \"matrix\"." );

  config_.add_parameter( "translation", 
    "0 0",
    "Translation [x y] vector replaces (0,2) & (1,2) elements in \"matrix\"." );

  config_.add_parameter( "reference_image:frame_number", 
    "0",
    "Frame number (unsigned int) identifying the reference frame in H_ref2wld." );

  config_.add_parameter( "reference_image:time_in_secs", 
    "0.0",
    "Timestamp (double for seconds) identifying the reference frame in H_ref2wld." );
}

homography_holder_process
::~homography_holder_process()
{
}

config_block
homography_holder_process
::params() const
{
  return config_;
}

bool
homography_holder_process
::set_params( config_block const& blk )
{
  try
  {
    vnl_double_3x3 mat;
    blk.get( "matrix", mat);
    homography::transform_t H( mat );

    //double s;
    //blk.get( "scale", s);
    //H.set_scale( s );

    //double t[2];
    //blk.get( "translation", t);
    //H.set_translation( t[0], t[1] );
    
    unsigned int frame_num;
    blk.get ("reference_image:frame_number", frame_num );
    
    double time_in_secs;
    blk.get ("reference_image:time_in_secs", time_in_secs );

    timestamp ref_ts( time_in_secs * 1e6, frame_num );

    this->H_ref2wld_.set_transform( H );
    this->H_ref2wld_.set_source_reference( ref_ts );
    // No data required for the world plane.

    this->config_.update( blk );
  }
  catch( unchecked_return_value & e )
  {
    log_error( this->name() << ": couldn't set parameters: "<< e.what() <<"\n" );
    return false;
  }

  return true;
}

bool
homography_holder_process
::initialize()
{
  return true;
}

bool
homography_holder_process
::step()
{
  return true;
}

image_to_plane_homography const& 
homography_holder_process
::homography_ref_to_wld()
{
  return this->H_ref2wld_;
}

} //namespace