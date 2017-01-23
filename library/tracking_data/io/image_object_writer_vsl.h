/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_WRITER_VSL_H_
#define _IMAGE_OBJECT_WRITER_VSL_H_

#include <tracking_data/io/image_object_writer.h>

#include <vsl/vsl_binary_io.h>


namespace vidtk {

// ----------------------------------------------------------------
/** Write image objects in VSL format.
 *
 *
 */
class image_object_writer_vsl
  : public image_object_writer
{
public:
  image_object_writer_vsl();
  virtual ~image_object_writer_vsl();

  virtual bool initialize(config_block const& blk);
  virtual void write( timestamp const& ts, std::vector< image_object_sptr > const& objs );

private:
  vsl_b_ofstream* bfstr_;

}; // end class ImageObjectWritervsl

} // end namespace

#endif /* _IMAGE_OBJECT_WRITER_VSL_H_ */
