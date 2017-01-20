/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "external_settings.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "external_settings" );

bool
external_settings
::load_config_from_file( const std::string& filename )
{
  bool success = true;

  try
  {
    config_block blk = this->config();
    blk.parse( filename.c_str() );
    this->read_config( blk );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( "Couldn't parse " << filename << " due to: " << e.what() );
    success = false;
  }

  return success;
}

} // end namespace vidtk
