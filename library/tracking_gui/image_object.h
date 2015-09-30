/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vgui_image_object_h_
#define vidtk_vgui_image_object_h_

#include <vgui/vgui_easy2D_tableau.h>
#include <vgui/vgui_text_tableau.h>
#include <tracking/image_object.h>

namespace vidtk
{
namespace vgui
{


struct draw_image_object
{
  draw_image_object();

  draw_image_object( vgui_easy2D_tableau_sptr easy );

  void init( vgui_easy2D_tableau_sptr easy );

  /// Draw the object on the tableau.
  void draw( vidtk::image_object const& obj );

  /// Draw the (x,y) component of the world location on the tableau.
  void draw_world2d( vidtk::image_object const& obj );

  /// Reset the parameters back to the defaults.
  void set_to_default();

  vgui_easy2D_tableau_sptr easy_;
  vgui_text_tableau_sptr text_;

  // If the r component of a colour is < 0, don't set a colour when
  // drawing that component.
  vil_rgba<float> loc_color_;
  vil_rgba<float> bbox_color_;
  vil_rgba<float> boundary_color_;

  bool show_area_;
  bool show_boundary_;
};


} // end namespace vgui
} // end namespace vidtk


#endif // vidtk_vgui_image_object_h_
