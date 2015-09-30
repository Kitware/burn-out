/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//test_image_sequence_accessor.cxx

#include <vul/vul_arg.h>
#include <vul/vul_file.h>
#include <vul/vul_sprintf.h>
#include <vul/vul_awk.h>
#include <vul/vul_string.h>
#include <vul/vul_timer.h>

#include <video/image_sequence_accessor.h>

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> inputSequenceArg( "--input", "The input image sequence to crop");
  vul_arg_parse( argc, argv );

  vidtk::image_sequence_accessor isa(inputSequenceArg(), vidtk::image_sequence_accessor::image_list_file);
  if(isa.is_valid())
  {
    vcl_cout << "opening worked" << vcl_endl;
  }
  if(isa.seek_frame(100))
  {
    vcl_cout << "seek forward works" << vcl_endl;
  }

  if(isa.seek_frame(10))
  {
    vcl_cout << "seek forward works" << vcl_endl;
  }

  vil_image_view<vxl_byte> const& image = isa.get_image();
  if(image)
  {
    vcl_cout << "image access works" << vcl_endl;
  }

  if(isa.seek_to_next())
  {
    vcl_cout << "seek next works" << vcl_endl;
  }

  bool set_roi = isa.set_roi("100x100+5+5");
  vil_image_view<vxl_byte> const& image2 = isa.get_image();
  if(set_roi && image2.ni() == 100 && image2.nj() == 100 )
  {
    vcl_cout << "string roi works" << vcl_endl;
  }
  isa.reset_roi();
  set_roi = isa.set_roi( 5,5,150,150);
  vil_image_view<vxl_byte> const& image3 = isa.get_image();
  if(set_roi && image3.ni() == 150 && image3.nj() == 150 )
  {
    vcl_cout << "number roi works" << vcl_endl;
  }

  isa.reset_roi();
  vcl_vector< vil_image_view<vxl_byte> > results;
  bool works = isa.get_frame_range(10,14,results) && results.size() == 5;
  for( unsigned int i = 0; i < 5; ++i )
  {
    works = works && results[i].ni() != 150 && results[i].nj() != 150;
  }
  if(works)
  {
    vcl_cout << "reading a set works" << vcl_endl;
  }

  return 0;
}
