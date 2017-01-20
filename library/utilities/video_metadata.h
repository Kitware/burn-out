/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_metadata_h_
#define vidtk_video_metadata_h_

#include <utilities/geo_lat_lon.h>
#include <utilities/homography.h>

#include <ostream>
#include <istream>
#include <vector>
#include <cmath>

#include <vxl_config.h>

namespace vidtk
{

// ----------------------------------------------------------------
/** Tracker metadata.
 *
 * This class represents the base class for tracker metadata.
 *
 * The basic information is the image timestamp, platform location,
 * platform orientation, and the sensor orientation. These are
 * considered the inputs.
 *
 */
class video_metadata
{
public:
  // -- TYPES --
  typedef std::vector < video_metadata > vector_t;
  typedef image_to_plane_homography homography_t;

  // -- CONSTRUCTORS --
  video_metadata();
  virtual ~video_metadata();


  // -- ACCESSORS --
  vxl_uint_64 timeUTC() const { return m_timeUTC; }
  const geo_coord::geo_lat_lon& platform_location() const { return ( m_platformLatLon ); }

  double platform_altitude() const { return ( m_platformAltitude );  }
  double platform_roll() const { return ( m_platformRoll ); }
  double platform_pitch() const { return ( m_platformPitch ); }
  double platform_yaw() const { return ( m_platformYaw ); }

  double sensor_roll() const { return ( m_sensorRoll ); }
  double sensor_pitch() const { return ( m_sensorPitch ); }
  double sensor_yaw() const { return ( m_sensorYaw ); }

  const geo_coord::geo_lat_lon& corner_ul() const { return ( m_corner_ul ); }
  const geo_coord::geo_lat_lon& corner_ur() const { return ( m_corner_ur ); }
  const geo_coord::geo_lat_lon& corner_lr() const { return ( m_corner_lr ); }
  const geo_coord::geo_lat_lon& corner_ll() const { return ( m_corner_ll ); }

  double slant_range() const { return ( m_slantRange ); }
  double sensor_horiz_fov() const { return ( m_sensorHorizFOV ); }
  double sensor_vert_fov() const { return ( m_sensorVertFOV ); }

  const geo_coord::geo_lat_lon& frame_center() const { return ( m_frameCenter ); }

  double horizontal_gsd() const { return ( m_horizontal_gsd ); }
  double vertical_gsd() const { return ( m_vertical_gsd ); }
  homography_t const &img_to_wld_homog() const { return ( m_img_to_wld_homog ); }
  std::string const &camera_mode() const { return ( m_camera_mode ); }

  bool has_valid_slant_range() const;
  bool has_valid_time() const;
  bool has_valid_corners() const ;
  bool has_valid_frame_center() const;
  bool has_valid_gsd() const;
  bool has_valid_horizontal_gsd() const;
  bool has_valid_vertical_gsd() const;
  bool has_valid_img_to_wld_homog() const;
  bool has_valid_camera_mode() const;
  bool is_valid() const;

  virtual std::ostream & to_stream (std::ostream& str) const;

  // -- MANIPULATORS --
  video_metadata& timeUTC(const vxl_uint_64& t) { m_timeUTC = t; return ( *this ); }

  // Platform location
  video_metadata& platform_location(const geo_coord::geo_lat_lon& loc) { m_platformLatLon = loc; return ( *this ); }
  video_metadata& platform_altitude(double v) { m_platformAltitude = v; return ( *this ); }

  // Platform orientation
  video_metadata& platform_roll(double v) { m_platformRoll = v; return ( *this ); }
  video_metadata& platform_pitch(double v) { m_platformPitch = v; return ( *this ); }
  video_metadata& platform_yaw(double v) { m_platformYaw = v; return ( *this ); }

  // Sensor orientation WRT platform
  video_metadata& sensor_roll(double v) { m_sensorRoll = v; return ( *this ); }
  video_metadata& sensor_pitch(double v) { m_sensorPitch = v; return ( *this ); }
  video_metadata& sensor_yaw(double v) { m_sensorYaw = v; return ( *this ); }

  // Image corner points.
  video_metadata& corner_ul(const geo_coord::geo_lat_lon& v) { m_corner_ul = v; return ( *this ); }
  video_metadata& corner_ur(const geo_coord::geo_lat_lon& v) { m_corner_ur = v; return ( *this ); }
  video_metadata& corner_lr(const geo_coord::geo_lat_lon& v) { m_corner_lr = v; return ( *this ); }
  video_metadata& corner_ll(const geo_coord::geo_lat_lon& v) { m_corner_ll = v; return ( *this ); }

  video_metadata& slant_range(double v) { m_slantRange = v; return (*this); }
  video_metadata& sensor_horiz_fov(double v) { m_sensorHorizFOV = v; return (*this); }
  video_metadata& sensor_vert_fov(double v) { m_sensorVertFOV = v; return (*this); }

  video_metadata& frame_center(const geo_coord::geo_lat_lon v) { m_frameCenter = v; return ( *this ); }

  video_metadata& horizontal_gsd(const double v) { m_horizontal_gsd = v; return ( *this ); }
  video_metadata& vertical_gsd(const double v) { m_vertical_gsd = v; return ( *this ); }
  video_metadata& img_to_wld_homog(const homography_t& v) { m_img_to_wld_homog = v; return ( *this ); }
  video_metadata& camera_mode(const std::string& s) { m_camera_mode = s; return ( *this ); }

  video_metadata& is_valid( bool in );

  bool operator == ( const video_metadata &rhs ) const;
  bool operator != ( const video_metadata &rhs )  const { return ( !( this->operator == ( rhs ) ) ); }

protected:


private:
  // Time from the metadata
  vxl_uint_64 m_timeUTC;

  // platform position (in degrees)
  geo_coord::geo_lat_lon m_platformLatLon;
  double m_platformAltitude; // meters ASL??

  // platform orientation (RPY) (in degrees)
  // Using a right handed coordinate system. Platform axis is along the X axis.
  // The Z axis is down, making the Y axis point to the right.
  double m_platformRoll;      // About the X axis. Positive roll is to the right
  double m_platformPitch;     // About the Y axis. Positive pitch is up.
  double m_platformYaw;       // About the Z axis. Positive yas is to the Right.

  // sensor orientation WRT the platform (in degrees)
  double m_sensorRoll;        // Relative to the platform
  double m_sensorPitch;       // Relative to the platform - Elevation
  double m_sensorYaw;         // Relative to the platform - Azimuth

  // Corner points of the associated image.
  // Corners start in upper-left and go clockwise.
  geo_coord::geo_lat_lon m_corner_ul;
  geo_coord::geo_lat_lon m_corner_ur;
  geo_coord::geo_lat_lon m_corner_lr;
  geo_coord::geo_lat_lon m_corner_ll;

  geo_coord::geo_lat_lon m_frameCenter;

  // Field of view and slant angle parameters for the camera.
  double m_slantRange;
  double m_sensorHorizFOV;
  double m_sensorVertFOV;

  // External device recommended ground sample distances (GSDs).
  double m_horizontal_gsd;
  double m_vertical_gsd;

  // External device recommended image to world homographies.
  homography_t m_img_to_wld_homog;

  // String indicating the camera mode (typically the zoom level).
  std::string m_camera_mode;

  bool m_is_valid;

}; // end class video_metadata

inline std::ostream & operator<< (std::ostream & str, const vidtk::video_metadata & obj)
{ return obj.to_stream (str); }

} // end namespace

#endif /* _TRACKERMETADATA_H_ */
