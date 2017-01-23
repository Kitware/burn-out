/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_READER_DEFAULT_H_
#define _IMAGE_OBJECT_READER_DEFAULT_H_

#include <tracking_data/io/image_object_reader_interface.h>
#include <boost/iostreams/filtering_stream.hpp>


namespace vidtk {
namespace ns_image_object_reader {


// ----------------------------------------------------------------
/** Default image object reader
 *
 * The first line is a header, and begins with "#".  Each subsequent
 * line has 12 columns containing the following data:
 *
 * \li Column 1: Frame number (-1 if not available)
 * \li Column 2: Time (-1 if not available)
 * \li Columns 3-5: World location (x,y,z)
 * \li Columns 6-7: Image location (i,j)
 * \li Column 8: Area
 * \li Columns 9-12: Bounding box in image coordinates (i0,j0,i1,j1)
 *
 * See image_object for more details about these values.
 */
class image_object_reader_default
  : public image_object_reader_interface
{
public:
  image_object_reader_default();
  virtual ~image_object_reader_default();

  virtual bool open( std::string const& filename);
  virtual bool read_next(ns_image_object_reader::datum_t& datum);

private:
  boost::iostreams::filtering_istream * in_stream_;

}; // end class ImageObjectReaderDefault

} // end namespace
} // end namespace


#endif /* _IMAGE_OBJECT_READER_DEFAULT_H_ */
