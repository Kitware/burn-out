/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef synthetic_path_generator_process_txx
#define synthetic_path_generator_process_txx

#include "synthetic_path_generator_process.h"

#include <utilities/unchecked_return_value.h>

#include <vnl/vnl_int_2.h>
#include <vnl/vnl_double_3.h>

#include <vgl/vgl_convex.h>
#include <vgl/vgl_area.h>

namespace vidtk
{

template <class PathFunction>
synthetic_path_generator_process<PathFunction>
::synthetic_path_generator_process( vcl_string const& name )
  : process( name, "synthetic_path_generator_process" ),
    rand_(1000),
    time_step_(1),
    number_of_time_steps_(1000),
    current_time_(0),
    std_dev_(0.25),
    path_(), image_(5400,5400,3),
    frame_rate_set_(false), frame_rate_(30.)
{
  image_.fill(100);
  identity_.set_identity();
  config_.add_parameter("time_step", "1", "The time step size");
  config_.add_parameter("number_of_time_steps", "1000", "The time step size");
  config_.add_parameter("image_size", "400 400", "The width and height of the fake image" );
  config_.add_parameter("std_dev", "0.25", "The standard devation of the added noise" );
  //TODO: add the optional parameter for frame rate.
  config_.add_subblock(path_.params(),path_.name());
}

template <class PathFunction>
synthetic_path_generator_process<PathFunction>
::~synthetic_path_generator_process()
{}

template <class PathFunction>
config_block
synthetic_path_generator_process<PathFunction>
::params() const
{
  return config_;
}

template <class PathFunction>
bool
synthetic_path_generator_process<PathFunction>
::set_params( config_block const& blk)
{
  try
  {
    time_step_ = blk.get<unsigned int>( "time_step" );
    number_of_time_steps_ = blk.get<unsigned int>( "number_of_time_steps" );
    vnl_int_2 size = blk.get< vnl_int_2 >( "image_size" );
    std_dev_ = blk.get< double >( "std_dev" );
    image_ = vil_image_view<vxl_byte>(static_cast<unsigned>(size[0]), static_cast<unsigned>(size[1]), 3);
    image_.fill(100);
    config_block sblk = blk.subblock( path_.name() );
    path_.set_params( sblk );
  }
  catch( unchecked_return_value & )
  {
    // restore previous state
    this->set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}

template <class PathFunction>
bool
synthetic_path_generator_process<PathFunction>
::initialize()
{
  current_correct_location_ = path_.advance(0);
  current_noisy_location_ = current_correct_location_ + vnl_double_2(rand_.normal(),rand_.normal())*std_dev_;
  return true;
}

template <class PathFunction>
bool
synthetic_path_generator_process<PathFunction>
::next_loc(vnl_double_2 & l)
{
  if(current_time_<number_of_time_steps_)
  {
    current_time_++;
    vnl_double_2 cloc = path_.advance(time_step_);
    l = cloc + vnl_double_2(rand_.normal(),rand_.normal())*std_dev_;
    current_noisy_location_ = l;
    current_correct_location_ = cloc;
    return true;
  }
  return false;
}

template <class PathFunction>
bool
synthetic_path_generator_process<PathFunction>
::current_correct_location(vnl_double_2 & l) const
{
  l = current_correct_location_;
  return (current_time_<=number_of_time_steps_);
}

template <class PathFunction>
bool
synthetic_path_generator_process<PathFunction>
::current_noisy_location(vnl_double_2 & l) const
{
  l = current_noisy_location_;
  return (current_time_<=number_of_time_steps_);
}

template <class PathFunction>
bool
synthetic_path_generator_process<PathFunction>
::step()
{
  vnl_double_2 cloc;
  objects_.clear();
  image_ = vil_image_view<vxl_byte>(image_.ni(), image_.nj(), 3);
  image_.fill(100);
  if(this->next_loc(cloc))
  {
    image_object_sptr obj_sptr = new image_object;
    image_object& obj = *obj_sptr; // to avoid the costly dereference
                                   // every time.
    vcl_vector< vgl_point_2d<double> > pts;
    for(double lx = cloc[0] - 3; lx <= cloc[0] + 3; ++lx)
    {
      for(double ly = cloc[1] - 3; ly <= cloc[1] + 3; ++ly)
      {
        image_(static_cast<unsigned int>(lx), static_cast<unsigned int>(ly)) = 255;
        image_(static_cast<unsigned int>(lx), static_cast<unsigned int>(ly),2) = 255;
        obj.bbox_.add( image_object::image_point_type( static_cast<unsigned int>(lx), static_cast<unsigned int>(ly) ) );
        pts.push_back( vgl_point_2d<double>( static_cast<unsigned int>(lx), static_cast<unsigned int>(ly) ) );
      }
    }

    // Move the max point, so that min_point() is inclusive and
    // max_point() is exclusive.
    obj.bbox_.set_max_x( obj.bbox_.max_x() + 1);
    obj.bbox_.set_max_y( obj.bbox_.max_y() + 1);

    obj.boundary_ = vgl_convex_hull( pts );
    double area = vgl_area( obj.boundary_ );
    obj.area_ = area;

    obj.mask_i0_ = obj.bbox_.min_x();
    obj.mask_j0_ = obj.bbox_.min_y();
/*    vcl_cout <<  obj.bbox_.width() << " " << obj.bbox_.height() << vcl_endl;*/
    vil_image_view<bool> mask_chip( obj.bbox_.width(), obj.bbox_.height() );
    mask_chip.fill(true);
    obj.mask_.deep_copy( mask_chip );
    obj.img_loc_ = cloc;
    obj.world_loc_ = vnl_double_3( obj.img_loc_[0], obj.img_loc_[1], 0 );

    objects_.push_back( obj_sptr );
    return true;
  }
  return false;
}

template <class PathFunction>
timestamp
synthetic_path_generator_process<PathFunction>
::timestamp() const
{
  vidtk::timestamp ts;
  ts.set_frame_number( current_time_ );
  if(frame_rate_set_)
  {
    double t = double (ts.frame_number()) / frame_rate_;
    ts.set_time( t * 1e6 );
  }
  return ts;
}

template <class PathFunction>
vil_image_view<vxl_byte> const&
synthetic_path_generator_process<PathFunction>
::image()
{
  return image_;
}

template <class PathFunction>
vcl_vector< image_object_sptr > const&
synthetic_path_generator_process<PathFunction>
::objects() const
{
  return objects_;
}

template <class PathFunction>
vgl_h_matrix_2d<double> const&
synthetic_path_generator_process<PathFunction>
::image_to_world_homography() const
{
    return identity_;
}

template <class PathFunction>
vgl_h_matrix_2d<double> const&
synthetic_path_generator_process<PathFunction>
::world_to_image_homography() const
{

    return identity_;
}

template <class PathFunction>
vnl_double_2
synthetic_path_generator_process<PathFunction>
::get_current_velocity() const
{
  return path_.get_velocity();
}

template <class PathFunction>
double
synthetic_path_generator_process<PathFunction>
::get_current_speed() const
{
  return path_.get_speed();
}

} //namespace vidtk

#endif
