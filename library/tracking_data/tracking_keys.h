/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tracking_keys_h_
#define vidtk_tracking_keys_h_

#include <utilities/property_map.h>

namespace vidtk
{

/// Defines "well known" property keys used in the tracking library.
namespace tracking_keys
{

/// \brief The key used to append image object (MOD) data to the
/// states of the new tracks.
///
/// The string representation for this key is "img_objs".
///
/// The data type is <tt>std::vector<image_object_sptr></tt>, and contains
/// the image objects (MODs) associated with that state.
///
/// \sa track_state
extern property_map::key_type img_objs;

/// \brief Used to indicate that a particular image object is being
/// used in a track.
///
/// The string representation for this key is "used_in_track".
///
/// The data type is <tt>bool</tt>.  If true, then the corresponding
/// image_object is part of a track.
///
/// This is typically used by a tracker to modify the objects in a
/// buffer to prevent a track initializer from re-using an image
/// object that was used to initialize a tracker.
///
/// \sa image_object
extern property_map::key_type used_in_track;

/// \brief Used to append a pixel value buffers of tracks.
///
/// The string representation for this key is "pixel_data".
///
/// The data type is <tt>vil_image_view< PixType ></tt>.
///
/// \sa image_object
extern property_map::key_type pixel_data;

/// \brief Boarder sized used with pixel_data.
///
/// The string representation for this key is "pixel_data_buffer".
///
/// The data type is <tt>unsigned int</tt>.
///
/// This value is used to specify how many pixels have been added to
/// each side of the image chip (pixel_data element). This value can
/// be zero or the entry can be missing from the property map.
///
/// \sa image_object
extern property_map::key_type pixel_data_buffer;

/// \brief Type of track.
///
/// Stores an identifier on what type of track this is.
///
/// The data type is <tt>unsigned int</tt>.
///
/// \sa track
extern property_map::key_type track_type;

/// \brief Histogram for object
///
/// Stores the histogram used for this detection.
///
/// The data type is <tt>image_histogram<vxl_byte, bool></tt>
///
/// \sa image_object
extern property_map::key_type histogram;

/// \brief Foreground model object.
///
/// This is key is used to store the current foreground model object
/// of the track.
///
/// The data type is <tt>fg_matcher_byte_sptr</tt>, which contains an
/// image chip.
extern property_map::key_type foreground_model;
extern property_map::key_type foreground_model_pix_format;

/// \brief Type of track.
///
/// Stores mean and variance of grayscale intensity of the object.
///
/// The data type is <tt>std::pair<float, float>( mean, var )</tt>.
///
/// \sa image_object
extern property_map::key_type intensity_distribution;

/// \brief The key used to append geo-spatial position to the
/// states of the tracks.
///
/// The string representation for this key is "lat_long".
///
/// Contains geo-spatial (latitude and longitude) position of the
/// track state. The data type is <tt>std::pair<double, double></tt>,
/// and contains the latitude and longitutde (in this order) for
/// the position of detection/track state on the ground (Earth) plane.
/// If you need UTM or MGRS versions of the geographic data, then
/// one can use vidtk::geographic::geo_coords for storage and conversions.
///
/// \sa track_state
extern property_map::key_type lat_long;

/// \brief The key used to append geo-spatial position to the
/// states of the tracks.
///
/// Stores a previously calcuated UTM for this state.
///
/// UTM is calculated from the state's lat/lon which is expensive.
/// Store the result here so we only need to calculate it once.
///
/// \sa track_state
extern property_map::key_type calculated_utm;


/// \brief The key used to append Ground Sample Distance (GSD) to the
/// states of the tracks.
///
/// The strileeng representation for this key is "gsd".
///
/// Contains Ground Sample Distance (GSD) of the track state.
/// The data type is <tt>float</tt>, and units are meters/pixel.
///
/// \sa track_state
extern property_map::key_type gsd;

/// \brief Note for object
///
/// Stores textual notes used for this track.
/// Each note is terminated by a newline character.
///
extern property_map::key_type note;

/// \brief Database primary key for track
///
/// Used to uniquely identify a tracking in the db
///
/// The data type is <tt>vxl_int_64</tt>.
///
/// \sa track
extern property_map::key_type primary_key;

/// \brief Track's terminated frame_number
///
/// The frame number on which the system published
/// the track as terminated.
///
/// The data type is <tt>unsigned</tt>.
///
/// \sa track
extern property_map::key_type terminated_frame;


/// \brief Create a copy of a property map
///

/// Deeply copies all tracking keys a property map has.
void deep_copy_property_map(const property_map& src, property_map& dest);


} // end namespace tracking_keys
} // end namespace vidtk


#endif // vidtk_tracking_keys_h_

// Local Variables:
// mode: c++
// fill-column: 70
// c-tab-width: 2
// c-basic-offset: 2
// c-basic-indent: 2
// c-indent-tabs-mode: nil
// end:
