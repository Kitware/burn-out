/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_RAW_DESCRIPTOR_WRITER_JSON_H_
#define _VIDTK_RAW_DESCRIPTOR_WRITER_JSON_H_

#include <tracking_data/io/raw_descriptor_writer_interface.h>

namespace vidtk {
namespace ns_raw_descriptor_writer {

// ----------------------------------------------------------------
/** \brief Write descriptor in JSON format.
 *
 * This class writes a raw descriptor to a stream in a JSON format.
 *
 */
class raw_descriptor_writer_json
  : public raw_descriptor_writer_interface
{
public:
  raw_descriptor_writer_json();
  virtual ~raw_descriptor_writer_json();

private:
  virtual bool write( descriptor_sptr_t desc );
  virtual bool initialize();
  virtual void finalize();

  unsigned descriptor_counter;

}; // end class raw_descriptor_writer_json

}
} //end namespace vidtk

#endif /* _VIDTK_RAW_DESCRIPTOR_WRITER_JSON_H_ */
