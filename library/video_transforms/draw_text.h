/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_draw_text_h
#define vidtk_draw_text_h

/// \file
/// A derived clase with contents to for drawing text on an image.

#include <video_transforms/draw_data.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

template< class PixType >
class draw_text
 : public draw_data< PixType >
{
public:
  draw_text();
  ~draw_text();

  void draw_message( vil_image_view< PixType >& img,
                     std::string const& msg,
                     unsigned starti, unsigned startj,
                     unsigned int rgb[] );

}; // draw_text

} // end namespace vidtk

#endif
