/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_writer_kw18.h"

#include <logger/logger.h>
#include <vul/vul_file.h>


namespace vidtk {

VIDTK_LOGGER ("image_object_writer_kw18");

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
image_object_writer_kw18
::image_object_writer_kw18()
  : track_id_(1),
    fstr_ (0)
{ }

image_object_writer_kw18
::~image_object_writer_kw18()
{
  delete fstr_;
}


// ----------------------------------------------------------------
bool
image_object_writer_kw18
::initialize(config_block const& blk)
{
  track_id_ = 1;

  // initialize the base class
  if (image_object_writer::initialize(blk) == false)
  {
    return false;
  }

  // Force .kw18 extension
  std::string fn_only = vul_file::strip_directory( filename_ );
  // Just in case the path includes a '.', strip_extension() is being supplied
  // fn_only
  std::string fn = vul_file::dirname( filename_ ) + "/" +
    vul_file::strip_extension(fn_only) + ".kw18";

  delete fstr_;
  fstr_ = new std::ofstream( fn.c_str() );
  if( ! *fstr_ )
  {
    LOG_ERROR("Couldn't open " << fn << " for writing." );
    return false;
  }

  // write header
  *fstr_ << "#1:Track-id 2:Track-length 3:Frame-number "
    "4:Tracking-plane-loc(x) 5:Tracking-plane-loc(y) "
    "6:velocity(x) 7:velocity(y) 8:Image-loc(x) 9:Image-loc(y) "
    "10:Img-bbox(TL_x) 11:Img-bbox(TL_y) 12:Img-bbox(BR_x) 13:Img-bbox(BR_y) "
    "14:Area "
    "15:World-loc(longitude) 16:World-loc(latitude) 17:World-loc(altitude) "
    "18:timesetamp(seconds) 19:detection-confidence\n"
    "# Written by ImageObjectWriterKW18\n";

  return true;
}


// ----------------------------------------------------------------
void
image_object_writer_kw18
::write( timestamp const& ts, std::vector< image_object_sptr > const& objs )
{
  if(fstr_ == NULL)
  {
    LOG_ERROR("Stream is NULL" );
    return;
  }
  fstr_->precision( 6 );

  size_t N = objs.size();
  for( size_t i = 0; i < N; ++i )
  {
    image_object const& o = *objs[i];


    *fstr_ << track_id_++ // track id
           << " 1 ";      // always one frame in length

    if( ts.has_frame_number() )
    {
      *fstr_ << ts.frame_number() << " ";
    }
    else
    {
      *fstr_ << "-1 ";
    }
    vgl_box_2d< unsigned > const& bbox = o.get_bbox();
    *fstr_ << "0 0 "           // tracking plane location (4:5)
           << "0 0 "            // velocity (6:7)
           << o.get_image_loc() << " " // image location (8:9)
           << bbox.min_x() << " " // bounding box (10:13)
           << bbox.min_y() << " "
           << bbox.max_x() << " "
           << bbox.max_y() << " "
           << o.get_area() << " " // area (14)
           << o.get_world_loc() << " "  // tracking plane loc / world loc (15:17)
      ;

    if( ts.has_time() )
    {
      *fstr_ << ts.time_in_secs() << " ";
    }
    else
    {
      *fstr_ << "-1 ";
    }

    *fstr_ << o.get_confidence();
    *fstr_ << std::endl;
    fstr_->flush();

  } // end for
}

} // end namespace
