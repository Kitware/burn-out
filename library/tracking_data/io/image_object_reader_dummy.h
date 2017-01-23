/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_READER_DUMMY_H_
#define _IMAGE_OBJECT_READER_DUMMY_H_

#include <tracking_data/io/image_object_reader_interface.h>

namespace vidtk {
namespace ns_image_object_reader {


// ----------------------------------------------------------------
/** Dummy image object reader
 *
 * This object is used as the reader when the correct reader for the
 * file format could not be found. In some test cases, the open will
 * fail and the test will still try to read. Adding a dummg reader is
 * easier and cleaner than adding multiple checks in the reader class.
 *
 * One mighe consider adding log messages to the methods, but the open
 * already failed and logged an error.
 */
class image_object_reader_dummy
  : public image_object_reader_interface
{
public:
  image_object_reader_dummy() { }
  virtual ~image_object_reader_dummy() { }

  virtual bool open( std::string const& ) { return false; }
  virtual bool read_next(ns_image_object_reader::datum_t& ) { return false; }

}; // end class ImageObjectReaderDummy

} // end namespace
} // end namespace


#endif /* _IMAGE_OBJECT_READER_DUMMY_H_ */
