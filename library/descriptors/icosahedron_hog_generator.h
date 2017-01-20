/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_icosahedron_hog_generator_h_
#define vidtk_icosahedron_hog_generator_h_

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/icosahedron_hog_descriptor.h>

#include <utilities/config_block.h>

#include <list>
#include <queue>
#include <vector>

#include <boost/circular_buffer.hpp>

namespace vidtk
{


#define settings_macro( add_param ) \
  add_param( \
    descriptor_name, \
    std::string, \
    "icosahedron_hog", \
    "Descriptor ID to output" ); \
  add_param( \
    num_threads, \
    unsigned int, \
    1, \
    "Number of threads to use" ); \
  add_param( \
    descriptor_length, \
    unsigned int, \
    20, \
    "Each descriptor is computed over this many frames" ); \
  add_param( \
    descriptor_spacing, \
    unsigned int, \
    10, \
    "Have a new descriptor start every n frames" ); \
  add_param( \
    descriptor_skip_count, \
    unsigned int, \
    1, \
    "When running the descriptor, only sample 1 in n frames" ); \
  add_param( \
    width_cells, \
    unsigned int, \
    3, \
    "Have n cells per the width of the BB around each track" ); \
  add_param( \
    height_cells, \
    unsigned int, \
    3, \
    "Have n cells per the height of the BB around each track" ); \
  add_param( \
    time_cells, \
    unsigned int, \
    3, \
    "Divide the time period over which this descriptor is formulated by n" ); \
  add_param( \
    bb_scale_factor, \
    double, \
    1.0, \
    "Percent [1,inf] to scale (increase) detected track BB by" ); \
  add_param( \
    bb_pixel_shift, \
    unsigned int, \
    3, \
    "Number of pixels to shift (expand) the detected track BB by" ); \
  add_param( \
    variable_smoothing, \
    bool, \
    true, \
    "Enable variable bin smoothing" ); \
  add_param( \
    min_region_area, \
    unsigned int, \
    1000, \
    "Minimum area (in pixels squared) required for HoG computation" );

init_external_settings( icosahedron_hog_settings, settings_macro )

#undef settings_macro

/// A generator for computing test hog descriptors in an online manner
template <typename PixType = vxl_byte>
class icosahedron_hog_generator : public online_descriptor_generator
{

public:

  icosahedron_hog_generator() {}
  ~icosahedron_hog_generator() {}

  bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  // Any data we want stored for each individual track we encounter
  struct hog_track_data : track_data_storage
  {
    // # of frames observed by this descriptor class
    unsigned int frames_observed;

    // Frame ID of start frame for this track
    unsigned int start_frame_id;

    // Recorded cropped image history for this track
    boost::circular_buffer< vil_image_view<float> > cropped_image_history;

    // Recorded BBs/Frame IDs corresponding to above grayscale images
    boost::circular_buffer< descriptor_history_entry > region_history;
  };

  // Used to set and initialize any track data we might want to store for the tracks
  // duration, this data will be provided via future step function calls
  virtual track_data_sptr new_track_routine( track_sptr const& new_track );

  // Standard update function called for all active tracks on the current
  // frame.
  virtual bool track_update_routine( track_sptr const& active_track,
                                     track_data_sptr track_data );

  // Called everytime a track is terminated, in case we want to output
  // any partially complete descriptors
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data );

private:

  // Settings which can be set externally by the caller
  icosahedron_hog_settings settings_;

};


} // end namespace vidtk

#endif // vidtk_icosahedron_hog_generator_h_
