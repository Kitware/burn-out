/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_select_process.h"

#include <vxl_config.h>

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>


namespace vidtk
{


/** Constructor.
 *
 *
 */
mask_select_process
::mask_select_process( vcl_string const& name )
  : process( name, "mask_select_process" )
{
  config_.add_parameter(
    "threshold",
    "0.03",
    "Percentage of data required to trigger mask." );
}


mask_select_process
::~mask_select_process()
{
}


config_block
mask_select_process
::params() const
{
  return config_;
}


bool
mask_select_process
::set_params( config_block const& blk )
{

  try
  {
    blk.get( "threshold", threshold_ );
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


bool
mask_select_process
::initialize()
{
  return true;
}



bool
mask_select_process
::step()
{
  vil_image_view<vxl_byte> grey_img, box_vert_smooth;

  unsigned ni = in_mask_.ni(), nj = in_mask_.nj();
  assert(data_image_.ni() == ni);
  assert(data_image_.nj() == nj);
  unsigned mask_count = 0;
  unsigned data_count = 0;
  for (unsigned i=0; i<ni; ++i)
  {
    for (unsigned j=0; j<nj; ++j)
    {
      if (in_mask_(i,j))
      {
        ++mask_count;
        if (data_image_(i,j,2) == 255)
        {
          ++data_count;
        }
      }
    }
  }
  double ratio = double(data_count) / mask_count;

  if (ratio > threshold_)
  {
    out_mask_ = in_mask_;
  }
  else
  {
    out_mask_.clear();
    out_mask_.set_size( ni, nj );
    out_mask_.fill( 0 );
  }

  return true;
}


void
mask_select_process
::set_data_image( vil_image_view<vxl_byte> const& img )
{
  data_image_ = img;
}

void
mask_select_process
::set_mask( vil_image_view<bool> const& mask )
{
  in_mask_ = mask;
}


vil_image_view<bool> const&
mask_select_process
::mask() const
{
  return out_mask_;
}


} // end namespace vidtk
