/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_rgb2yuv_h_
#define vidtk_rgb2yuv_h_

#include <limits>

namespace vidtk
{

template<class PIX_TYPE>
void rgb2yuv( PIX_TYPE R, PIX_TYPE G, PIX_TYPE B, PIX_TYPE * yuv )
{
  // thanks to http://en.wikipedia.org/wiki/YUV

  /*
  // these integer approximations seem to give different results than
  // the explicit double-valued equations...
  int y = ( ( 66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
  int u = ( ( -38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
  int v = ( ( 112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
  */

  double min_v = std::numeric_limits<PIX_TYPE>::min();
  double max_v = std::numeric_limits<PIX_TYPE>::max();

  double rd = (R-min_v)/(max_v-min_v), gd=(G-min_v)/(max_v-min_v), bd=(B-min_v)/(max_v-min_v);
  double yd = (0.299*rd) + (0.587*gd) + (0.114*bd);
  double ud = 0.436*(bd - yd) / (1.0 - 0.114);
  double vd = 0.615*(rd - yd) / (1.0 - 0.299);

  yuv[0] = static_cast<PIX_TYPE>( yd * max_v + min_v );
  yuv[1] = static_cast<PIX_TYPE>( (ud+0.436) * (max_v/(2*0.436))+min_v );
  yuv[2] = static_cast<PIX_TYPE>( (vd+0.615) * (max_v/(2*0.615))+min_v );
}

} // end namespace vidtk

#endif // end vidtk_rgb2yuv_h_
