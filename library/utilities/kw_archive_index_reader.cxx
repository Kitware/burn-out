/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "kw_archive_index_reader.h"

#include <vul/vul_file.h>

#include <vil/file_formats/vil_jpeg.h>
#include <vil/vil_stream_core.h>

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vnl/io/vnl_io_matrix_fixed.h>
#include <vnl/io/vnl_io_vector_fixed.h>
#include <vil/io/vil_io_image_view.h>

#include <logger/logger.h>

// Create static default logger for log macros
VIDTK_LOGGER("kw_archive_index_reader");

namespace vidtk
{

kw_archive_index_reader::
kw_archive_index_reader()
  : info_stream_(NULL),
    info_b_stream_(NULL),
    have_data_file_( false ),
    compressed_image_( false ),
    gsd_( 0 )
{
}


kw_archive_index_reader::
~kw_archive_index_reader()
{
  delete this->info_b_stream_;
  delete this->info_stream_;
}


bool
kw_archive_index_reader
::open(std::string index_filename)
{
  std::ifstream index(index_filename.c_str());
  if (!index)
  {
    LOG_DEBUG("Failed to open index file " << index_filename);
    return false;
  }

  // Validate the version numbers of the index file
  int version;
  std::string archive_file;
  std::string meta_file;

  // Parse out the first line as an integer version
  {
    std::string version_string;
    if (!std::getline(index, version_string))
    {
      LOG_DEBUG("Failed to get the version line of index file in "
                << index_filename);
      return false;
    }
    std::istringstream str(version_string);
    str >> version;
    if (!str)
    {
      LOG_DEBUG("Failed to parse version of index file in "
                << index_filename);
      return false;
    }
  }
  if(version==3)
  {
    if (!std::getline(index, archive_file)) return false;
    if (!std::getline(index, meta_file)) return false;
    if (!std::getline(index, this->mission_id_)) return false;
  }
  else if(version==4)
  {
    if (!std::getline(index, archive_file)) return false;
    if (!std::getline(index, meta_file)) return false;
    if (!std::getline(index, this->mission_id_)) return false;
    if (!std::getline(index, this->stream_id_)) return false;
  }
  else
  {
    LOG_DEBUG("Cannot handle version=" << version << " files");
    return false;
  }

  if (!index)
  {
    LOG_DEBUG("Failed to parse header of " << index_filename);
    return false;
  }

  // The main content of the archive is in two places: a .data file
  // and a .meta file. The contents of the .meta file is the same as
  // the .data file except for the pixel data, so we prefer the .data
  // file to the .meta file.
  std::string dirname = vul_file::dirname(index_filename);
  std::string info_filename;
  if (vul_file::exists(archive_file))
  {
    info_filename = archive_file;
    this->have_data_file_ = true;
  }
  else if (vul_file::exists(dirname+"/"+archive_file))
  {
    info_filename = dirname+"/"+archive_file;
    this->have_data_file_ = true;
  }
  else if (vul_file::exists(meta_file))
  {
    info_filename = meta_file;
    this->have_data_file_ = false;
  }
  else if (vul_file::exists(dirname+"/"+meta_file))
  {
    info_filename = dirname+"/"+meta_file;
    this->have_data_file_ = false;
  }
  else
  {
    LOG_DEBUG("Failed to find .data or .meta file corresponding to index file "
              << index_filename << " (" << archive_file << " and " << meta_file
              << ")");
    return false;
  }

  delete this->info_stream_;
  this->info_stream_= new vidtk::large_file_ifstream(info_filename.c_str(),
                                                     std::ios::in|std::ios::binary);
  if (!*this->info_stream_)
  {
    LOG_DEBUG("Failed to open " << info_filename);
    return false;
  }

  delete this->info_b_stream_;
  this->info_b_stream_ = new vsl_b_istream(this->info_stream_);
  if (!*this->info_b_stream_)
  {
    LOG_DEBUG("Failed to binary stream for " << info_filename);
    return false;
  }

  LOG_DEBUG("Opened binary stream for " << info_filename);

  version=0;
  vsl_b_read( *this->info_b_stream_, version );
  if (this->have_data_file_)
  {
    if (version==2)
    {
      this->compressed_image_ = false;
    }
    else if (version==3)
    {
      this->compressed_image_ = true;
    }
    else
    {
      LOG_DEBUG("Can only handle version 2 and 3 .data files");
      return false;
    }
  }
  else
  {
    if (version!=2)
    {
      LOG_DEBUG("Can only handle version 2 .meta files");
      return false;
    }
  }

  // Okay, everything seems good.  Load up the index file into a
  // in-memory map. We do this after checking for the existence of the
  // .data or .meta file and so on to return quickly on error (as
  // opposed to parsing the .index file and then finding that the
  // other required file is missing).
  //
  vxl_int_64 ts, pos;
  while (index >> ts >> pos)
  {
    this->index_[ts] = pos;
  }

  // We should've failed due to EOF, not because we couldn't parse
  // something.
  if (index.eof())
  {
    return true;
  }
  else
  {
    LOG_DEBUG("Failed while parsing .index file");
    return false;
  }
}


std::string const&
kw_archive_index_reader
::mission_id() const
{
  return this->mission_id_;
}


std::string const&
kw_archive_index_reader
::stream_id() const
{
  return this->stream_id_;
}


bool
kw_archive_index_reader
::read_next_frame()
{
  if( this->info_stream_ == NULL || this->info_b_stream_ == NULL )
  {
    LOG_DEBUG("One of the streams are NULL");
    return false;
  }
  // The vsl binary streams are very verbose when an error occurs. We
  // expect the reads to fail at EOF, so instead of haven't it
  // verbosely fail, we check for EOF first and exit quietly in that
  // case. If not EOF, then any failure of the reads below indicate
  // real failures, and the verbosity is warranted.
  this->info_stream_->peek();
  if (this->info_stream_->eof())
  {
    return false;
  }

  vxl_int_64 u_seconds;
  vxl_int_64 frame_num;
  vxl_int_64 img_width, img_height;
  vxl_int_64 homog_target;
  std::vector< vnl_vector_fixed< double, 2 > > corners;
  vnl_matrix_fixed< double, 3, 3 > frame_to_ref;
  vil_image_view<vxl_byte> img;
  double lgsd;

  this->info_b_stream_->clear_serialisation_records();
  vsl_b_read(*this->info_b_stream_, u_seconds);
  if (this->have_data_file_)
  {
    if (!this->compressed_image_)
    {
      vsl_b_read(*this->info_b_stream_, img);
    }
    else
    {
      char fmt_marker;
      vsl_b_read(*this->info_b_stream_, fmt_marker);
      vil_file_format* fmt;
      if (fmt_marker=='J')
      {
        static vil_jpeg_file_format jpeg_fmt;
        fmt = &jpeg_fmt;
      }
      else
      {
        LOG_ERROR("Unknown compression format " << fmt_marker);
        return false;
      }
      vil_stream* mem_stream = new vil_stream_core();
      mem_stream->ref();
      std::vector<char> bytes;
      vsl_b_read(*this->info_b_stream_, bytes);
      mem_stream->write(&bytes[0], bytes.size());
      vil_image_resource_sptr img_res = fmt->make_input_image(mem_stream);
      img = img_res->get_view();
      mem_stream->unref();
    }
  }
  vsl_b_read(*this->info_b_stream_, frame_to_ref);
  vsl_b_read(*this->info_b_stream_, corners);
  vsl_b_read(*this->info_b_stream_, lgsd);
  vsl_b_read(*this->info_b_stream_, frame_num);
  vsl_b_read(*this->info_b_stream_, homog_target);
  vsl_b_read(*this->info_b_stream_, img_width);
  vsl_b_read(*this->info_b_stream_, img_height);

  if (!*this->info_stream_)
  {
    return false;
  }

  this->ts_ = vidtk::timestamp(static_cast<double>(u_seconds),
                               static_cast<unsigned int>(frame_num));

  this->points_
    .corner_ul(vidtk::geo_coord::geo_lat_lon(corners[0][0], corners[0][1]))
    .corner_ur(vidtk::geo_coord::geo_lat_lon(corners[1][0], corners[1][1]))
    .corner_ll(vidtk::geo_coord::geo_lat_lon(corners[2][0], corners[2][1]))
    .corner_lr(vidtk::geo_coord::geo_lat_lon(corners[3][0], corners[3][1]));

  this->image_ = img;

  this->frame_to_ref_.set_transform(frame_to_ref);
  this->frame_to_ref_.set_source_reference(this->ts_);
  vidtk::timestamp dest_ts;
  dest_ts.set_frame_number(static_cast<unsigned int>(homog_target));
  this->frame_to_ref_.set_dest_reference(dest_ts);

  this->gsd_ = lgsd;

  return true;
}


bool
kw_archive_index_reader
::eof() const
{
  return this->info_stream_->eof();
}


vidtk::timestamp const&
kw_archive_index_reader
::timestamp() const
{
  return this->ts_;
}


vil_image_view<vxl_byte> const&
kw_archive_index_reader
::image() const
{
  return this->image_;
}


vidtk::video_metadata const&
kw_archive_index_reader
::corner_points() const
{
  return this->points_;
}


vidtk::image_to_image_homography const&
kw_archive_index_reader
::frame_to_reference() const
{
  return this->frame_to_ref_;
}


double
kw_archive_index_reader
::gsd() const
{
  return this->gsd_;
}


} // end namespace vidtk
