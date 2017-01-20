/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_WRITER_KW18_H_
#define _IMAGE_OBJECT_WRITER_KW18_H_

#include <tracking_data/io/image_object_writer.h>
#include <fstream>


namespace vidtk {

// ----------------------------------------------------------------
/** Write image objects in kw18 format.
 *
 * This class writes the omage object in a KW18 format suitable for
 * working with score tracks.
 */
class image_object_writer_kw18
  : public image_object_writer
{
public:
  image_object_writer_kw18();
  virtual ~image_object_writer_kw18();

  virtual bool initialize(config_block const& blk);
  virtual void write( timestamp const& ts, std::vector< image_object_sptr > const& objs );


private:
  int track_id_;
  std::ofstream * fstr_; // output file stream

}; // end class ImageObjectWriterKW18

} // end namespace

#endif /* _IMAGE_OBJECT_WRITER_KW18_H_ */
