/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/project_to_world_process.h>

#include <tracking_data/image_object.h>

#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_inverse.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_2.h>

#include <fstream>
#include <limits>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_project_to_world_process_cxx__
VIDTK_LOGGER("project_to_world_process_cxx");


namespace vidtk
{


project_to_world_process
::project_to_world_process( std::string const& _name )
  : process( _name, "project_to_world_process" )
{
  config_.add_optional(
    "pmatrix_filename",
    "This file contains computed 3x4 (world2image) projection matrix." );
  config_.add_parameter(
    "pmatrix",
    "1 0 0 0  0 1 0 0  0 0 0 1",
    "Optional project matrix to apply to all detection locations." );
  config_.add_parameter(
    "image_shift",
    "0 0",
    "Shift to apply to image coordinates across all detections." );
  config_.add_parameter(
    "update_area",
    "true",
    "Whether or not to update the area of all detections." );
  config_.add_parameter(
    "use_gsd_scale_image",
    "false",
    "True if we want to use scaled image based upon "
    "regional GSDs." );
  config_.add_parameter(
    "location_type",
    "centroid",
    "calculate area using one of the following options: "
    "centroid: center point of gsd scale image "
    "bottom: bottom point of gsd scale image.");
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
  try
  {
    P_matrix_ = blk.get<vnl_double_3x4>( "pmatrix" );
    update_area_ = blk.get<bool>( "update_area" );
    image_shift_ = blk.get<vnl_int_2>( "image_shift" );
    use_gsd_scale_image_ = blk.get<bool>( "use_gsd_scale_image" );

    has_pmatrix_filename_ = blk.has( "pmatrix_filename" );
    if( has_pmatrix_filename_ )
    {
      pmatrix_filename_ = blk.get<std::string>( "pmatrix_filename" );
    }

    std::string loc = blk.get<std::string>( "location_type");
    if( loc == "centroid" )
    {
      loc_type_ = centroid;
    }
    else if( loc == "bottom" )
    {
      loc_type_ = bottom;
    }
    else
    {
      throw config_block_parse_error("Location type must be set to centroid or bottom.");
    }
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
project_to_world_process
::initialize()
{
  std::ifstream pmat_str_;
  vnl_double_3x4 *tmpM ;

  if( has_pmatrix_filename_ )
  {
    pmat_str_.open( pmatrix_filename_.c_str() );

    if( !pmat_str_ )
    {
      LOG_ERROR( "Couldn't open \"" << pmatrix_filename_
                 << "\" for reading, using Pmatrix instead." );

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
            LOG_WARN( "Failed to load pmatrix file." );
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
  typedef std::vector< image_object_sptr >::iterator iter_type;

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

    tracker_world_coord_type world_loc = obj.get_world_loc();
    back_project( obj.get_image_loc(), world_loc );
    obj.set_world_loc( world_loc );

    if( update_area_ )
    {
      this->update_area( obj );
    }
  }

  return true;
}


void
project_to_world_process
::set_source_objects( std::vector< image_object_sptr > const& objs )
{
  objs_ = objs;
}


std::vector< image_object_sptr >
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
  double factor;
  vgl_box_2d< unsigned > const& bbox = obj.get_bbox();
  if ( use_gsd_scale_image_ )
  {
    vidtk_pixel_coord_type img_loc = obj.get_image_loc();

    if ( loc_type_ == bottom )
    {
      img_loc[1] = bbox.max_y();
      //We've modified the loc, set in img_object
      obj.set_image_loc( img_loc );
    }

    LOG_ASSERT ( img_loc[0] < static_cast<signed>(img_.ni()), "Out of image bounds." );
    LOG_ASSERT ( img_loc[1] < static_cast<signed>(img_.nj()), "Out of image bounds." );

    if( bbox.width() > 0 )
    {
      factor = img_(img_loc[0], img_loc[1], 0);
      if ( factor == std::numeric_limits<double>::max() )
      {
        factor = 0;
      }
      if( obj.get_image_area() >= 0)
      {
        obj.set_area( obj.get_image_area() * factor * factor );
      }
      else
      {
        obj.set_area( -1 );
      }
    }
    else
    {
      obj.set_area( -1 );
    }
  }
  else
  {
    vnl_double_3 x0, x1;
    back_project( vnl_double_2( bbox.min_x(), bbox.max_y() ),
                  x0 );
    back_project( vnl_double_2( bbox.max_x(), bbox.max_y() ),
                  x1 );

    if( bbox.width() > 0 )
    {
      factor = (x1-x0).magnitude() / bbox.width();
      if( obj.get_image_area() >= 0)
      {
        obj.set_area( obj.get_image_area() * factor * factor );
        //std::ofstream outfile("file.txt", std::ios::app);
        //outfile << "Object area: " << obj.area_ << std::endl;
        //outfile.close();
      }
      else
      {
        obj.set_area( -1 );
      }
    }
    else
    {
      obj.set_area( -1 );
    }
  }
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


void
project_to_world_process
::set_gsd_image ( vil_image_view<double> const& img )
{
  img_ = img;
}

} // end namespace vidtk
