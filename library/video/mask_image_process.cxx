/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_image_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vil/vil_convert.h>
#include <vil/vil_new.h>
#include <vil/vil_load.h>


namespace vidtk
{
mask_image_process
::mask_image_process( vcl_string const& name )
  : process( name, "mask_image_process" )
{
  config_.add_parameter(
    "masked_value",
    "false",
    "The value to set for the pixels that are masked out.");
}


bool
mask_image_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "masked_value", masked_value_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  config_.update( blk );
  return true;
}



mask_image_process
::~mask_image_process()
{
}



config_block
mask_image_process
::params() const
{
  return config_;
}



bool
mask_image_process
::initialize()
{
  in_img_ = NULL;
  mask_img_ = NULL;

  return true;
}



bool
mask_image_process
::step()
{
  if( in_img_ == NULL )
  {
    log_error( this->name() << ": input image not set" );
    return false;
  }

  if( mask_img_ == NULL )
  {
    out_img_ = *in_img_;
    in_img_ = NULL;
    return true;
  }

  if( in_img_->ni() != mask_img_->ni()
   || in_img_->nj() != mask_img_->nj()
   || in_img_->nplanes() !=  1
   || mask_img_->nplanes() != 1)
  {
    log_error( "Input image (" << in_img_->ni() << "x" << in_img_->nj()
	       << ") and mask image (" << mask_img_->ni() << "x" << mask_img_->nj()
	       << ") have unequal size" );
    return false;
  }

  unsigned ni = in_img_->ni();
  unsigned nj = in_img_->nj();

  out_img_.set_size(ni, nj);

  for(unsigned j=0;j<nj;++j)
  {
    for(unsigned i=0;i<ni;++i)
    {
      out_img_(i,j) = (*mask_img_)(i,j) ? masked_value_ : (*in_img_)(i,j);
    }
  }

  // record that we've processed these input images
  in_img_ = NULL;
  mask_img_ = NULL;

  return true;
}



void
mask_image_process
::set_source_image( vil_image_view<bool> const& img )
{
  in_img_ = &img;
}


void
mask_image_process
::set_mask_image( vil_image_view<bool> const& img )
{
  mask_img_ = &img;
}


vil_image_view<bool> const&
mask_image_process
::image() const
{
  return out_img_;
}

vil_image_view<bool> const&
mask_image_process
::copied_image() const
{
  copied_out_img_ = vil_image_view< bool >();
  copied_out_img_.deep_copy( out_img_ );
  return copied_out_img_;
}


} // end namespace vidtk
