/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "kw_archive_index_writer.h"

#include <cstdlib>

#include <vul/vul_file.h>

#include <vil/file_formats/vil_jpeg.h>
#include <vil/vil_pixel_format.h>
#include <vil/vil_stream_core.h>

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vnl/io/vnl_io_matrix_fixed.h>
#include <vnl/io/vnl_io_vector_fixed.h>
#include <vil/io/vil_io_image_view.h>
#include <vil/vil_convert.h>
#include <vnl/vnl_double_2.h>

#include <logger/logger.h>

// Create static default logger for log macros
VIDTK_LOGGER("kw_archive_index_writer");

namespace vidtk
{


// NOTE: The constructor defines the default values of the parameters,
// so make sure that the construction defaults stay consistent with
// the constructor documentation. We intentionally do not document the
// setter functions to minimize the chance of the documentation not
// matching the code.  Also, for clarity, explicitly construct all the
// parameter objects, even if the explicit constructor would do the
// same as the default constructor. E.g., keep base_filename("") even
// though the default constructor for a string would do the same,
// because having the line clarifies that the value has been explicitly
// decided as the default value.

/// \brief The parameters controlling how the archive is written.
///
///
template< class PixType >
kw_archive_index_writer < PixType >::open_parameters
::open_parameters()
  : base_filename_(""),
    separate_meta_(false),
    overwrite_(false),
    mission_id_(""),
    stream_id_(""),
    compress_image_( false )
{
}

// This macro implements the set_X methods
#define DO_PARAMETER(NAME, TYPE)                                        \
  template< class PixType >                                              \
  typename kw_archive_index_writer < PixType >::open_parameters&        \
  kw_archive_index_writer < PixType >::open_parameters                  \
  ::set_ ## NAME(TYPE const& value)                                     \
  {                                                                     \
    NAME ## _ = value;                                                  \
    return *this;                                                       \
  }

DO_PARAMETER(base_filename, std::string)
DO_PARAMETER(separate_meta, bool)
DO_PARAMETER(overwrite, bool)
DO_PARAMETER(mission_id, std::string)
DO_PARAMETER(stream_id, std::string)
DO_PARAMETER(compress_image, bool)

#undef DO_PARAMETER


template< class PixType >
kw_archive_index_writer< PixType >::
kw_archive_index_writer()
: index_stream_(NULL),
  meta_stream_(NULL),
  meta_bstream_(NULL),
  data_stream_(NULL),
  data_bstream_(NULL),
  data_version_( 0 ),
  compress_image_( 0 )
{
}


template< class PixType >
kw_archive_index_writer< PixType >::
~kw_archive_index_writer()
{
  this->close();
}


template< class PixType >
bool
kw_archive_index_writer< PixType >
::open(open_parameters const& param)
{
  this->compress_image_ = param.compress_image_;

  std::string index_filename = param.base_filename_ + ".index";
  std::string meta_filename  = param.base_filename_ + ".meta";
  std::string data_filename  = param.base_filename_ + ".data";

  if (param.overwrite_==false)
  {
    if (vul_file::exists(index_filename))
    {
      LOG_DEBUG(index_filename << " already exists");
      return false;
    }
    if (vul_file::exists(data_filename))
    {
      LOG_DEBUG(data_filename << " already exists");
      return false;
    }
  }

  // We check for the existence of a .meta file if we aren't going to
  // overwrite it: i.e., overwrite is false, or we aren't writing one
  // at all (separate_meta==false) because we don't want to deal with
  // the confusion of writing a new .index file and a new .data file
  // and have an old .meta file laying around.
  if ((param.overwrite_==false || param.separate_meta_==false) &&
      vul_file::exists(meta_filename))
  {
    LOG_DEBUG(meta_filename << " already exists");
    return false;
  }

  this->index_stream_ = new std::ofstream(index_filename.c_str(),
                                         std::ios::out|std::ios::trunc);
  if (!*this->index_stream_)
  {
    LOG_DEBUG("Failed to open " << index_filename << " for writing\n");
    return false;
  }

  if (param.separate_meta_)
  {
    this->meta_stream_ = new vidtk::large_file_ofstream(meta_filename.c_str(),
                                                        std::ios::out|std::ios::trunc|std::ios::binary);
    this->meta_bstream_ = new vsl_b_ostream(this->meta_stream_);
    if (!*this->meta_stream_)
    {
      LOG_DEBUG("Failed to open " << meta_filename << " for writing\n");
      return false;
    }
  }

  this->data_stream_ = new vidtk::large_file_ofstream(data_filename.c_str(),
                                                      std::ios::out|std::ios::trunc|std::ios::binary);
  this->data_bstream_ = new vsl_b_ostream(this->data_stream_);
  if (!*this->data_stream_)
  {
    LOG_DEBUG("Failed to open " << data_filename << " for writing\n");
    return false;
  }

  // Write the headers
  *this->index_stream_
    << "4\n" // Version number
    << vul_file::basename(data_filename) << "\n";
  if (this->data_bstream_!=NULL)
  {
    *this->index_stream_ << vul_file::basename(meta_filename) << "\n";
  }
  else
  {
    *this->index_stream_ << "\n";
  }
  *this->index_stream_
    << param.mission_id_ << "\n"
    << param.stream_id_ << "\n";


  if (this->compress_image_)
  {
    this->data_version_ = 3;
  }
  else
  {
    this->data_version_ = 2;
  }

  vsl_b_write(*this->data_bstream_, this->data_version_); // version number

  if (this->meta_bstream_)
  {
    vsl_b_write(*this->meta_bstream_, static_cast<int>(2)); // version number
  }

  if (!*this->index_stream_ || !*this->data_stream_ ||
      (param.separate_meta_ && !*this->meta_stream_))
  {
    LOG_DEBUG("Failed while writing headers");
    return false;
  }
  else
  {
    return true;
  }
}


template< class PixType >
void
kw_archive_index_writer< PixType >
::close()
{
  // Must set pointers to zero to prevent multiple calls to close from
  // doing bad things.
  delete this->index_stream_;
  this->index_stream_ = 0;

  delete this->meta_bstream_;
  this->meta_bstream_ = 0;

  delete this->meta_stream_;
  this->meta_stream_ = 0;

  delete this->data_bstream_;
  this->data_bstream_ = 0;

  delete this->data_stream_;
  this->data_stream_ = 0;
}

template< class PixType >
void
kw_archive_index_writer< PixType >
::convert_byte_view(vil_image_view<vxl_byte> const& image,
               vil_image_view<vxl_byte> & byte_view)
{
  byte_view = image; // copying produces identical results for 8 bit images
}

template< class PixType >
void
kw_archive_index_writer< PixType >
::convert_byte_view(vil_image_view<vxl_uint_16> const& image,
               vil_image_view<vxl_byte> & byte_view)
{
  vil_convert_stretch_range(image, byte_view); // stretching tends to produce better rounding for 16 bit images
}

template< class PixType >
bool
kw_archive_index_writer< PixType >
::write_frame(vidtk::timestamp const& ts,
              vidtk::video_metadata const& corner_points,
              vil_image_view<PixType> const& image,
              vidtk::image_to_image_homography const& frame_to_ref,
              double gsd)
{
  bool status = true;

  *this->index_stream_
    << static_cast<vxl_int_64>(ts.time()) << " "
    << static_cast<vxl_int_64>(this->data_stream_->tellp())
    << "\n";

  LOG_DEBUG("Pixel is: " << int(image(10,10)));

  this->write_frame_data(*this->data_bstream_,
                         /*write image=*/ true,
                         ts, corner_points, image, frame_to_ref, gsd);
  if (!this->data_stream_)
  {
    LOG_DEBUG("Failed while writing to .data stream");
    status=false;
  }

  if (this->meta_bstream_)
  {
    this->write_frame_data(*this->meta_bstream_,
                           /*write image=*/ false,
                           ts, corner_points, image, frame_to_ref, gsd);
    if (!this->meta_stream_)
    {
      LOG_DEBUG("Failed while writing to .meta stream");
      status=false;
    }
  }

  return status;
}


// This function does the actual writing, so that it can be called
// both for writing the .meta and for writing the .data
template< class PixType >
void
kw_archive_index_writer< PixType >
::write_frame_data(vsl_b_ostream& stream,
                   bool write_image,
                   vidtk::timestamp const& ts,
                   vidtk::video_metadata const& corner_points,
                   vil_image_view<PixType> const& image,
                   vidtk::image_to_image_homography const& frame_to_ref,
                   double gsd)
{
  vxl_int_64 u_seconds = static_cast<vxl_int_64>(ts.time());
  vxl_int_64 frame_num = static_cast<vxl_int_64>(ts.frame_number());
  vxl_int_64 ref_frame_num = static_cast<vxl_int_64>(frame_to_ref.get_dest_reference().frame_number());
  vnl_matrix_fixed< double, 3, 3 > const& homog = frame_to_ref.get_transform().get_matrix();
  vil_image_view <vxl_byte> byte_view;
  convert_byte_view(image, byte_view);
  std::vector< vnl_vector_fixed< double, 2 > > corners;

  corners.push_back(vnl_double_2(corner_points.corner_ul().get_latitude(),
                                 corner_points.corner_ul().get_longitude()));
  corners.push_back(vnl_double_2(corner_points.corner_ur().get_latitude(),
                                 corner_points.corner_ur().get_longitude()));
  corners.push_back(vnl_double_2(corner_points.corner_ll().get_latitude(),
                                 corner_points.corner_ll().get_longitude()));
  corners.push_back(vnl_double_2(corner_points.corner_lr().get_latitude(),
                                 corner_points.corner_lr().get_longitude()));

  if (frame_to_ref.get_source_reference() != ts)
  {
    // Log the error and ignore the source reference
    LOG_ERROR( "Source reference of homography does not match frame timestamp" );
  }

  stream.clear_serialisation_records();
  vsl_b_write(stream, u_seconds);
  if (write_image)
  {
    if (this->compress_image_)
    {
      assert(this->data_version_==3);
      vsl_b_write(stream, 'J'); // J=jpeg
      vil_stream* mem_stream = new vil_stream_core();
      mem_stream->ref();
      vil_jpeg_file_format fmt;
      vil_image_resource_sptr img_res
        = fmt.make_output_image(mem_stream,
                                byte_view.ni(), byte_view.nj(), byte_view.nplanes(),
                                VIL_PIXEL_FORMAT_BYTE);
      img_res->put_view(byte_view);
      this->image_write_cache_.resize(mem_stream->file_size());
      mem_stream->seek(0);
      LOG_DEBUG("Compressed image is " << mem_stream->file_size() << " bytes");
      mem_stream->read(&this->image_write_cache_[0], mem_stream->file_size());
      vsl_b_write(stream, this->image_write_cache_);
      mem_stream->unref();
    }
    else
    {
      assert(this->data_version_==2);
      vsl_b_write(stream, byte_view);
    }
  }
  vsl_b_write(stream, homog);
  vsl_b_write(stream, corners);
  vsl_b_write(stream, gsd);
  vsl_b_write(stream, frame_num);
  vsl_b_write(stream, ref_frame_num);
  vsl_b_write(stream, static_cast<vxl_int_64>(byte_view.ni()));
  vsl_b_write(stream, static_cast<vxl_int_64>(byte_view.nj()));
}


} // end namespace vidtk
