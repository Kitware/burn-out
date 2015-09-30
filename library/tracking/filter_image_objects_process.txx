/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_image_objects_process.h"

#include <boost/bind.hpp>

#include <vcl_algorithm.h>
#include <vgl/vgl_area.h>

#include <tracking/tracking_keys.h>
#include <tracking/ghost_detector.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{

template <class PixType>
filter_image_objects_process< PixType >
::filter_image_objects_process( vcl_string const& name )
  : process( name, "filter_image_objects_process" ),
    grad_ghost_detector_( NULL ),
    src_objs_( NULL ),
    filter_used_in_track_( false ),
    used_in_track_( false ),
    ghost_gradients_( false ),
    min_ghost_var_( 0 ),
    pixel_padding_( 0 )
{
  config_.add( "disabled", "false" );
  config_.add( "min_area", "-1" );
  config_.add( "max_area", "-1" );
  config_.add( "max_aspect_ratio", "-1" );
  config_.add( "min_occupied_bbox", "-1" );

  config_.add_parameter( "filter_used_in_track",
    "false",
    "Whether to filter based on *used_in_track* field or not?" );

  config_.add_parameter( "used_in_track",
    "true",
    "Remove objects with specified (true/false) used_in_track key value.");

  config_.add_parameter( "ghost_detection:enabled",
    "false",
    "Remove objects that show gradient signature of a ghost/shadow detection.");

  config_.add_parameter( "ghost_detection:min_grad_mag_var",
    "15.0",
    "Threshold below which MODs are labelled as ghosts. See "
    "tracking/ghost_detector.h for detailed comments." );

  config_.add_parameter( "ghost_detection:pixel_padding",
    "2",
    "Thickness of pixel padding around the MOD." );

}


template <class PixType>
filter_image_objects_process< PixType >
::~filter_image_objects_process()
{
  delete grad_ghost_detector_;
}


template <class PixType>
config_block
filter_image_objects_process< PixType >
::params() const
{
  return config_;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "min_area", min_area_ );
    blk.get( "max_area", max_area_ );
    blk.get( "max_aspect_ratio", max_aspect_ratio_ );
    blk.get( "min_occupied_bbox", min_occupied_bbox_ );
    blk.get( "filter_used_in_track", filter_used_in_track_ );
    blk.get( "used_in_track", used_in_track_ );
    blk.get( "disabled", disabled_ );
    blk.get( "ghost_detection:enabled", ghost_gradients_ );
    blk.get( "ghost_detection:min_grad_mag_var", min_ghost_var_ );
    blk.get( "ghost_detection:pixel_padding", pixel_padding_ );
  }
  catch( unchecked_return_value& )
  {
    log_error (name() << ": set_params() failed\n");
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::initialize()
{
  if( ghost_gradients_ )
  {
    grad_ghost_detector_ = new ghost_detector< PixType >( min_ghost_var_,
                                                          pixel_padding_ );
  }

  return true;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::reset()
{
  delete grad_ghost_detector_;
  return this->initialize();
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::step()
{
  if( disabled_ ) return true;

  log_assert( src_objs_ != NULL, "Source objects not set" );
  log_assert( !ghost_gradients_ || src_img_, "Source image required when "
    "performing ghost detection.\n" );

  objs_ = *src_objs_;
  vcl_vector< image_object_sptr >::iterator begin = objs_.begin();
  vcl_vector< image_object_sptr >::iterator end = objs_.end();

  if( min_area_ >= 0.0 )
  {
    end = vcl_remove_if( begin, end,
                         boost::bind( &self_type::filter_out_min_area, this, _1 ) );
  }

  if( max_area_ >= 0.0 )
  {
    end = vcl_remove_if( begin, end,
                         boost::bind( &self_type::filter_out_max_area, this, _1 ) );
  }

  if( max_aspect_ratio_ >= 0.0 )
  {
    end = vcl_remove_if( begin, end,
                         boost::bind( &self_type::filter_out_max_aspect_ratio, this, _1 ) );
  }

  if( min_occupied_bbox_ >= 0.0 )
  {
    end = vcl_remove_if( begin, end,
                         boost::bind( &self_type::filter_out_min_occupied_bbox, this, _1 ) );
  }

  if( filter_used_in_track_ )
  {
    end = vcl_remove_if( begin, end,
      boost::bind( &self_type::filter_out_used_in_track, this, _1 ) );
  }

  if( ghost_gradients_ )
  {
    end = vcl_remove_if( begin, end,
      boost::bind( &self_type::filter_out_ghost_gradients, this, _1 ) );
  }

  // Actually remove the filtered out elements.
  objs_.erase( end, objs_.end() );

  src_objs_ = NULL; // prepare for the next set

  return true;
}



template <class PixType>
void
filter_image_objects_process< PixType >
::set_source_objects( vcl_vector< image_object_sptr > const& objs )
{
  src_objs_ = &objs;
}

template <class PixType>
void
filter_image_objects_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  src_img_ = &img;
}

template <class PixType>
vcl_vector< image_object_sptr > const&
filter_image_objects_process< PixType >
::objects() const
{
  return objs_;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_min_area( image_object_sptr const& obj ) const
{
  return obj->area_ < min_area_;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_max_area( image_object_sptr const& obj ) const
{
  return obj->area_ > max_area_;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_max_aspect_ratio( image_object_sptr const& obj ) const
{
  double maxWH = vcl_max( obj->bbox_.width(), obj->bbox_.height() );
  double minWH = vcl_min( obj->bbox_.width(), obj->bbox_.height() );

  if( minWH == 0 )
  {
    return true;
  }
  if( ( maxWH / minWH ) > max_aspect_ratio_ )
  {
    return true;
  }

  return false;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_min_occupied_bbox( image_object_sptr const& obj ) const
{
  //Both areas have to be in pixels or meters.
  double area_ratio = vgl_area( obj->boundary_ ) / obj->bbox_.area();

  return area_ratio < min_occupied_bbox_;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_used_in_track( image_object_sptr const& obj ) const
{
  //Remove the object if it matches the specified (used_in_track_) criteria
  return (obj->data_.is_set( tracking_keys::used_in_track ) == used_in_track_);
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_ghost_gradients( image_object_sptr const& obj ) const
{
  //Remove the object if gradient signature matches that of a ghost/shadow MOD.
  return ( grad_ghost_detector_->has_ghost_gradients( obj, *src_img_ ) );
}

} // end namespace vidtk
