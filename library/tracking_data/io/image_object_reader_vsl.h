/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_READER_VSL_H_
#define _IMAGE_OBJECT_READER_VSL_H_

#include <tracking_data/io/image_object_reader_interface.h>

#include <vsl/vsl_binary_io.h>

namespace vidtk {
namespace ns_image_object_reader {


// ----------------------------------------------------------------
/** Read image objects in VSL format.
 *
 *
 */
class image_object_reader_vsl
  : public image_object_reader_interface
{
public:
  image_object_reader_vsl();
  virtual ~image_object_reader_vsl();

  virtual bool open( std::string const& filename);
  virtual bool read_next(ns_image_object_reader::datum_t& datum);

private:
  vsl_b_ifstream* bfstr_;

  // data for the current location in file.
  std::vector< image_object_sptr > current_obj_list_;
  vidtk::timestamp current_ts_;
  std::vector< image_object_sptr >::const_iterator current_obj_it_;

}; // end class

} // end namespace
} // end namespace

#endif /* _IMAGE_OBJECT_READER_VSL_H_ */
