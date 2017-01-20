/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "draw_data.h"

namespace vidtk
{

template< class PixType>
draw_data< PixType >
::draw_data( unsigned int dot_width )
 : dot_width_( dot_width )
{
}

template< class PixType>
draw_data< PixType >
::~draw_data()
{
}


template< class PixType>
void
draw_data< PixType >
::draw_dot( vil_image_view< PixType >& img,
            unsigned i, unsigned j,
            unsigned int rgb[] )
{
  unsigned const np = img.nplanes();
  for( unsigned p = 0; p < np; ++p )
  {
    img(i,j,p) = rgb[p];
  }
}


template< class PixType>
void
draw_data< PixType >
::draw_variable_size_dot( vil_image_view< PixType >& img,
                          unsigned i, unsigned j,
                          unsigned int rgb[] )
{
  unsigned int half_dot_width = dot_width_/2;
  unsigned int l = i - half_dot_width;
  unsigned int u = j - half_dot_width;
  unsigned int r = i + half_dot_width;
  unsigned int b = j + half_dot_width;
  if(l >= img.ni())
  {
    l = 0;
  }
  if(u >= img.nj())
  {
    u = 0;
  }
  if(r >= img.ni())
  {
    r = img.ni()-1;
  }
  if(b >= img.nj())
  {
    b = img.nj()-1;
  }
  for( i = l; i <= r; ++i )
  {
    for( j = u; j <= b; ++j )
    {
      this->draw_dot( img, i, j, rgb );
    }
  }
}

} // end namespace vidtk
