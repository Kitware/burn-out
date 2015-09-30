/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_merge_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vil/vil_transform.h>
#include <vil/vil_new.h>
#include <vil/vil_load.h>


namespace vidtk
{
mask_merge_process
::mask_merge_process( vcl_string const& name )
  : process( name, "mask_merge_process" )
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

  return true;
}


static bool bin_log_or(bool a, bool b)
{
  return a || b;
}


bool
mask_merge_process
::step()
{
  if( a_img_ && b_img_ )
  {
    if( a_img_.ni() != b_img_.ni()
     || a_img_.nj() != b_img_.nj())
    {
      log_error( "Mask a (" << a_img_.ni() << "x" << a_img_.nj()
                 << ") and mask b (" << b_img_.ni() << "x" << b_img_.nj()
                 << ") have unequal size" );
      return false;
    }
    if( a_img_.nplanes() != 1
     || b_img_.nplanes() != 1)
    {
      log_error( "Mask a (" << a_img_.nplanes()
                 << ") or mask b (" << b_img_.nplanes()
                 << ") have more than one plane" );
      return false;
    }

    // force allocation of new memory for output
    out_img_ = vil_image_view<bool>(NULL);
    vil_transform( a_img_, b_img_, out_img_, bin_log_or );
  }
  else
  {
    if ( a_img_ )
    {
      out_img_ = a_img_;
    }
    else if ( b_img_ )
    {
      out_img_ = b_img_;
    }
    else
    {
      out_img_ = vil_image_view<bool>(NULL);
    }
  }

  // record that we've processed these input images
  a_img_ = vil_image_view<bool>(NULL);
  b_img_ = vil_image_view<bool>(NULL);

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


vil_image_view<bool> const&
mask_merge_process
::mask() const
{
  return out_img_;
}


} // end namespace vidtk
