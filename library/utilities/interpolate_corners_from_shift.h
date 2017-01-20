/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_interpolate_corners_from_shift_h_
#define vidtk_interpolate_corners_from_shift_h_

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>
#include <utilities/external_settings.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{

#define settings_macro( add_param, add_array, add_enumr ) \
  add_enumr( \
    algorithm, \
    (LAST_2_POINTS) (REGRESSION), \
    REGRESSION, \
    "Algorithm to use to compute corner points. Can be either last_2_points " \
    "(to use the last 2 points only) or regression (to use multiple points)." ); \
  add_param( \
    never_generate_if_seen, \
    bool, \
    true, \
    "If we ever observe any true corner points in the input metadata " \
    "never re-generate them internally and pass input metadata." ); \
  add_param( \
    remove_other_metadata, \
    bool, \
    false, \
    "If set, other possible sources of deriving corner points (FOVs, " \
    "camera parameters) will be assumed to be invalid and be removed " \
    "from the output." ); \
  add_param( \
    min_distance, \
    double, \
    0.0, \
    "Minimum (real) distance between 2 locations in the ground " \
    "plane required to consider using them for corner point regression. " \
    "Units are in whatever coordinate system the metadata is using." ); \
  add_param( \
    min_points_to_use, \
    unsigned, \
    10, \
    "Minimum number of groundplane points to use in regression operation." ); \
  add_param( \
    max_points_to_use, \
    unsigned, \
    1000, \
    "Maximum number of groundplane points to use in regression operation." ); \
  add_param( \
    refine_geometric_error, \
    bool, \
    false, \
    "Should we refine our regressed homography using a geometric error " \
    "constraint?" ); \


init_external_settings3( interpolate_corners_settings, settings_macro )


/// \brief Interpolate image corner points using stabilization results and
/// the centroid of the image in world coordinates.
///
/// This class was designed to be called in an online fashion, for every
/// new metadata packet received.
class interpolate_corners_from_shift
{
public:

  /// \brief Initialize class with default settings.
  interpolate_corners_from_shift();

  /// \brief Destructor.
  virtual ~interpolate_corners_from_shift();

  /// \biref Configure class with new settings.
  void configure( const interpolate_corners_settings& settings );

  /// \brief Process a new frame, updating the input metadata packet
  /// accordingly.
  ///
  /// \param ni Number of image columns (in pixels)
  /// \param nj Number of image rows (in pixels)
  /// \param homog Homography for the current frame to some reference frame
  /// \param ts Input timestamp for the current frame
  /// \param meta Metadata packet to be modified
  ///
  /// \return Returns true if metadata has been modified
  bool process_packet( const unsigned ni, const unsigned nj,
                       const image_to_image_homography& homog,
                       const timestamp ts,
                       video_metadata& meta );

private:

  // Internal subclass storing members so they are not exposed
  class priv;
  boost::scoped_ptr< priv > d;
};


} // end namespace vidtk


#endif // vidtk_interpolate_corners_from_shift_h_
