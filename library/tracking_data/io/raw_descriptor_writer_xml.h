/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_RAW_DESCRIPTOR_WRITER_XML_H_
#define _VIDTK_RAW_DESCRIPTOR_WRITER_XML_H_

#include <tracking_data/io/raw_descriptor_writer_interface.h>

namespace vidtk {
namespace ns_raw_descriptor_writer {

// ----------------------------------------------------------------
/** \brief Write descriptor in XML format.
 *
 * This class writes a raw descriptor to a stream in a XML format.
 */
class raw_descriptor_writer_xml
  : public raw_descriptor_writer_interface
{
public:
  raw_descriptor_writer_xml();
  virtual ~raw_descriptor_writer_xml();

private:
  virtual bool write( descriptor_sptr_t desc );
  virtual bool initialize();
  virtual void finalize();

}; // end class raw_descriptor_writer_xml

}
} //end namespace vidtk

#endif /* _VIDTK_RAW_DESCRIPTOR_WRITER_XML_H_ */
