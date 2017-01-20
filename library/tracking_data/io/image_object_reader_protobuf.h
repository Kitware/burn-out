/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VISTK_IMAGE_OBJECT_READER_PROTOBUF_H_
#define _VISTK_IMAGE_OBJECT_READER_PROTOBUF_H_

#include <tracking_data/io/image_object_reader_interface.h>

#include <string>
#include <fstream>


namespace vidtk {
namespace ns_image_object_reader {

// ----------------------------------------------------------------
/**
 * @brief
 *
 */
class image_object_reader_protobuf
  : public image_object_reader_interface
{
public:
  image_object_reader_protobuf();
  virtual ~image_object_reader_protobuf();

  /**
   * @brief Open file for reading
   *
   */
  virtual bool open( std::string const& filename);

 /**
  * @brief Read next image object
  *
  * The outupt is returned through the parameter, which is a
  * timestamp/image_object pair.
  *
  * The semantics of this call requires us to always return some image
  * objects, for an empty vector indicates end of file. Our approach is
  * to skip containers that do not have any image objects.
  *
  * @param[out] datum Timestamp and image object pair returned here.
  *
  * @return \b true if returning a value. \b false if end of file.
  */
  virtual bool read_next(ns_image_object_reader::datum_t& datum);


private:
  std::ifstream m_stream;

  // Current batch of data we are working on
  timestamp m_timestamp;
  std::vector< image_object_sptr > m_obj_list;
  std::vector< image_object_sptr >::const_iterator m_next_obj;


}; // end class image_object_reader_protobuf


} // end namespace
} // end namespace

#endif /* _VISTK_IMAGE_OBJECT_READER_PROTOBUF_H_ */
