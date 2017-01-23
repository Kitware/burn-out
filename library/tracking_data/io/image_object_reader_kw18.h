/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_READER_KW18_H_
#define _IMAGE_OBJECT_READER_KW18_H_

#include <tracking_data/io/image_object_reader_interface.h>
#include <boost/iostreams/filtering_stream.hpp>


namespace vidtk {
namespace ns_image_object_reader {


// ----------------------------------------------------------------
/** kw18 image object reader
 */
class image_object_reader_kw18
  : public image_object_reader_interface
{
public:
  image_object_reader_kw18();
  virtual ~image_object_reader_kw18();

  virtual bool open( std::string const& filename );
  virtual bool read_next( ns_image_object_reader::datum_t& datum );

private:
  boost::iostreams::filtering_istream * in_stream_;

}; // end class image_object_reader_kw18

} // end namespace
} // end namespace


#endif /* _IMAGE_OBJECT_READER_KW18_H_ */
