/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_WRITER_DEFAULT_H_
#define _IMAGE_OBJECT_WRITER_DEFAULT_H_

#include <tracking_data/io/image_object_writer.h>
#include <fstream>


namespace vidtk {

// ----------------------------------------------------------------
/** Default image object writer
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
class image_object_writer_default
  : public image_object_writer
{
public:
  image_object_writer_default();
  virtual ~image_object_writer_default();

  virtual bool initialize(config_block const& blk);
  virtual void write(timestamp const& ts, std::vector< image_object_sptr > const& objs);

private:
  std::ofstream * fstr_; // output file stream

}; // end class ImageObjectWriterDefault

} // end namespace

#endif /* _IMAGE_OBJECT_WRITER_DEFAULT_H_ */
