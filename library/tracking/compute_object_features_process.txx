/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "compute_object_features_process.h"

#include <boost/bind.hpp>
#include <utilities/unchecked_return_value.h>
#include <utilities/image_histogram.h>
#include <tracking/image_object.h>
#include <tracking/tracking_keys.h>
#include <vil/vil_crop.h>
#include <vil/vil_math.h>
#include <vil/vil_convert.h>
#include <vcl_utility.h>

namespace vidtk
{

template < class PixType >
compute_object_features_process<PixType>
::compute_object_features_process( vcl_string const& name )
  : process( name, "compute_object_features_process" ),
    config_(),
    src_objs_( NULL ),
    image_( NULL ), //image to check, use .nplanes()
    objs_(),
    compute_mean_intensity_( false ),
    compute_histogram_( false )
{
    config_.add_parameter( "intensity_distribution", "false", 
      "Compute mean and variance of grayscale image intensity from"
      " pixels inside the bbox of the current image object.");
    config_.add_parameter( "compute_histogram", "false", 
      "Compute 3-D color or 1-D intensity histgoram from pixels inside"
      " the bbox of the current image object. Dimensionality of the "
      " histogram will be the same as the number of planes in the source"
      " image.");
}


template < class PixType >
config_block
compute_object_features_process<PixType>
::params() const
{
  return config_;
}


template < class PixType >
bool
compute_object_features_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    compute_mean_intensity_ = blk.get<bool>( "intensity_distribution" );
    compute_histogram_ = blk.get<bool>( "compute_histogram" );
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


template < class PixType >
bool
compute_object_features_process<PixType>
::initialize()
{
  return true;
}


// ----------------------------------------------------------------
/** Main process step.
 *
 *
 */
template < class PixType >
bool
compute_object_features_process<PixType>
::step()
{
  // make sure all required inputs are present
  // if not, then terminate this process.
  if ( (src_objs_ == NULL)
       || ( image_ == NULL)
    )
  {
    return false;
  }


  objs_ = *src_objs_;

  //compute color histogram for all MODs.
  for(unsigned i = 0; i < objs_.size(); i++)
  {
    if(compute_histogram_)
    {
      vil_image_view<PixType> im_chip = objs_[i]->templ( *image_ );

      image_histogram<PixType, bool> hist( im_chip, 
        vcl_vector<unsigned>( im_chip.nplanes(), 8 ) );
      objs_[i]->data_.set( tracking_keys::histogram, hist );
    }// if compute_histogram_

    if( compute_mean_intensity_ )
    {
      float mean, var;
      compute_intensity_mean_var( objs_[i], mean, var );
      objs_[i]->data_.set( tracking_keys::intensity_distribution,
                           vcl_pair<float, float>( mean, var ) );
    }
  }

  // prepare for the next set
  src_objs_ = NULL;

  return true;
}


template < class PixType >
void
compute_object_features_process<PixType>
::set_source_objects( vcl_vector< image_object_sptr > const& objs )
{
  src_objs_ = &objs;
}


template < class PixType >
vcl_vector< image_object_sptr > const&
compute_object_features_process<PixType>
::objects() const
{
  return objs_;
}

template < class PixType >
void
compute_object_features_process<PixType>
::set_source_image( vil_image_view<PixType> const& im )
{
  image_ = &im;
}


//TODO: Compute mean intensity only from the pixels inside the object mask

template< class PixType >
void
compute_object_features_process<PixType>
::compute_intensity_mean_var( image_object_sptr const & obj, 
                              float & mean,
                              float & var )
{
  vil_image_view<PixType> obj_img = vil_crop( *image_,
                                              obj->bbox_.min_x(), 
                                              obj->bbox_.width(), 
                                              obj->bbox_.min_y(), 
                                              obj->bbox_.height() );

  vil_image_view< vil_rgb<PixType> >  obj_img_rgb;
  vil_convert_cast( obj_img, obj_img_rgb );
  vil_image_view< PixType >  obj_img_grey;
  vil_convert_rgb_to_grey( obj_img_rgb, obj_img_grey );
  vil_math_mean_and_variance( mean, var, obj_img_grey, 0);
}

} // end namespace vidtk
