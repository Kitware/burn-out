/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vgui_pvo_track_h_
#define vidtk_vgui_pvo_track_h_

#include <tracking_gui/track.h>

namespace vidtk
{
namespace vgui
{


struct draw_pvo_track : public draw_track
{
  draw_pvo_track();

  draw_pvo_track( vgui_easy2D_tableau_sptr easy );

  /// Draw the track on the tableau.
  ///
  /// This overload takes an optional world-to-image transformation that should
  /// be used to draw the trail, and an optional frame range to restrict
  /// drawing of the trail to a certain time period.
  void draw_pvo( vidtk::track const& trk,
                 vnl_matrix_fixed<double,3,3> const* world_to_image = 0,
                 unsigned int end_frame = 0xFFFFFFFF,
                 unsigned int start_frame = 0 );

  void set_to_default_pvo();

  vil_rgba<float> person_color_;
  vil_rgba<float> vehicle_color_;
  vil_rgba<float> other_color_;
  vil_rgba<float> unknown_color_;
};


} // end namespace vgui
} // end namespace vidtk


#endif // vidtk_vgui_pvo_track_h_
