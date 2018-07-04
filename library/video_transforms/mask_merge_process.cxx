/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_merge_process.h"

#include <vil/vil_transform.h>
#include <vil/vil_fill.h>

#include <logger/logger.h>

VIDTK_LOGGER( "mask_merge_process" );


namespace vidtk
{
mask_merge_process
::mask_merge_process( std::string const& _name )
  : process( _name, "mask_merge_process" )
{
}


bool
mask_merge_process
::set_params( config_block const& /*blk*/ )
{
  return true;
}


mask_merge_process
::~mask_merge_process()
{
}


config_block
mask_merge_process
::params() const
{
  return config_;
}


bool
mask_merge_process
::initialize()
{
  a_img_ = vil_image_view<bool>(NULL);
  b_img_ = vil_image_view<bool>(NULL);
  c_img_ = vil_image_view<bool>(NULL);

  border_ = image_border();

  return true;
}


static bool bin_log_or(bool a, bool b)
{
  return a || b;
}


bool
merge_masks( const vil_image_view< bool >& mask1,
             const vil_image_view< bool >& mask2,
             vil_image_view< bool >& output )
{
  if( mask1 && mask2 )
  {
    if( mask1.ni() != mask2.ni() || mask1.nj() != mask2.nj() )
    {
      LOG_ERROR( "Mask 1 (" << mask1.ni() << "x" << mask1.nj()
                 << ") and mask 2 (" << mask2.ni() << "x" << mask2.nj()
                 << ") have unequal size" );
      return false;
    }
    if( mask1.nplanes() != 1 || mask2.nplanes() != 1 )
    {
      LOG_ERROR( "Mask 1 (" << mask1.nplanes()
                 << ") or mask 2 (" << mask2.nplanes()
                 << ") have more than one plane" );
      return false;
    }

    vil_transform( mask1, mask2, output, bin_log_or );
  }
  else
  {
    if( mask1 )
    {
      output = mask1;
    }
    else if( mask2 )
    {
      output = mask2;
    }
    else
    {
      output = vil_image_view<bool>(NULL);
    }
  }

  return true;
}


bool
mask_merge_process
::step()
{
  // force allocation of new memory for output
  out_img_ = vil_image_view<bool>(NULL);

  // merge all possible input masks
  if( !merge_masks( a_img_, b_img_, out_img_ ) ||
      !merge_masks( out_img_, c_img_, out_img_ ) )
  {
    return false;
  }

  // fill in region around optional border
  if( border_.volume() > 0 )
  {
    borderless_out_img_.deep_copy( out_img_ );

    for( int i = 0; i < border_.min_x(); ++i )
    {
      vil_fill_col( out_img_, i, true );
    }
    for( int i = border_.max_x(); i < static_cast<int>( out_img_.ni() ); ++i )
    {
      vil_fill_col( out_img_, i, true );
    }
    for( int j = 0; j < border_.min_y(); ++j )
    {
      vil_fill_row( out_img_, j, true );
    }
    for( int j = border_.max_y(); j < static_cast<int>( out_img_.nj() ); ++j )
    {
      vil_fill_row( out_img_, j, true );
    }
  }
  else
  {
    borderless_out_img_ = out_img_;
  }

  // record that we've processed these input images
  a_img_ = vil_image_view<bool>(NULL);
  b_img_ = vil_image_view<bool>(NULL);
  c_img_ = vil_image_view<bool>(NULL);

  border_ = image_border();

  return true;
}


void
mask_merge_process
::set_mask_a( vil_image_view<bool> const& img )
{
  a_img_ = img;
}


void
mask_merge_process
::set_mask_b( vil_image_view<bool> const& img )
{
  b_img_ = img;
}


void
mask_merge_process
::set_mask_c( vil_image_view<bool> const& img )
{
  c_img_ = img;
}


void
mask_merge_process
::set_border( image_border const& border )
{
  border_ = border;
}


vil_image_view<bool>
mask_merge_process
::mask() const
{
  return out_img_;
}


vil_image_view<bool>
mask_merge_process
::borderless_mask() const
{
  return borderless_out_img_;
}


} // end namespace vidtk
