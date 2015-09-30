/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_draw_data_h
#define vidtk_draw_data_h

/// \file A base clase implementation of the contents to be drawn on image.
///
/// This class is started by forking off of tools/drawTracksOnVideo.cxx to 
/// start moving the implementations into a more generalized set of utility 
/// functions that can be used to draw text, boxes, lines, etc.

#include <vil/vil_image_view.h>

namespace vidtk 
{

template< class PixType >
class draw_data
{
public:
  draw_data( unsigned int dot_width = 1 );
  ~draw_data();

protected:
  void draw_dot( vil_image_view< PixType >& img,
                 unsigned i, unsigned j,
                 unsigned int rgb[] ); 

  void draw_variable_size_dot( vil_image_view< PixType >& img,
                               unsigned i, unsigned j,
                               unsigned int rgb[] ); 
private:
  unsigned int dot_width_;
};

} // end namespace vidtk

#endif
