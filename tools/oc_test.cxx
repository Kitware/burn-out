/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "vcl_iostream.h"
#include "vcl_sstream.h"
#include "utilities/oc_timestamp_hack.h"
#include "utilities/timestamp.h"

//
// should produce e.g.
//
// $ tools/oc_test 59419
// 59419 -> 1.19517e+09
// $ tools/oc_test 59420
// Not found
// $ tools/oc_test 1
// 1 -> 1.19508e+09
//


int main(int argc, char *argv[])
{
  if (argc != 2) {
    vcl_cerr << "Usage: " << argv[0] << " frame_num\n";
    return 1;
  }

  vidtk::oc_timestamp_hack oc_hack;

  if (!oc_hack.load_file("/mnt/video1/video_data/ocean_city_webcam/raw/downtown_day01_log.txt")) {
    vcl_cerr << "Couldn't init hack object\n";
    return 1;
  }

  vcl_istringstream ss( argv[1] );
  unsigned f;
  ss >> f;

  double d;
  if (oc_hack.get_time_of_day( f, d )) {
    vcl_cerr << f << " -> " << d << "\n";
  } else {
    vcl_cerr << "Not found\n";
  }
}


