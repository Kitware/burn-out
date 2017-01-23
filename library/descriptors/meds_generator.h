/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_meds_generator_h_
#define vidtk_meds_generator_h_

#include <descriptors/online_descriptor_generator.h>

#include <utilities/config_block.h>

#include <string>

namespace vidtk
{


#define settings_macro( add_param, add_array ) \
  add_param( \
    descriptor_name, \
    std::string, \
    "TrackMEDS", \
    "Descriptor ID to output" ); \
  add_param( \
    sampling_rate, \
    unsigned, \
    1, \
    "Downsampling factor for the descriptor" ); \
  add_array( \
    person_tracker_ids, \
    unsigned, \
    0, \
    "", \
    "Tracker IDs for known person trackers" ); \
  add_array( \
    vehicle_tracker_ids, \
    unsigned, \
    0, \
    "", \
    "Tracker IDs for known vehicle trackers" ); \
  add_param( \
    buffer_length, \
    unsigned, \
    20, \
    "Buffer length" );

init_external_settings2( meds_settings, settings_macro )

#undef settings_macro

/// \brief Misc Event Detection Summary Descriptor (MEDS)
///
/// Outputs several raw descriptor properties useful for both object
/// classification and event classification. For bins [0,20], these
/// properties include:
///
/// 0. Came from person tracker?
/// 1. Came from vehicle tracker?
/// 2. Max velocity
/// 3. Max acceleration
/// 4. Current velocity
/// 5. Current acceleration
/// 6. Current bbox target area
/// 7. Current bbox aspect ratio
/// 8. Current estimated target area
/// 9. Average of bbox size
/// 10. Average of bbox ratio
/// 11. Average of target speed
/// 12. Average of velocity angle
/// 13. Average deviation of velocity magnitude
/// 14. Average deviation of veolcity angle
/// 15. Average deviation of bbox ratio
/// 16. Average deviation of bbox area
/// 17. Percent FG coverage score, if available
/// 18. Maximum recorded distance from start location
/// 19. Current track length (in time)
/// 20. Distance traveled during buffering period
class meds_generator : public online_descriptor_generator
{

public:

  /// Constructor.
  meds_generator();

  /// Destructor.
  virtual ~meds_generator() {}

  /// Set any settings for this descriptor generator.
  virtual bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  /// Called the first time we receive a new track.
  virtual track_data_sptr new_track_routine( track_sptr const& new_track );

  /// Standard update function.
  virtual bool track_update_routine( track_sptr const& active_track,
                                     track_data_sptr track_data );

  /// Handle terminated tracks.
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data );

private:

  // Settings which can be set externally by the caller
  meds_settings settings_;

};

} // end namespace vidtk

#endif // meds_generator_h_
