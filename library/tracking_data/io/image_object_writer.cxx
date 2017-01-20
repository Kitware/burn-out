/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "image_object_writer.h"

#include <logger/logger.h>
#include <vul/vul_file.h>


namespace vidtk {

VIDTK_LOGGER ("image_object_writer");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
image_object_writer
::image_object_writer()
  : allow_overwrite_ (false)
{ }

image_object_writer
::~image_object_writer()
 { }


bool
image_object_writer
::initialize(config_block const& blk)
{
  try
  {
    filename_ = blk.get < std::string >( "filename" );
    allow_overwrite_ = blk.get < bool >( "overwrite_existing" );
  }
  catch( config_block_parse_error& e)
  {
    LOG_ERROR ("image_object_writer: " << e.what() );
    return false;
  }

  if ( ( ! allow_overwrite_ )
       && vul_file::exists( filename_ ))
  {
    return false;
  }

  return true;
}


} // end namespace
