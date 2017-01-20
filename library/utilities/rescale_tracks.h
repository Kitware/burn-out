/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_rescale_tracks_h_
#define vidtk_rescale_tracks_h_

#include <tracking_data/track.h>

#include <utilities/external_settings.h>

namespace vidtk
{

#define settings_macro( add_param ) \
  add_param( \
    scale_factor, \
    double, \
    1.0, \
    "Scale factor to scale track image coordinates and bounding boxes by" ); \
  add_param( \
    resize_chips, \
    bool, \
    false, \
    "Should we resize the image chips stored in the track?" ); \
  add_param( \
    deep_copy, \
    bool, \
    true, \
    "Should the source tracks be deep copied?" );

init_external_settings( rescale_tracks_settings, settings_macro )

#undef settings_macro


/// \brief Upscale the image coordinates stored in some track vector.
///
/// This class is useful for upscaling the image coordinates stored in a
/// track vector. It may come into play when we have downsampled some source
/// imagery, run it through a tracker, then want to upscale the tracks
/// to the scale of the original source imagery. Output tracks will be deep
/// copied if the process is not disabled, otherwise inputs will be passed.
void rescale_tracks( const track::vector_t& input_tracks,
                     const rescale_tracks_settings& parameters,
                     track::vector_t& output_tracks );


} // end namespace vidtk


#endif // vidtk_rescale_tracks_process_h_
