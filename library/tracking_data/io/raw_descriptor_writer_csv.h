/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_RAW_DESCRIPTOR_WRITER_CSV_H_
#define _VIDTK_RAW_DESCRIPTOR_WRITER_CSV_H_

#include <tracking_data/io/raw_descriptor_writer_interface.h>

namespace vidtk {
namespace ns_raw_descriptor_writer {

// ----------------------------------------------------------------
/** \brief Write descriptor in CSV format.
 *
 * This class writes a raw descriptor to a stream in a CVS format.
 * The columns are in the following order:
 *
 *  1:descriptor_id
 *  2:start_frame
 *  3:end_frame
 *  4:start_timestamp
 *  5:end_timestamp
 *  6:track_references (may be a group of values)
 *  7:descriptor_data_vector
 *
 */
class raw_descriptor_writer_csv
  : public raw_descriptor_writer_interface
{
public:
  raw_descriptor_writer_csv();
  virtual ~raw_descriptor_writer_csv();

private:
  virtual bool write( descriptor_sptr_t desc );
  virtual bool initialize();
  virtual void finalize();

}; // end class raw_descriptor_writer_csv

}
} //end namespace vidtk

#endif /* _VIDTK_RAW_DESCRIPTOR_WRITER_CSV_H_ */
