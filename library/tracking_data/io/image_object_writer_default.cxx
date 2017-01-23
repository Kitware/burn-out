/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "image_object_writer_default.h"

#include <logger/logger.h>
#include <vul/vul_file.h>
#include <iomanip>
#include <limits>


namespace vidtk
{

VIDTK_LOGGER ("image_object_writer_default");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
image_object_writer_default
::image_object_writer_default()
  : fstr_ (0)
{ }


image_object_writer_default
::~image_object_writer_default()
{
  delete fstr_;
}

// ----------------------------------------------------------------
bool
image_object_writer_default
::initialize(config_block const& blk)
{
  // initialize the base class
  if (image_object_writer::initialize(blk) == false)
  {
    return false;
  }

  // Force .det extension
  std::string fn_only = vul_file::strip_directory( filename_ );
  // Just in case the path includes a '.', strip_extension() is being supplied
  // fn_only
  std::string fn = vul_file::dirname( filename_ ) + "/" +
    vul_file::strip_extension(fn_only) + ".det";

  delete fstr_;
  fstr_ = new std::ofstream( fn.c_str() );
  if( ! *fstr_ )
  {
    LOG_ERROR( "Couldn't open " << fn << " for writing." );
    return false;
  }

  // write header
  *fstr_ << "# 1:Frame-number  2:Time  3-5:World-loc(x,y,z)  6-7:Image-loc(i,j)  "
    "8:Area   9-12:Img-bbox(i0,j0,i1,j1)\n"
    "# Written by ImageObjectWriterDefault\n";

  return true;
}


// ----------------------------------------------------------------
void
image_object_writer_default
::write( timestamp const& ts, std::vector< image_object_sptr > const& objs )
{
  if( fstr_ == NULL )
  {
    LOG_ERROR( "Stream is NULL" );
    return;
  }
  // if you change this format, change the documentation above and
  // change the header output in initialize();

  size_t N = objs.size();
  for( size_t i = 0; i < N; ++i )
  {
    image_object const& o = *objs[i];

    if( ts.has_frame_number() )
    {
      *fstr_ << ts.frame_number() << " ";
    }
    else
    {
      *fstr_ << "-1 ";
    }

    if( ts.has_time() )
    {
      *fstr_ << std::setprecision(std::numeric_limits<long double>::digits10 + 1)
             << ts.time() << " ";
    }
    else
    {
      *fstr_ << "-1 ";
    }

    const vgl_box_2d< unsigned >& bbox = o.get_bbox();
    *fstr_ << o.get_world_loc() << " "
           << o.get_image_loc() << " "
           << o.get_area() << " "
           << bbox.min_x() << " "
           << bbox.min_y() << " "
           << bbox.max_x() << " "
           << bbox.max_y() << "\n";
  }
  fstr_->flush();
}
} // end namespace
