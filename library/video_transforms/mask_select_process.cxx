/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_select_process.h"

#include <vxl_config.h>

#include <logger/logger.h>

VIDTK_LOGGER("mask_select_process");


namespace vidtk
{


/** Constructor.
 *
 *
 */
mask_select_process
::mask_select_process( std::string const& _name )
  : process( _name, "mask_select_process" ),
    threshold_( 0.0 )
{
  config_.add_parameter(
    "threshold",
    "0.0",
    "Minimum fraction ([0.0, 1.0] range) of burn-in pixels in the frame "
    "required to trigger the mask. Default: 0.0 implies the mask is always ON." );
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
    threshold_ = blk.get<double>( "threshold" );
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

  if (ratio >= threshold_)
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
::set_mask( vil_image_view<bool> const& _mask )
{
  in_mask_ = _mask;
}


vil_image_view<bool>
mask_select_process
::mask() const
{
  return out_mask_;
}


} // end namespace vidtk
