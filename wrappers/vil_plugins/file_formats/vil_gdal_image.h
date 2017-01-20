/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIL_FILE_FORMATS_GDAL_H_
#define VIL_FILE_FORMATS_GDAL_H_

#include <vil/vil_file_format.h>
#include <vil/vil_stream.h>
#include <vil/vil_image_view.h>
#include <vil/vil_image_resource.h>
#include <map>

class GDALDataset;

//: TODO
class vil_gdal_file_format : public vil_file_format
{
 public:
  virtual char const *tag() const;
  virtual vil_image_resource_sptr make_input_image(vil_stream *vs);
  virtual vil_image_resource_sptr make_output_image(vil_stream* vs,
    unsigned nx, unsigned ny, unsigned nplanes, enum vil_pixel_format);
};

class vil_gdal_image_impl;

//: TODO
class vil_gdal_image : public vil_image_resource
{
  friend class vil_gdal_file_format;
  friend class vil_gdal_image_impl;
public:

  // Construct a new NITF decoder
  vil_gdal_image(vil_stream *s);

  // Desctructor
  virtual ~vil_gdal_image(void);

  //: Dimensions:  Planes x ni x nj.
  // This concept is treated as a synonym to components.
  virtual unsigned nplanes() const;

  //: Dimensions:  Planes x ni x nj.
  // The number of pixels in each row.
  virtual unsigned ni() const;

  //: Dimensions:  Planes x ni x nj.
  // The number of pixels in each column.
  virtual unsigned nj() const;

  //: Pixel Format.
  //  A standard RGB RGB RGB of chars image has
  // pixel_format() == VIL_PIXEL_FORMAT_BYTE
  virtual enum vil_pixel_format pixel_format() const;

  //: Create a read/write view of a copy of this data.
  // This function will always return a
  // multi-plane scalar-pixel view of the data.
  // \return 0 if unable to get view of correct size, or if resource is write-only.
  virtual vil_image_view_base_sptr get_copy_view(unsigned i0, unsigned ni,
                                                 unsigned j0, unsigned nj) const;

  // Decode SDK only suports reading, not writing
  virtual bool put_view(const vil_image_view_base& im, unsigned i0, unsigned j0)
  {  return false;  }

  //: Return a string describing the file format.
  // Only file images have a format, others return 0
  virtual char const* file_format() const;

  //: Extra property information
  virtual bool get_property(char const* tag, void* property_value = 0) const;

  virtual bool is_good(void) const;

  void print_metadata(void) const;

  //return the data based on metadata tag
  bool get_data(char const* tag, std::string &metadata) const;

private:
  //: Convert short form property names into long form names
  // Example, "FRLC_LOC" converts to "NITF::IM001::TRE_BLOCKA_FRLC_LOC"
  static std::string map_property_name(const std::string& in);

  std::map<std::string, std::string> parsed_data_;
  GDALDataset *gdal_data_;
  vil_gdal_image_impl *impl_;
  std::string gdal_filename_;
};

#endif
