/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pvo_track.h"

#include <tracking/pvo_probability.h>

namespace vidtk
{
namespace vgui
{

draw_pvo_track
::draw_pvo_track()
  : draw_track()
{
  set_to_default_pvo();
}

draw_pvo_track
::draw_pvo_track( vgui_easy2D_tableau_sptr easy )
  : draw_track( easy )
{
  set_to_default_pvo();
}

void
draw_pvo_track
::draw_pvo( vidtk::track const& trk,
            vnl_matrix_fixed<double,3,3> const* world_to_image,
            unsigned int end_frame,
            unsigned int start_frame )
{
  pvo_probability const& pvo = trk.get_pvo();
  const double person = pvo.get_probability_person();
  const double vehicle = pvo.get_probability_vehicle();
  const double other = pvo.get_probability_other();

  if( vehicle > person && vehicle > other )
  {
    trail_color_ = vehicle_color_;
  }
  else if( person > vehicle && person > other )
  {
    trail_color_ = person_color_;
  }
  else if( vehicle == person )
  {
    trail_color_ = unknown_color_;
  }
  else if( other > person && other > vehicle )
  {
    trail_color_ = other_color_;
  }

  draw_track::draw( trk, world_to_image, end_frame, start_frame );
}

void
draw_pvo_track
::set_to_default_pvo()
{
  person_color_ = vil_rgba<float>( 0, 0, 1 );
  vehicle_color_ = vil_rgba<float>( 0, 1, 0 );
  other_color_ = vil_rgba<float>( 1, 0, 0 );
  unknown_color_ = vil_rgba<float>( 1, 1, 0 );
  draw_track::set_to_default();
}

} // end namespace vgui
} // end namespace vidtk
