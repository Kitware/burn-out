/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_reader_kw18.h"

#include <vil/vil_load.h>
#include <vil/vil_new.h>
#include <vil/vil_image_resource.h>
#include <vil/vil_crop.h>
#include <vil/vil_fill.h>
#include <vul/vul_file.h>
#include <vul/vul_file_iterator.h>

#include <utilities/shell_comments_filter.h>
#include <utilities/blank_line_filter.h>
#include <logger/logger.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <fstream>
#include <stdio.h>

namespace vidtk {
namespace ns_image_object_reader {

namespace {

VIDTK_LOGGER ("image_object_reader_kw18");

  // field numbers for KW18 file format
  enum{
    // 0: Object ID
    // 1: Track length (always 1 for detections)
    COL_FRAME = 2,
    COL_LOC_X,    // 3
    COL_LOC_Y,    // 4
    COL_VEL_X,    // 5
    COL_VEL_Y,    // 6
    COL_IMG_LOC_X,// 7
    COL_IMG_LOC_Y,// 8
    COL_MIN_X,    // 9
    COL_MIN_Y,    // 10
    COL_MAX_X,    // 11
    COL_MAX_Y,    // 12
    COL_AREA,     // 13
    COL_WORLD_X,  // 14
    COL_WORLD_Y,  // 15
    COL_WORLD_Z,  // 16
    COL_TIME,     // 17
    COL_CONFIDENCE// 18
  };

} // end namepsace

// ----------------------------------------------------------------
/** Constructor
 *
 */
image_object_reader_kw18
::image_object_reader_kw18()
  : in_stream_(0)
{ }


image_object_reader_kw18
::~image_object_reader_kw18()
{
  delete in_stream_;
}


// ----------------------------------------------------------------
bool
image_object_reader_kw18
::open(std::string const& filename)
{
  if (0 != in_stream_) // meaning we are already open
  {
    return true;
  }

  filename_ = filename;

  std::string const file_ext( filename_.substr(filename_.rfind(".")+1) );
  if ( ! (file_ext == "kw18" || file_ext == "trk") )
  {
    return false;
  }

  std::ifstream* fstr = new std::ifstream( this->filename_.c_str() );
  if ( ! *fstr )
  {
    LOG_ERROR( "Couldn't open " << this->filename_ << " for reading." );
    return false;
  }

  in_stream_ = new boost::iostreams::filtering_istream();

  in_stream_->push (vidtk::blank_line_filter());
  in_stream_->push (vidtk::shell_comments_filter());
  in_stream_->push (*fstr);

  return true;
}


// ----------------------------------------------------------------
bool
image_object_reader_kw18
::read_next(ns_image_object_reader::datum_t& datum)
{
  if ( in_stream_ == NULL )
  {
    LOG_ERROR( "Stream is null." );
    return false;
  }
  if ( in_stream_->eof() )
  {
    return false;
  }

  std::string line;
  while ( std::getline(*in_stream_, line) )
  {
    std::vector< std::string > col;

    col.clear();
    boost::char_separator < char > sep(" ");
    typedef boost::tokenizer < boost::char_separator<char> > tok_t;
    tok_t tok( line, sep );
    for(tok_t::iterator it = tok.begin(); it != tok.end(); ++it)
    {
      col.push_back( *it );
    }

    // skip lines suppressed by filtered stream
    if ( col.empty() )
    {
      continue;
    }

    if ( ( col.size() < 18 ) || ( col.size() > 20 ) )
    {
      LOG_ERROR( "This is not a kw18 kw19 or kw20 file; found " << col.size() <<
                 " columns in\n\"" << line << "\"" );
      return false;
    }

    // get timestamp - need frame number and timestamp
    datum.first = vidtk::timestamp( atof( col[COL_TIME].c_str() )*1e6, // timestamp in seconds
                                    atoi( col[COL_FRAME].c_str() ));

    image_object * obj = new image_object();
    datum.second = obj;

    if ( atof( col[COL_IMG_LOC_X].c_str() ) != -1.0 )
    {
      // only pass on the object if it is 'valid'
      obj->set_image_loc( atof( col[COL_IMG_LOC_X].c_str() ),
                          atof( col[COL_IMG_LOC_Y].c_str() ));

      int min_x = atoi( col[COL_MIN_X].c_str() );
      int max_x = atoi( col[COL_MAX_X].c_str() );
      int min_y = atoi( col[COL_MIN_Y].c_str() );
      int max_y = atoi( col[COL_MAX_Y].c_str() );

      obj->set_bbox(
        ( min_x < 0 ? 0 : min_x ),
        ( max_x < 0 ? 0 : max_x ),
        ( min_y < 0 ? 0 : min_y ),
        ( max_y < 0 ? 0 : max_y ) );

      obj->set_image_area( obj->get_bbox().volume() );

      obj->set_area( atof( col[COL_AREA].c_str() ) );

      obj->set_world_loc(atof( col[COL_WORLD_X].c_str() ), // x
                         atof( col[COL_WORLD_Y].c_str() ), // y
                         atof( col[COL_WORLD_Z].c_str() )); // z

      if (atof(col[COL_CONFIDENCE].c_str()) != -1.0)
      {
        obj->set_confidence(atof(col[COL_CONFIDENCE].c_str()));
      }

      return true;
    }
    else
    {
      LOG_DEBUG("Image X location is -1, invalid object, throwing it out");
    }
  }

  return false;
}

} // end namespace ns_image_object_reader
} // end namespace vidtk
