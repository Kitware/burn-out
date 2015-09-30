/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_fstream.h>
#include <tracking/project_to_world_process.h>
#include <utilities/unchecked_return_value.h>
#include <vnl/vnl_vector_fixed.h>
#include <utilities/log.h>
#include <vnl/vnl_inverse.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_2.h>

#include <vcl_fstream.h>

namespace vidtk
{


project_to_world_process
::project_to_world_process( vcl_string const& name )
  : process( name, "project_to_world_process" )
{
  config_.add_optional( "pmatrix_filename", 
    "This file contains computed 3x4 (world2image) projection matrix." );
  config_.add( "pmatrix", "1 0 0 0  0 1 0 0  0 0 0 1" );
  config_.add( "image_shift", "0 0" );
  config_.add( "update_area", "false" );
}


project_to_world_process
::~project_to_world_process()
{
}


config_block
project_to_world_process
::params() const
{
  return config_;
}


bool
project_to_world_process
::set_params( config_block const& blk )
{
  blk.get( "pmatrix", P_matrix_ );
  blk.get( "update_area", update_area_ );
  blk.get( "image_shift", image_shift_ );

  try
  {
    has_pmatrix_filename_ = blk.has( "pmatrix_filename" );
    if( has_pmatrix_filename_ )
    {
      blk.get( "pmatrix_filename", pmatrix_filename_ );    
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
project_to_world_process
::initialize()
{
  vcl_ifstream pmat_str_;
  vnl_double_3x4 *tmpM ;
  
  if( has_pmatrix_filename_ )
  {
    pmat_str_.open( pmatrix_filename_.c_str() );
    if( ! pmat_str_ )
    {
      log_error( "Couldn't open \"" << pmatrix_filename_
                 << "\" for reading, using Pmatrix instead.\n" );

      tmpM = &P_matrix_;
    }
    else
    {
      vnl_double_3x4 M;
      for( unsigned i = 0; i < 3; ++i )
      {
        for( unsigned j = 0; j < 4; ++j )
        {
          if( ! (pmat_str_ >> M(i,j)) )
          {
            log_warning( "Failed to load pmatrix file.\n" );
            return false;
          }
        }
      }
      tmpM = &M;
    }
  }
  else
  {
    tmpM = &P_matrix_;
  }

  //TODO: Check for a 4x3 forw_cam_proj_ instead of a 3x3 one.
  forw_cam_proj_.set_column(0, tmpM->get_column(0) );
  forw_cam_proj_.set_column(1, tmpM->get_column(1) );
  forw_cam_proj_.set_column(2, tmpM->get_column(3) );  

  back_cam_proj_ = vnl_inverse( forw_cam_proj_ );

  back_proj_ = back_cam_proj_;

  return true;
}


bool
project_to_world_process
::step()
{
  typedef vcl_vector< image_object_sptr >::iterator iter_type;

  if( image_shift_[0] != 0 || image_shift_[1] != 0 )
  {
    for( iter_type it = objs_.begin(), end = objs_.end();
         it != end; ++it )
    {
      (*it)->image_shift( image_shift_[0], image_shift_[1] );
    }
  }

  for( iter_type it = objs_.begin(), end = objs_.end();
       it != end; ++it )
  {
    image_object& obj = **it;

    back_project( obj.img_loc_, obj.world_loc_ );

    if( update_area_ )
    {
      this->update_area( obj );
    }
  }

  return true;
}


void
project_to_world_process
::set_source_objects( vcl_vector< image_object_sptr > const& objs )
{
  objs_ = objs;
}


vcl_vector< image_object_sptr > const&
project_to_world_process
::objects() const
{
  return objs_;
}


void
project_to_world_process
::back_project( vnl_vector_fixed<double,2> const& img_loc,
                vnl_vector_fixed<double,3>& world_loc ) const
{
  vnl_double_3 ip( img_loc[0], img_loc[1], 1.0 );
  vnl_double_3 wp = back_proj_ * ip;
  world_loc[0] = wp[0] / wp[2];
  world_loc[1] = wp[1] / wp[2];
  world_loc[2] = 0;
}


/// Compute the "area" of the object by assuming the the object is
/// a flat plane represented by the bounding box, and that the
/// plane is perpendicular to the ground plane.  Assume the pixels
/// are square.  Assume that the base the the bounding box is on
/// the ground plane.  Assume that the pixel areas (in world units)
/// is constant around the object.
void
project_to_world_process
::update_area( image_object& obj )
{
  // Find the world length of the base of the bounding box to
  // determine the sizes of pixels in the real world at that
  // location.
  vnl_double_3 x0, x1;
  back_project( vnl_double_2( obj.bbox_.min_x(), obj.bbox_.max_y() ),
                x0 );
  back_project( vnl_double_2( obj.bbox_.max_x(), obj.bbox_.max_y() ),
                x1 );

  double factor = (x1-x0).magnitude() / obj.bbox_.width();
  obj.area_ = obj.bbox_.area() * factor * factor;
}


void
project_to_world_process
::set_image_to_world_homography( vgl_h_matrix_2d<double> const& H )
{
  back_proj_ = back_cam_proj_ * H.get_matrix();
}

vnl_double_3x3 const&
project_to_world_process
::image_to_world_homography()
{
  return back_proj_;
}


} // end namespace vidtk
