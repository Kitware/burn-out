/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_sequence_accessor.h"

#include <video_io/generic_frame_process.h>

namespace vidtk
{

image_sequence_accessor
::image_sequence_accessor(std::string fname, type t)
{
  frame_process_ = new generic_frame_process< vxl_byte >("src");
  config_block params = frame_process_->params();
  //params.print( std::cout );
  std::string image_type[] = {"image_list:glob","image_list:file"};
  switch(t)
  {
    case image_list_glob:
    case image_list_file:
      params.set( "type", "image_list" );
      params.set( image_type[t], fname );
      break;
    case stream:
      params.set( "type", "vidl_ffmpeg" );
      params.set( "vidl_ffmpeg:filename", fname );
      break;
  }
  is_valid_ = frame_process_->set_params( params ) && frame_process_->initialize();
}

vil_image_view<vxl_byte>
image_sequence_accessor
::get_image()
{
  return frame_process_->image();
}

bool
image_sequence_accessor
::seek_frame(unsigned int frame_number)
{
  return frame_process_->seek(frame_number);
}

bool
image_sequence_accessor
::seek_to_next()
{
  return frame_process_->step();
}

bool
image_sequence_accessor
::set_roi(std::string roi)
{
  return frame_process_->set_roi(roi);
}

bool
image_sequence_accessor
::set_roi( unsigned int x, unsigned int y,
           unsigned int w, unsigned int h )
{
  return frame_process_->set_roi(x,y,w,h);
}

void
image_sequence_accessor
::reset_roi()
{
  frame_process_->reset_roi();
}

bool
image_sequence_accessor
::get_frame_range(unsigned int begin, unsigned int end,
                  std::vector< vil_image_view<vxl_byte> > & results )
{
  bool result = this->seek_frame(begin);
  for(unsigned int i = begin; result && i <= end; ++i)
  {
    vil_image_view<vxl_byte> const& img = frame_process_->image();
    if(img)
    {
      results.push_back(img);
    }
    result = result && frame_process_->step();
  }
  return result;
}

bool
image_sequence_accessor
::is_valid()
{
  return this->is_valid_;
}

}
