/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_IMAGE_OBJECT_WRITER_PROTOBUF_H_
#define _VIDTK_IMAGE_OBJECT_WRITER_PROTOBUF_H_

#include <tracking_data/io/image_object_writer.h>
#include <fstream>

namespace vidtk {

// ----------------------------------------------------------------
/**
 * @brief
 *
 */
class image_object_writer_protobuf
  : public image_object_writer
{
public:
  image_object_writer_protobuf();
  virtual ~image_object_writer_protobuf();

  virtual bool initialize(config_block const& blk);
  virtual void write( timestamp const& ts, std::vector< image_object_sptr > const& objs );

private:
  std::ofstream * fstr_; // output file stream
}; // end class image_object_writer_protobuf

} // end namespace

#endif /* _VIDTK_IMAGE_OBJECT_WRITER_PROTOBUF_H_ */
