/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_save_unsigned_image_h_
#define vidtk_save_unsigned_image_h_

#include <vil/vil_save.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

void save_unsigned_image( vil_image_view< unsigned > corr_surf,
                          std::string fname )
{
  double min = 2e30;
  double max = 0;
  for( unsigned int i = 0; i < corr_surf.ni(); ++i )
  {
    for( unsigned int j = 0; j < corr_surf.nj(); ++j )
    {
      if(corr_surf(i,j) < min) min = corr_surf(i,j);
      if(corr_surf(i,j) > max) max = corr_surf(i,j);
    }
  }
  double mid = (min+max) * 0.5;
  vil_image_view< vxl_byte > img(corr_surf.ni(),corr_surf.nj(),3);
  for( unsigned int i = 0; i < corr_surf.ni(); ++i )
  {
    for( unsigned int j = 0; j < corr_surf.nj(); ++j )
    {
      unsigned r = 0, g = 0, b = 0;
      double v = corr_surf(i,j);
      if(v<mid)
      {
        double t = (mid-min);
        double s = (v-min)/(t?t:1);
        r = (vxl_byte)((1-s)*255);
        b = (vxl_byte(s*255));
      }
      else
      {
        double t = (max-mid);
        double s = (v-mid)/(t?t:1);
        if(s>1.0){ s = 1.0; }
        g = (vxl_byte((s)*255));
        b =(vxl_byte)((1-s)*255);
      }
      img(i,j,0) = r;
      img(i,j,1) = g;
      img(i,j,2) = b;
    }
  }
  vil_save(img, fname.c_str());
}

} // vidtk

#endif //vidtk_save_unsigned_image_h_ 
