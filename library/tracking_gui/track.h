/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_vgui_track_h_
#define vidtk_vgui_track_h_

#include <vil/vil_rgba.h>
#include <vgui/vgui_easy2D_tableau.h>
#include <vgui/vgui_text_tableau.h>
#include <tracking/track.h>

namespace vidtk
{
namespace vgui
{


struct draw_track
{
  draw_track();

  draw_track( vgui_easy2D_tableau_sptr easy );

  void init( vgui_easy2D_tableau_sptr easy );

  /// Draw the track on the tableau.
  int draw( vidtk::track const& trk );
  
  /// Draw the track up to a frame number.
  int draw_upto( vidtk::track const& trk, unsigned int end_frame );

  /// Draw the track on the tableau.
  ///
  /// This overload takes an optional world-to-image transformation that should
  /// be used to draw the trail, and an optional frame range to restrict
  /// drawing of the trail to a certain time period.
  int draw( vidtk::track const& trk,
             vnl_matrix_fixed<double,3,3> const* world_to_image,
             unsigned int end_frame = 0xFFFFFFFF,
             unsigned int start_frame = 0 );

  /// Draw the (x,y) part of the track on the tableau in world coordinates.
  int draw_world2d( vidtk::track const& trk );

  /// Draw a n-sigma gate centered at loc.
  ///
  /// If \a proj is provided, the ellipse will be projected by this
  /// matrix.  Typically proj is the inverse of the camera matrix,
  /// transferring world coordinates to camera coordinates.
  void draw_gate( vnl_vector_fixed<double,2> const& loc,
                  vnl_matrix_fixed<double,2,2> const& cov,
                  double sigma,
                  vnl_matrix_fixed<double,3,3>* proj = 0 );


  /// Reset the parameters back to the defaults.
  void set_to_default();

  vgui_easy2D_tableau_sptr easy_;
  vgui_text_tableau_sptr text_;
  double cutoff_;

  bool draw_point_at_end_;
  bool draw_trail_;
  bool draw_obj_boundary_;
  bool draw_obj_;

  vil_rgba<float> trail_color_;
  vil_rgba<float> current_color_;
  vil_rgba<float> past_color_;
};


} // end namespace vgui
} // end namespace vidtk


#endif // vidtk_vgui_track_h_
