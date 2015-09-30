/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

// more like a test, really...
// Given a r(ead) or w(rite) flag and an image:
// - if reading, try to read the timestamp and strip it from the image.
// - if writing, try to write the current time to the image.
// Either way, the resulting image is written to ts-out.png.
//

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>

#include <utilities/timestamp.h>
#include <utilities/timestamp_image.h>

int main( int argc, char *argv[] )
{
  if ((argc != 3) || ((argv[1][0] != 'r') && (argv[1][0] != 'w'))) {
    vcl_cerr << "Usage: " << argv[0] << " r|w image_file\n";
    return 1;
  }

  vil_image_view<vxl_byte> src_img = vil_load( argv[2] );
  if (!src_img) {
    vcl_cerr << "Couldn't read '" << argv[2] << "'; exiting\n";
    return 1;
  }

  vil_image_view<vxl_byte> dst_img;

  if (argv[1][0] == 'r') {

    vidtk::timestamp ts = vidtk::get_timestamp( src_img, dst_img );
    if (ts.has_time() || ts.has_frame_number()) {
      vil_save( dst_img, "ts-out.png" );
      vcl_cerr << "Timestamp was "
	       << ts.time() << " : " << ts.frame_number() << "\n";
    } else {
      vcl_cerr << "Couldn't read timestamp\n";
    }

  } else {

    vidtk::add_timestamp( src_img, dst_img );
    vil_save( dst_img, "ts-out.png" );

  }
}



