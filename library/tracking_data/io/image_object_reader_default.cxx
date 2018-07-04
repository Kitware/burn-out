/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "image_object_reader_default.h"

#include <utilities/shell_comments_filter.h>
#include <utilities/blank_line_filter.h>
#include <logger/logger.h>

#include <vul/vul_file.h>

#include <iostream>
#include <fstream>
#include <stdio.h>


namespace vidtk {
namespace ns_image_object_reader {


VIDTK_LOGGER ("image_object_reader_default");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
image_object_reader_default
::image_object_reader_default()
  : in_stream_(0)
{ }


image_object_reader_default
::~image_object_reader_default()
{
  delete in_stream_;
}


// ----------------------------------------------------------------
bool
image_object_reader_default
::open(std::string const& filename)
{
  if (0 != in_stream_) // meaning we are already open
  {
    return true;
  }

  filename_ = filename;

  // Currently the only semantic test for our file type is the file
  // extension
  std::string const file_ext( filename_.substr(filename_.rfind(".")+1) );
  if (file_ext != "det")
  {
    return false;
  }

  std::ifstream* fstr = new std::ifstream( this->filename_.c_str() );
  if( ! *fstr )
  {
    LOG_ERROR( "Couldn't open " << this->filename_ << " for reading." );
    return false;
  }

  // build boost filter to remove comments and blank lines
  // order is important.
  in_stream_ = new boost::iostreams::filtering_istream();

  in_stream_->push (vidtk::blank_line_filter());
  in_stream_->push (vidtk::shell_comments_filter());
  in_stream_->push (*fstr);

  return true;
}


// ----------------------------------------------------------------
bool
image_object_reader_default
::read_next(ns_image_object_reader::datum_t& datum)
{
  if( in_stream_ == NULL )
  {
    LOG_ERROR( "Stream is null." );
    return false;
  }
  if( in_stream_->eof() )
  {
    return false;
  }

  while (1) // need to skip "bad" objects
  {
    unsigned frame_num;
    double first_time;

    *in_stream_ >> frame_num;
    *in_stream_ >> first_time;
    datum.first = vidtk::timestamp(first_time, frame_num);

    // Read all the (valid) objects with next timestamp.  -1 being
    // invalid values.

    image_object * obj = new image_object();
    datum.second = obj;
    std::string buffer;
    unsigned tmp[4];
    tracker_world_coord_type wld_loc;
    vidtk_pixel_coord_type img_loc;
    float_type area;

    std::getline (*in_stream_, buffer);
    int count = sscanf (buffer.c_str(),
                        "%lf %lf %lf " // world loc
                        "%lf %lf "     // img loc
                        "%lf "         // area
                        "%u %u %u %u " // bounding box
                        ,
                        &wld_loc[0],
                        &wld_loc[1],
                        &wld_loc[2],

                        &img_loc[0],
                        &img_loc[1],

                        &area,

                        &tmp[0], &tmp[1], &tmp[2], &tmp[3]
      );

    if (count != 10)
    {
      if (count != EOF)
      {
        LOG_ERROR("Invalid format record from file " << filename_);
      }
      return false;  // unrecoverable
    }

    if ( !in_stream_->good() )
    {
      LOG_ERROR("Error reading file " << filename_);
      return false;
    }

    if( img_loc[0] != -1.0 )
    {
      //Only pass on the object if it is 'valid'
      obj->set_world_loc( wld_loc );
      obj->set_image_loc( img_loc );

      vgl_box_2d<unsigned> bbox;
      bbox.set_min_x(tmp[0]);
      bbox.set_min_y(tmp[1]);
      bbox.set_max_x(tmp[2]);
      bbox.set_max_y(tmp[3]);
      obj->set_bbox( bbox );

      obj->set_image_area( bbox.volume() );
      obj->set_area( area );

      return true;
    }
    else
    {
      LOG_WARN("Frame: " << frame_num  << ", Image X location is -1, invalid object, throwing it out");
    }

  } // end while

    return false;
}

} // end namespace
} // end namespace
