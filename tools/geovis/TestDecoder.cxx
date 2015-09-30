/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "video/vidl_ffmpeg_frame_process.h"

#include <iostream>

int main(int argc,
         char *argv[])
{
  if(argc!=2)
    {
    std::cerr << "video filename missing in argument" << std::endl;
    return 1;
    }
  
  std::string fileName=argv[1];
  
  vidtk::vidl_ffmpeg_frame_process *decoder=
    new vidtk::vidl_ffmpeg_frame_process("reader_process");
  
  vidtk::config_block c;
  c.add_parameter("filename","name of the file");
  c.set("filename",fileName);
    // c.set("start_at_frame",0); // optional
    // c.set("stop_after_frame",1); // optional
    
  decoder->set_params(c);
  bool status=decoder->initialize();
  if(!status)
    {
    std::cerr <<"failed to initialized the decoder" << std::endl;
    return 0;
    }
  
  // loop over each frame
  
  int frame=0;
  while(frame<100)
    {
    std::cout << "frame=" << frame << std::endl;
  
//    status=decoder->seek(frame);
  
    if(!status)
      {
      std::cerr <<"failed to seek frame" << frame << "." << std::endl;
      return 0;
      }
  
    status=decoder->step();
    if(!status)
      {
      std::cerr <<"failed to step." << std::endl;
      return 0;
      }
    
    vil_image_view<vxl_byte> image=decoder->image();
//    vil_image_view<vxl_byte> image=decoder->image();
    
    int w=static_cast<int>(image.ni());
    int h=static_cast<int>(image.nj());
    
    std::cout << "w=" << w << " h="<< h
              << " nplanes=" << image.nplanes() << std::endl;
    
    ++frame;
    }
  
  return 1;
}
