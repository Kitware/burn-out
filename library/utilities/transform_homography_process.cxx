/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "transform_homography_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vnl/vnl_inverse.h>
#include <vcl_fstream.h>

namespace vidtk
{
transform_homography_process
::transform_homography_process( vcl_string const& name )
  : process( name, "transform_homography_process" )
{
  config_.add_optional(
    "premult_scale",
    "double.  Premultiply scaling matrix compose of this parameter the input homography by this. Since the points "
    "are multiplied on the right (y=Hx), the this premultiplcation matrix "
    "applies a transform *after* the homography is applied." );
  config_.add_optional(
    "premult_matrix",
    "3x3 matrix.  Premultiply the input homography by this. Since the points "
    "are multiplied on the right (y=Hx), the this premultiplcation matrix "
    "applies a transform *after* the homography is applied." );
  config_.add_optional(
    "postmult_matrix",
    "3x3 matrix.  Postmultiply the input homography by this. Since the points "
    "are multiplied on the right (y=Hx), the this postmultiplcation matrix "
    "applies a transform *before* the homography is applied." );

  config_.add_optional( 
    "premult_filename", 
    "This file contains computed 3x3 (world2img0) homography matrix." );
}


transform_homography_process
::~transform_homography_process()
{
}


config_block
transform_homography_process
::params() const
{
  return config_;
}


bool
transform_homography_process
::set_params( config_block const& blk )
{
  try
  {
    premult_ = blk.has( "premult_scale" );
    if( premult_ )
    {
      double v;
      blk.get( "premult_scale", v );
      premult_M_.fill(0.0);
      premult_M_(0,0) = premult_M_(1,1) = v;
      premult_M_(2,2) = 1;
    }
    if( blk.has( "premult_matrix" ) )
    {
      if(premult_)
      {
        log_error( "Cannot use both premult_matrix and premult_scaler for homography. \n" );
        return false;
      }
      premult_ = true;
      double v[9];
      blk.get( "premult_matrix", v );
      premult_M_.copy_in( &v[0] );
    }

    postmult_ = blk.has( "postmult_matrix" );
    if( postmult_ )
    {
      double v[9];
      blk.get( "postmult_matrix", v );
      postmult_M_.copy_in( &v[0] );
    }

    premult_file_ = blk.has( "premult_filename" );
    if( premult_file_ )
    {
      if( premult_ ) 
      {
        log_error( "Cannot use both premult_matrix and premult_filename: \"" <<
                    premult_filename_
                   << "\" for homography. \n" );
        return false;        
      }
      premult_ = premult_file_;
      blk.get( "premult_filename", premult_filename_ );    
    }
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
transform_homography_process
::initialize()
{
  vcl_ifstream pmat_str;
  if(!premult_)
  {
    premult_M_.set_identity(); //THIS MIGHT NEED TO REMOVED
  }
  
  if( premult_file_ )
  {
    pmat_str.open( premult_filename_.c_str() );
    if( ! pmat_str )
    {
      log_error( "Couldn't open \"" << premult_filename_
                 << "\" for reading. \n" );

      return false;
    }
    else
    {
      vnl_matrix_fixed<double,3,3> M;
      for( unsigned i = 0; i < 3; ++i )
      {
        for( unsigned j = 0; j < 3; ++j )
        {
          if( ! (pmat_str >> M(i,j)) )
          {
            log_warning( "Failed to load pmatrix file.\n" );
            return false;
          }
        }
      }
      premult_M_ = M; //deep copy.
    }
  }

  return true;
}


bool
transform_homography_process
::step()
{
  log_assert( inp_H_ != NULL,
              "Source homography has not been set" );

  out_H_ = *inp_H_;

  if( premult_ )
  {
    out_H_.set( premult_M_ * out_H_.get_matrix() );
  }

  if( postmult_ )
  {
    out_H_.set( out_H_.get_matrix() * postmult_M_ );
  }

  out_inv_H_ = out_H_.get_inverse();

  // Mark the homography as being "used".
  inp_H_ = NULL;

  return true;
}


void
transform_homography_process
::set_premult_homography( vgl_h_matrix_2d<double> const& M )
{
  premult_ = true;
  premult_M_ = M.get_matrix();
}

vgl_h_matrix_2d<double>
transform_homography_process
::get_premult_homography() const
{
  return vgl_h_matrix_2d<double>(premult_M_);
}

void
transform_homography_process
::set_postmult_homography( vgl_h_matrix_2d<double> const& M )
{
  postmult_ = true;
  postmult_M_ = M.get_matrix();
}



void
transform_homography_process
::set_source_homography( vgl_h_matrix_2d<double> const& H )
{
  inp_H_ = &H;
}


vgl_h_matrix_2d<double> const&
transform_homography_process
::homography() const
{
  return out_H_;
}


vgl_h_matrix_2d<double> const&
transform_homography_process
::inv_homography() const
{
  return out_inv_H_;
}


} // end namespace vidtk
