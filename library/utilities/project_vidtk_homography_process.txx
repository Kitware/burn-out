/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
/// Implementation providing projection of homographies.

#include "project_vidtk_homography_process.h"

#include <vnl/vnl_inverse.h>
#include <fstream>
#include <typeinfo>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_project_vidtk_homography_process_txx__
VIDTK_LOGGER("project_vidtk_homography_process");


namespace vidtk
{

template< class ST, class DT >
project_vidtk_homography_process< ST, DT >
::project_vidtk_homography_process( std::string const & _name )
  : process( _name, "project_vidtk_homography_process" ),
    project_( false )
{
  config_.add_parameter(
    "scale",
    "1.0",
    "a scalar. A projection matrix P is prepared as a scaling transform with \
    this scale." );

  config_.add_parameter(
    "matrix",
    "1 0 0 0 1 0 0 0 1",
    "3x3 matrix.  Let this matrix be P and input homography be H,"
    " the operation performed will be P^-1 * H * P. This operation can be useful"
    " to projecting a homography in a different coordinate system, e.g. cropping"
    " operation. For now only pre/post or projection matrix can be used at a time.");

  P_.set_identity();
  P_inv_.set_identity();
}

template< class ST, class DT >
project_vidtk_homography_process< ST, DT >
::~project_vidtk_homography_process()
{
}

template< class ST, class DT >
config_block
project_vidtk_homography_process< ST, DT >
::params() const
{
  return config_;
}

template< class ST, class DT >
bool
project_vidtk_homography_process< ST, DT >
::set_params( config_block const& blk )
{
  this->project_ = false;
  try
  {
    double v = blk.get< double >( "scale" );
    if( v != 1.0 ) // Just for optimization
    {
      P_.set_identity();
      P_.set_scale( v );
      this->project_ = true;
    }

    double m[9];
    blk.get( "matrix" , m); // TODO: Change to the templated type.
    vnl_matrix_fixed<double,3,3> M;
    M.copy_in( &m[0] );
    if( !M.is_identity() ) // Just for optimization
    {
      P_ = M;

      if (this->project_)
      {
        throw config_block_parse_error( " invalid scaling parameters provided (only one of scale or matrix should be set)." );
      }

      this->project_ = true;
    }

    P_inv_ = P_.get_inverse();

  }
  catch( config_block_parse_error const & e )
  {
    LOG_ERROR( name() << ": problems setting parameters: "<< e.what() );
    return false;
  }

  return true;
}

template< class ST, class DT >
bool
project_vidtk_homography_process< ST, DT >
::initialize()
{
  got_homog_ = false;

  return true;
}

template< class ST, class DT >
bool
project_vidtk_homography_process< ST, DT >
::reset()
{
  got_homog_ = false;

  return true;
}

template< class ST, class DT >
bool
project_vidtk_homography_process< ST, DT >
::step()
{
  if (!got_homog_)
  {
    return false;
  }

  got_homog_ = false;
  output_H_ = input_H_;
  if( project_ )
  {
    // Currently this mode is only supporting P through a config parameter,
    // not through the input port where it could be different on every frame.
    // We can extend this support as the need arises.
    output_H_.set_transform( P_inv_ * input_H_.get_transform() * P_ );
  }

  return true;
}

/// ------------------- Input port -------------------------------

template< class ST, class DT >
void
project_vidtk_homography_process< ST, DT >
::set_source_homography( homog_t const& H )
{
  input_H_ = H;
  got_homog_ = true;
}

  /// ------------------- Output port -------------------------------

template< class ST, class DT >
typename project_vidtk_homography_process< ST, DT >::homog_t
project_vidtk_homography_process< ST, DT >
::homography() const
{
  return output_H_;
}

//---------------------------------------------------------------------------//

} // end namespace vidtk
