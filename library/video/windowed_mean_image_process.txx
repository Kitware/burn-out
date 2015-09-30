/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "windowed_mean_image_process.h"
#include <vnl/vnl_math.h>
#include <vil/vil_math.h>
#include <vil/vil_transform.h>
#include <vil/vil_plane.h>
#include <vil/vil_convert.h>
#include <vcl_cmath.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{


template<typename ST, typename MT>
windowed_mean_image_process<ST,MT>
::windowed_mean_image_process( vcl_string const& name )
  : process( name, "windowed_mean_image_process" ),
    radius_(0)
{
  config_.add_parameter(
    "radius", "0",
    "The radius of the window in which to compute the mean and variance. "
    "A radius of r means a window of size (2r+1)x(2r+1) centered around "
    "each pixel." );
}


template<typename ST, typename MT>
windowed_mean_image_process<ST,MT>
::~windowed_mean_image_process()
{
}


template<typename ST, typename MT>
config_block
windowed_mean_image_process<ST,MT>
::params() const
{
  return config_;
}


template<typename ST, typename MT>
bool
windowed_mean_image_process<ST,MT>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "radius", radius_ );
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


template<typename ST, typename MT>
bool
windowed_mean_image_process<ST,MT>
::initialize()
{
  return true;
}


template<typename ST, typename MT>
bool
windowed_mean_image_process<ST,MT>
::step()
{
  log_assert( in_img_ != NULL, "source image not set" );

  // copy into local variables to better allow the optimizer to
  // propagate constants.
  unsigned const rad = radius_;
  unsigned const ni = in_img_->ni();
  unsigned const nj = in_img_->nj();
  unsigned const np = in_img_->nplanes();

  if( rad == 0 )
  {
    vil_convert_cast( *in_img_, mean_img_ );
    if( ! var_img_ )
    {
      var_img_.set_size( ni, nj, np );
      var_img_.fill( 0.0 );
    }
  }

  log_assert( 2*rad+1 < ni && 2*rad+1 < nj,
              "not implemented for image size < window size" );

  mean_img_.set_size( ni, nj, np );
  var_img_.set_size( ni, nj, np );

  double area = vnl_math::sqr( 2.0*rad+1.0 );

  // Use integral images to quickly compute the local means and
  // variances.

  for( unsigned p = 0; p < np; ++p )
  {
    vil_image_view<vxl_byte> plane = vil_plane( *in_img_, p );
    vil_image_view<double> sum, sum_sq;
    vil_math_integral_sqr_image( plane, sum, sum_sq );

    for( unsigned j = rad; j < nj-rad; ++j )
    {
      for( unsigned i = rad; i < ni-rad; ++i )
      {
        unsigned const p0i = i-rad;
        unsigned const p0j = j-rad;
        unsigned const p1i = i+rad+1;
        unsigned const p1j = j+rad+1;
        mean_img_(i,j,p) = ( sum(p0i,p0j) + sum(p1i,p1j)
                             - sum(p1i,p0j) - sum(p0i,p1j) ) / area;

        var_img_(i,j,p) = ( sum_sq(p0i,p0j) + sum_sq(p1i,p1j)
                            - sum_sq(p1i,p0j) - sum_sq(p0i,p1j) ) / area
          - vnl_math::sqr( mean_img_(i,j,p) );
      }
    }

    // Fill in the border pixels:
    // Top
    for( unsigned j = 0; j < rad; ++j )
    {
      for( unsigned i = 0; i < ni; ++i )
      {
        mean_img_(i,j,p) = mean_img_(i,rad,p);
        var_img_(i,j,p) = var_img_(i,rad,p);
      }
    }
    // Bottom
    for( unsigned j = nj-rad; j < nj; ++j )
    {
      for( unsigned i = 0; i < ni; ++i )
      {
        mean_img_(i,j,p) = mean_img_(i,nj-rad-1,p);
        var_img_(i,j,p) = var_img_(i,nj-rad-1,p);
      }
    }
    // Left
    for( unsigned j = rad; j < nj-rad; ++j )
    {
      for( unsigned i = 0; i < rad; ++i )
      {
        mean_img_(i,j,p) = mean_img_(rad,j,p);
        var_img_(i,j,p) = var_img_(rad,j,p);
      }
    }
    // Right
    for( unsigned j = rad; j < nj-rad; ++j )
    {
      for( unsigned i = ni-rad; i < ni; ++i )
      {
        mean_img_(i,j,p) = mean_img_(ni-rad-1,j,p);
        var_img_(i,j,p) = var_img_(ni-rad-1,j,p);
      }
    }
  }

  in_img_ = NULL;

  return true;
}


template<typename ST, typename MT>
void
windowed_mean_image_process<ST,MT>
::set_source_image( vil_image_view<ST> const& img )
{
  in_img_ = &img;
}


template<typename ST, typename MT>
vil_image_view<double> const&
windowed_mean_image_process<ST,MT>
::mean_image() const
{
  return mean_img_;
}


template<typename ST, typename MT>
vil_image_view<double> const&
windowed_mean_image_process<ST,MT>
::variance_image() const
{
  return var_img_;
}


template<typename ST, typename MT>
vil_image_view<double>
windowed_mean_image_process<ST,MT>
::stddev_image() const
{
  vil_image_view<double> std;
  std.deep_copy( var_img_ );

  double (*sqrt)(double) = &vcl_sqrt;
  vil_transform( std, sqrt );

  return std;
}


} // end namespace vidtk
