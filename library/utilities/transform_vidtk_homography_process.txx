/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "transform_vidtk_homography_process.h"

#include <fstream>

#include <logger/logger.h>
VIDTK_LOGGER("transform_vidtk_homography_process_txx");

namespace vidtk
{

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::transform_vidtk_homography_process( std::string const& _name )
  : process( _name, "transform_vidtk_homography_process" )
{
  config_.add_optional(
    "premult_scale",
    "double.  Premultiply scaling matrix compose of this parameter the input "
    "homography by this. Since the points are multiplied on the right (y=Hx), "
    "the this premultiplcation matrix applies a transform *after* the homography "
    "is applied." );
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

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::~transform_vidtk_homography_process()
{
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
config_block
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::params() const
{
  return config_;
}



template< class S, class D >
struct transform_vidtk_homography_process_converter
{
  static bool
  set_value( S const & /*s*/, D & /*d*/ )
  {
    return false;
  }
};

template< class S >
struct transform_vidtk_homography_process_converter<S,S>
{
  static bool
  set_value( S const & s, S & d )
  {
    d = s;
    return true;
  }
};

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
bool
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::set_params( config_block const& blk )
{
  try
  {
    premult_ = blk.has( "premult_scale" );
    if( premult_ )
    {
      double v = blk.get<double>( "premult_scale" );
      vnl_matrix_fixed<double,3,3> M;
      M.fill(0.0);
      M(0,0) = M(1,1) = v;
      M(2,2) = 1;
      premult_M_.set_transform( homography::transform_t(M) );
    }

    if( blk.has( "premult_matrix" ) )
    {
      if(premult_)
      {
        throw config_block_parse_error( "Cannot use both premult_matrix and " \
                                        "premult_scaler for homography. " );
      }
      premult_ = true;
      vnl_matrix_fixed<double,3,3> v;
      v = blk.get< vnl_matrix_fixed<double,3,3> >( "premult_matrix");
      premult_M_.set_transform( v );
    }

    postmult_ = blk.has( "postmult_matrix" );
    if( postmult_ )
    {
      vnl_matrix_fixed<double,3,3> v;
      v = blk.get< vnl_matrix_fixed<double,3,3> >( "postmult_matrix");
      postmult_M_.set_transform( v );
    }

    premult_file_ = blk.has( "premult_filename" );
    if( premult_file_ )
    {
      if( premult_ )
      {
        throw config_block_parse_error(
          "Cannot use both premult_matrix and " \
          "premult_filename: \"" + premult_filename_ + "\" for homography." );
      }
      premult_ = premult_file_;
      premult_filename_ = blk.get<std::string>( "premult_filename" );
    }
  }
  catch( config_block_parse_error const &e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
bool
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::initialize()
{
  std::ifstream pmat_str;
  if(!premult_)
  {
    premult_M_.set_identity( false ); //THIS MIGHT NEED TO REMOVED
  }

  if( premult_file_ )
  {
    pmat_str.open( premult_filename_.c_str() );
    if( ! pmat_str )
    {
      LOG_ERROR( "Couldn't open \"" << premult_filename_
                 << "\" for reading. " );

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
            LOG_WARN( "Failed to load pmatrix file." );
            return false;
          }
        }
      }
      premult_M_.set_transform( homography::transform_t(M) ); //deep copy.
    }
  }

  return true;
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
bool
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::step()
{
  if(!inp_H_)
  {
    return false;
  }

  OutSrcType rSrc;
  OutDestType rDest;
  homography::transform_t rH = inp_H_->get_transform();
  bool is_valid = inp_H_->is_valid();

  if( premult_ )
  {
    rH = premult_M_.get_transform() * rH;
    rDest = premult_M_.get_dest_reference();
    is_valid = is_valid && premult_M_.is_valid();
  }
  else
  {
    bool is_set = transform_vidtk_homography_process_converter<InDestType, OutDestType>::set_value( inp_H_->get_dest_reference(), rDest );
    if(!is_set)
    {
      LOG_WARN( this->name() << ": Failed to set premult_ transformation matrix" );
    }
  }

  if( postmult_ )
  {
    rH = rH * postmult_M_.get_transform();
    rSrc = postmult_M_.get_source_reference();
    is_valid = is_valid && postmult_M_.is_valid();
  }
  else
  {
    bool is_set = transform_vidtk_homography_process_converter<InSrcType,OutSrcType>::set_value( inp_H_->get_source_reference(), rSrc );
    if(!is_set)
    {
      LOG_WARN( this->name() << ": Failed to set postmult_ transformation matrix" );
    }
  }

  out_H_.set_transform(rH);
  out_H_.set_source_reference(rSrc);
  out_H_.set_dest_reference(rDest);
  out_H_.set_valid(is_valid);
  out_inv_H_ = out_H_.get_inverse();

  // Mark the homography as being "used".
  inp_H_ = NULL;

  return true;
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
void
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::set_premult_homography( pre_homog_t const& M )
{
  premult_ = true;
  premult_M_ = M;
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
vgl_h_matrix_2d<double>
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::get_premult_homography() const
{
  return premult_M_.get_transform();
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
void
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::set_postmult_homography( post_homog_t const& M )
{
  postmult_ = true;
  postmult_M_ = M;
}


template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
void
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::set_source_homography( src_homog_t const& H )
{
  inp_H_ = &H;
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
typename transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >::out_homog_t
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::homography() const
{
  return out_H_;
}

template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
typename transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >::inv_out_homog_t
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::inv_homography() const
{
  return out_inv_H_;
}


template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
typename transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >::out_homog_t::transform_t
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::bare_homography() const
{
  return out_H_.get_transform();
}


template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
typename transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >::inv_out_homog_t::transform_t
transform_vidtk_homography_process< InSrcType, InDestType, OutSrcType, OutDestType >
::inv_bare_homography() const
{
  return out_inv_H_.get_transform();
}

} // end namespace vidtk
