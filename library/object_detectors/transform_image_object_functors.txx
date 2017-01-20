/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "transform_image_object_functors.h"

#include <tracking_data/image_object_util.h>
#include <tracking_data/tracking_keys.h>

#include <vil/vil_image_resource.h>
#include <vil/vil_new.h>

namespace vidtk
{

// ------------------------------------------------------------------
template< class PIXEL_TYPE >
add_image_clip_to_image_object< PIXEL_TYPE >
::add_image_clip_to_image_object( )
  : buffer_(0),
    img_(NULL)
{
}


// ------------------------------------------------------------------
template< class PIXEL_TYPE >
config_block
add_image_clip_to_image_object< PIXEL_TYPE >
::params() const
{
  config_block r;
  r.add_parameter("buffer_size", "1", "Number pixels added arround the bbox");
  return r;
}


// ------------------------------------------------------------------
template< class PIXEL_TYPE >
bool
add_image_clip_to_image_object< PIXEL_TYPE >
::set_params(config_block const & blk)
{
  // If an exception is thrown from here it is caught in the calling
  // scope. This is why we do not have a catch here.
  buffer_ = blk.get<unsigned int>("buffer_size");
  return true;
}


// ------------------------------------------------------------------
template< class PIXEL_TYPE >
bool
add_image_clip_to_image_object< PIXEL_TYPE >
::set_input( vil_image_view<PIXEL_TYPE> const * in)
{
  if(in == NULL)
  {
    return false;
  }
  img_ = in;
  return true;
}


// ------------------------------------------------------------------
template < class PIXEL_TYPE >
image_object_sptr
add_image_clip_to_image_object< PIXEL_TYPE >
  ::operator()( image_object_sptr img_obj ) const
{
  if ( img_ )
  {
    vil_image_view< PIXEL_TYPE > in_data;
    in_data.deep_copy( clip_image_for_detection( *img_, img_obj, buffer_ ) );

    // Get underlying resource data for image
    vil_image_resource_sptr data = vil_new_image_resource_of_view( in_data );
    img_obj->set_image_chip( data, buffer_ );
  }
  return img_obj;
}

}// namespace vidtk
