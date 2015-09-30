/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// IMPORTANT NOTE: 
///     This class is deprecated. Pleaes use compute_object_features_process
///     instead. In addition, image_color_histogram and image_color_histogram2
///     are also depcrecated. Please use image_histogram instead. 

#include "compute_color_histogram_process.h"

#include <boost/bind.hpp>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <utilities/image_histogram.h>
#include <tracking/image_object.h>
#include <tracking/tracking_keys.h>


namespace vidtk
{

template < class PixType >
compute_color_histogram_process<PixType>
::compute_color_histogram_process( vcl_string const& name )
  : process( name, "compute_color_histogram_process" ),
    src_objs_( NULL ),
    image_( NULL ),
    config_(),
    objs_()
{
}


template < class PixType >
compute_color_histogram_process<PixType>
::~compute_color_histogram_process()
{
}


template < class PixType >
config_block
compute_color_histogram_process<PixType>
::params() const
{
  return config_;
}


template < class PixType >
bool
compute_color_histogram_process<PixType>
::set_params( config_block const& blk )
{
  config_.update( blk );
  return true;
}


template < class PixType >
bool
compute_color_histogram_process<PixType>
::initialize()
{
  return true;
}


template < class PixType >
bool
compute_color_histogram_process<PixType>
::step()
{
  typedef image_histogram<vxl_byte, bool> THistogram;

  log_assert( src_objs_ != NULL, "Source objects not set" );
  log_assert( image_ != NULL, "Image is null" );

  objs_ = *src_objs_;

  //compute color histogram for all MODs.
  for(unsigned i = 0; i < objs_.size(); i++)
  {
    vil_image_view<PixType> im_chip =
      objs_[i]->templ( *image_ );

    THistogram &hist =
      objs_[i]->data_.get_or_create_ref<THistogram>( tracking_keys::histogram );

    hist.add( im_chip, &objs_[i]->mask_ );
  }
  src_objs_ = NULL; // prepare for the next set

  return true;
}


template < class PixType >
void
compute_color_histogram_process<PixType>
::set_source_objects( vcl_vector< image_object_sptr > const& objs )
{
  src_objs_ = &objs;
}


template < class PixType >
vcl_vector< image_object_sptr > const&
compute_color_histogram_process<PixType>
::objects() const
{
  return objs_;
}

template < class PixType >
void
compute_color_histogram_process<PixType>
::set_source_image( vil_image_view<PixType> const& im )
{
  image_ = &im;
}


} // end namespace vidtk
