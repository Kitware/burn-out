/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_image_process.h"

#include <logger/logger.h>
VIDTK_LOGGER("mask_image_process_cxx");

namespace vidtk
{
mask_image_process
::mask_image_process( std::string const& _name )
  : process( _name, "mask_image_process" ),
    masked_value_( false )
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
    masked_value_ = blk.get<bool>( "masked_value" );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
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
    LOG_ERROR( this->name() << ": input image not set" );
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
    LOG_ERROR( "Input image (" << in_img_->ni() << "x" << in_img_->nj()
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


vil_image_view<bool>
mask_image_process
::image() const
{
  return out_img_;
}

vil_image_view<bool>
mask_image_process
::copied_image() const
{
  copied_out_img_ = vil_image_view< bool >();
  copied_out_img_.deep_copy( out_img_ );
  return copied_out_img_;
}


} // end namespace vidtk
