/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_IMAGE_OBJECT_READER_INTERFACE_H_
#define _VIDTK_IMAGE_OBJECT_READER_INTERFACE_H_

#include <utilities/timestamp.h>
#include <tracking_data/image_object.h>
#include <string>
#include <vector>
#include <utility>


namespace vidtk {
namespace ns_image_object_reader {

typedef std::pair< vidtk::timestamp, vidtk::image_object_sptr > datum_t;
typedef std::pair< vidtk::timestamp, std::vector< vidtk::image_object_sptr > > ts_object_vector_t;


// ----------------------------------------------------------------
/** Abstract base class for all image object file formats.
 *
 *
 */
class image_object_reader_interface
{
public:
  image_object_reader_interface();
  virtual ~image_object_reader_interface();

  /**
   * @brief Try to open the file.
   *
   * True is returned if the file format is readable; false otherwise.
   * If the file format is recognisable, the file in a state ready to
   * read.
   *
   * @param filename Name of file to open
   */
  virtual bool open( std::string const& filename) = 0;

  /**
   * @brief Read next timestamp/image_object set.
   *
   * @param[out] datum Next timestamp/image_object from file.
   * @return \b true if data is returned. \b false if end of file.
   */
  virtual bool read_next(ns_image_object_reader::datum_t& datum) = 0;


protected:

  datum_t rnt_last_read_;

  std::string filename_;

}; // end class image_object_reader_interface

} // end namespace
} // end namespace

#endif /* _VIDTK_IMAGE_OBJECT_READER_INTERFACE_H_ */
