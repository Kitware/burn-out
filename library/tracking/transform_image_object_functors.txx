/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "transform_image_object_functors.h"

#include <tracking/tracking_keys.h>

namespace vidtk
{

template< class PIXEL_TYPE >
add_image_clip_to_image_object< PIXEL_TYPE >
::add_image_clip_to_image_object( )
  : buffer_(0),
    img_(NULL)
{
}

template< class PIXEL_TYPE >
config_block
add_image_clip_to_image_object< PIXEL_TYPE >
::params() const
{
  config_block r;
  r.add_parameter("buffer_size", "1", "Number pixels added arround the bbox");
  return r;
}

template< class PIXEL_TYPE >
bool
add_image_clip_to_image_object< PIXEL_TYPE >
::set_params(config_block const & blk)
{
  try
  {
    buffer_ = blk.get<unsigned int>("buffer_size");
  }
  catch( const config_block_parse_error& e )
  {
    throw config_block_parse_error( e );
    return false;
  }
  return true;
}

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

template< class PIXEL_TYPE >
image_object_sptr
add_image_clip_to_image_object< PIXEL_TYPE >
::operator()( image_object_sptr in )const
{
  if(img_)
  {
    vil_image_view<PIXEL_TYPE> data;
    data.deep_copy( in->templ(*img_, buffer_) );
    in->data_.set( vidtk::tracking_keys::pixel_data, data );
    in->data_.set( vidtk::tracking_keys::pixel_data_buffer,
                  buffer_ );
  }
  return in;
}

}// namespace vidtk