/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "transform_image_object_process.h"

#include <utilities/log.h>

namespace vidtk
{

template< class PIXEL_TYPE >
transform_image_object_process< PIXEL_TYPE >
::transform_image_object_process( vcl_string const & name,
                                  function_ptr fun )
  : process( name, "transform_image_object_process" ),
    config_(),
    disabled_(true),
    function_(fun),
    img_(NULL),
    in_objs_(NULL),
    out_objs_(0)
{
  if(function_)
  {
    config_ = function_->params();
  }
  config_.add_parameter("disabled", "true", "Whether or not the process is disabled");
}
template< class PIXEL_TYPE >
config_block
transform_image_object_process< PIXEL_TYPE >
::params() const
{
  return config_;
}

template< class PIXEL_TYPE >
bool
transform_image_object_process< PIXEL_TYPE >
::set_params( config_block const& blk )
{
  try
  {
    // Always read the disabled parameter first.  This way other parameters can be
    // skipped if the process is disabled
    disabled_ = blk.get<bool>("disabled");
    if( !disabled_ )
    {
      if(function_ && !function_->set_params(blk))
      {
        return false;
      }
    }
  }
  catch( const config_block_parse_error& e )
  {
    log_error( name() << ": Unable to set parameters: " << e.what() << vcl_endl );
    return false;
  }
  config_.update( blk );
  return true;
}

template< class PIXEL_TYPE >
bool
transform_image_object_process< PIXEL_TYPE >
::initialize()
{
  in_objs_ = NULL;
  out_objs_.clear();
  img_ = NULL;
  if(function_ == NULL)
  {
    return false;
  }
  return true;
}

template< class PIXEL_TYPE >
bool
transform_image_object_process< PIXEL_TYPE >
::step()
{
  if(function_ == NULL)
  {
    return false;
  }

  out_objs_.clear();
  if(disabled_)
  {
    if(in_objs_)
    {
      out_objs_ = *in_objs_;
      in_objs_ = NULL;
    }
    return true;
  }
  if(!in_objs_ || !function_->set_input(img_))
  {
    log_error( this->name() << ": Required inputs not available.\n" );
    return false;
  }

  out_objs_.reserve( in_objs_->size() );
  for( vcl_vector< image_object_sptr >::const_iterator o = in_objs_->begin(); o != in_objs_->end(); ++o )
  {
    image_object_sptr r = (*function_)(*o);
    if(r)
    {
      out_objs_.push_back(r);
    }
  }
  in_objs_ = NULL;

  return true;
}

template< class PIXEL_TYPE >
void
transform_image_object_process< PIXEL_TYPE >
::set_image( vil_image_view<PIXEL_TYPE> const& img )
{
  img_ = &img;
}

template< class PIXEL_TYPE >
void
transform_image_object_process< PIXEL_TYPE >
::set_objects( vcl_vector< image_object_sptr > const& objs )
{
  in_objs_ = &objs;
}

template< class PIXEL_TYPE >
vcl_vector< image_object_sptr > const&
transform_image_object_process< PIXEL_TYPE >
::objects() const
{
  return out_objs_;
}

}
