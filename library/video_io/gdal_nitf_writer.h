/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gdal_nitf_writer_h_
#define vidtk_gdal_nitf_writer_h_

#include <utilities/video_metadata.h>
#include <utilities/timestamp.h>

#include <vil/vil_image_view.h>

#include <string>

namespace vidtk
{
struct gdal_nitf_writer_impl;

template< class PIX_TYPE >
class gdal_nitf_writer
{
public:
  gdal_nitf_writer();

  ~gdal_nitf_writer();

  bool open( std::string const& fname, unsigned int ni, unsigned int nj, unsigned int nplanes,
             timestamp const& ts, video_metadata const& vm, unsigned int block_x, unsigned int block_y);

  bool write_block(vil_image_view<PIX_TYPE> & img, unsigned int ux, unsigned int uy);

  void close();

  bool is_open() const;
private:
  gdal_nitf_writer_impl * impl_;
};

}//namespace vidtk

#endif //#ifndef vidtk_gdal_nitf_writer_h_
