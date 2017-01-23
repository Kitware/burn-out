/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_holder_process.h"

#include <vector>
#include <vnl/vnl_double_3x3.h>
#include <utilities/timestamp.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_homography_holder_process_cxx__
VIDTK_LOGGER("homography_holder_process_cxx");


namespace vidtk
{

homography_holder_process
::homography_holder_process( std::string const& _name )
 : process( _name, "homography_holder_process" )
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
    homography::transform_t H;
    H.set_identity();

    bool has_user_value = false;
    blk.get_has_user_value( std::string("matrix"), has_user_value );

    if( has_user_value )
    {
      vnl_double_3x3 mat;
      mat = blk.get<vnl_double_3x3>( "matrix" );
      H.set( mat );
    }
    else
    {
      double s = blk.get<double>( "scale" );
      H.set_scale( s );

      double t[2];
      blk.get( "translation", t);
      H.set_translation( t[0], t[1] );
    }

    unsigned int frame_num = blk.get<unsigned>("reference_image:frame_number" );

    double time_in_secs = blk.get<double>("reference_image:time_in_secs" );

    timestamp ref_ts( time_in_secs * 1e6, frame_num );

    this->H_ref2wld_.set_transform( H );
    this->H_ref2wld_.set_source_reference( ref_ts );
    // No data required for the world plane.

    this->config_.update( blk );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );
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

image_to_plane_homography
homography_holder_process
::homography_ref_to_wld()
{
  return this->H_ref2wld_;
}

} //namespace
