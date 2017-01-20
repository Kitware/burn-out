/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "raw_descriptor_writer_xml.h"

#include <vul/vul_sprintf.h>

#include <logger/logger.h>

namespace vidtk {
namespace ns_raw_descriptor_writer {

VIDTK_LOGGER("raw_descriptor_writer_xml");


// ----------------------------------------------------------------------------
raw_descriptor_writer_xml::
raw_descriptor_writer_xml()
{
}

raw_descriptor_writer_xml::
~raw_descriptor_writer_xml()
{
}


// ----------------------------------------------------------------------------
bool
raw_descriptor_writer_xml::
write( descriptor_sptr_t /*desc*/ )
{
  if ( ! is_good() )
  {
    LOG_ERROR( "Output stream not open or in error state" );
    return false;
  }

  // When the feature is finally implemented, we can return true instead;
  LOG_ERROR( "XML formatted descriptors not yet implemented" );
  return false;
} // raw_descriptor_writer_xml::write


// ----------------------------------------------------------------------------
bool
raw_descriptor_writer_xml::
initialize()
{
  // Write out header
  output_stream() << "# 1:descriptor_id, 2:start_frame, 3:end_frame, "
                  << "4:start_timestamp, 5:end_timestamp, "
                  << "6:track_references, 7:descriptor_data_vector"
                  << std::endl;
  return true;
}


void
raw_descriptor_writer_xml::
finalize()
{
}


} // end namespace
} // end namespace vidtk
