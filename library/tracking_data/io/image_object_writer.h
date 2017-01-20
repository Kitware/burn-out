/*keg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_IMAGE_OBJECT_WRITER_H_
#define _VIDTK_IMAGE_OBJECT_WRITER_H_


#include <string>
#include <vector>
#include <tracking_data/image_object.h>
#include <utilities/timestamp.h>
#include <utilities/config_block.h>

namespace vidtk
{

// ----------------------------------------------------------------
/** Abstract base class for all image object writers.
 *
 * This class defines the interface for all image object writers. This
 * class is subclassed for the different file formats.
 *
 * Refer to image_object_writer_process for usage of subclasses.
 *
 * \sa image_object_writer_process
 */
class image_object_writer
{
public:
  image_object_writer();
  virtual ~image_object_writer();

  /**
   * @brief Write a timestamp and set of image objects.
   *
   * @param[in] ts - pointer to timestamp to write
   * @param[in] objs - refers to a list of image objects to write
   */
  virtual void write( timestamp const& ts, std::vector< image_object_sptr > const& objs ) = 0;
  virtual bool initialize( config_block const& blk );


protected:
  std::string filename_;
  bool allow_overwrite_;
}; // end class image_object_writer


} // end namespace

#endif /* _VIDTK_IMAGE_OBJECT_WRITER_H_ */
