/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_rgb2yuv_h_
#define vidtk_rgb2yuv_h_

namespace vidtk
{

void rgb2yuv( vxl_byte R, vxl_byte G, vxl_byte B, unsigned char* yuv ) 
{
  // thanks to http://en.wikipedia.org/wiki/YUV

  /*
  // these integer approximations seem to give different results than
  // the explicit double-valued equations...
  int y = ( ( 66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
  int u = ( ( -38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
  int v = ( ( 112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
  */

  double rd = R/255.0, gd=G/255.0, bd=B/255.0;
  double yd = (0.299*rd) + (0.587*gd) + (0.114*bd);
  double ud = 0.436*(bd - yd) / (1.0 - 0.114);
  double vd = 0.615*(rd - yd) / (1.0 - 0.299);

  yuv[0] = static_cast<unsigned char>( yd * 255 );
  yuv[1] = static_cast<unsigned char>( (ud+0.436) * (255.0/(2*0.436)) );
  yuv[2] = static_cast<unsigned char>( (vd+0.615) * (255.0/(2*0.615)) );
}

void rgb2yuv( vxl_uint_16 R, vxl_uint_16 G, vxl_uint_16 B, unsigned char* yuv ) 
{
  vxl_byte Rb = R >> 8, Gb = G >> 8, Bb = B >> 8;
  rgb2yuv(Rb, Gb, Bb, yuv);
}

} // end namespace vidtk

#endif // end vidtk_rgb2yuv_h_
