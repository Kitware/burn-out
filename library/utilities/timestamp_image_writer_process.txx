/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/timestamp_image_writer_process.h>
#include <utilities/timestamp_image.h>

namespace vidtk
{

timestamp_image_writer_process
::timestamp_image_writer_process( const vcl_string& name )
  : process( name )
{
  // what configurables do we want?
}

timestamp_image_writer_process
::~timestamp_image_writer_process()
{
}


config_block
timestamp_image_writer_process
::params() const
{
  return config_;
}


bool
timestamp_image_writer_process
::set_params( const config_block& blk )
{
  // ?
  return true;
}

bool
timestamp_image_writer_process
::initialize()
{
  // ?
  return true;
}


bool
timestamp_image_writer_process
::step()
{
  return add_timestamp( unstamped_image_, timestamped_image_, ts_ );
}


