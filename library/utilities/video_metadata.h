/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_VIDEO_METADATA_H_
#define _VIDTK_VIDEO_METADATA_H_

#include <utilities/timestamp.h>
#include <vcl_ostream.h>
#include <vcl_istream.h>
#include <vcl_vector.h>
#include <vcl_cmath.h>
#include <vxl_config.h>


namespace vidtk
{

// ----------------------------------------------------------------
/** Tracker metadata.
 *
 * This class represents the base class for tracker metadata.
 *
 * The basic information is the image timestampp, platform location,
 * platform orientation, and the sensor orientation. These are
 * considered the inputs.
 *
 */
class video_metadata
{
public:
  // -- TYPES --
  typedef vcl_vector < video_metadata > vector_t;

// ----------------------------------------------------------------
/** Geo-coordinate as latitude / longitude.
 *
 * This class represents a Latitude / longitude geolocated point.
 *
 * The values for latitude and longitude are in degrees, but there is
 * no required convention for longitude. It can be either 0..360 or
 * -180 .. 0 .. 180. It is up to the application.  If a specific
 * convention must be enforced, make a subclass.
 */
  class lat_lon_t
  {
  public:
    static const double INVALID;  // used to indicate uninitialized value

    lat_lon_t() : m_lat(INVALID), m_lon(INVALID) { }
    lat_lon_t(double lat, double lon) : m_lat(lat), m_lon(lon) { }

    lat_lon_t& lat(double l) { m_lat = l; return ( *this ); }
    lat_lon_t& lon(double l) { m_lon = l; return ( *this ); }
    double lat() const { return ( m_lat ); }
    double lon() const { return ( m_lon ); }

    bool is_valid() const { return ((vcl_fabs(m_lat) <= 90.0) && (vcl_fabs(m_lon) <= 180.0)); }

    bool operator == ( const lat_lon_t &rhs ) const
    { return ( ( rhs.lat() == this->lat() ) && ( rhs.lon() == this->lon() ) ); }
    bool operator != ( const lat_lon_t &rhs ) const { return ( !( this->operator == ( rhs ) ) ); }
    lat_lon_t operator+ (const lat_lon_t & rhs) const;

    vcl_ostream & ascii_serialize(vcl_ostream& str) const;
    vcl_istream & ascii_deserialize(vcl_istream& str);

  private:
    double m_lat;
    double m_lon;
  }; // -- end class lat_lon_t --



  // -- CONSTRUCTORS --
  video_metadata();
  virtual ~video_metadata();


  // -- ACCESSORS --
  vxl_uint_64 timeUTC() const { return m_timeUTC; }
  const lat_lon_t& platform_location() const { return ( m_platformLatLon ); }

  double platform_altitude() const { return ( m_platformAltitude );  }
  double platform_roll() const { return ( m_platformRoll ); }
  double platform_pitch() const { return ( m_platformPitch ); }
  double platform_yaw() const { return ( m_platformYaw ); }

  double sensor_roll() const { return ( m_sensorRoll ); }
  double sensor_pitch() const { return ( m_sensorPitch ); }
  double sensor_yaw() const { return ( m_sensorYaw ); }

  const lat_lon_t& corner_ul() const { return ( m_corner_ul ); }
  const lat_lon_t& corner_ur() const { return ( m_corner_ur ); }
  const lat_lon_t& corner_lr() const { return ( m_corner_lr ); }
  const lat_lon_t& corner_ll() const { return ( m_corner_ll ); }

  double slant_range() const { return (m_slantRange); }
  double sensor_horiz_fov() const { return (m_sensorHorizFOV); }
  double sensor_vert_fov() const { return (m_sensorVertFOV); }

  const lat_lon_t &  frame_center() const { return ( m_frameCenter ); }

  bool has_valid_time() const;
  bool has_valid_corners() const ;
  bool has_valid_frame_center() const;
  bool is_valid() const;

  virtual vcl_ostream & to_stream (vcl_ostream& str) const;
  vcl_ostream & ascii_serialize(vcl_ostream& str) const;
  vcl_istream & ascii_deserialize(vcl_istream& str);


  // -- MANIPULATORS --
  video_metadata& timeUTC(const vxl_uint_64& t) { m_timeUTC = t; return ( *this ); }

  // Platform location
  video_metadata& platform_location(const lat_lon_t& loc) { m_platformLatLon = loc; return ( *this ); }
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
  video_metadata& corner_ul(const lat_lon_t& v) { m_corner_ul = v; return ( *this ); }
  video_metadata& corner_ur(const lat_lon_t& v) { m_corner_ur = v; return ( *this ); }
  video_metadata& corner_lr(const lat_lon_t& v) { m_corner_lr = v; return ( *this ); }
  video_metadata& corner_ll(const lat_lon_t& v) { m_corner_ll = v; return ( *this ); }

  video_metadata& slant_range(double v) { m_slantRange = v; return (*this); }
  video_metadata& sensor_horiz_fov(double v) { m_sensorHorizFOV = v; return (*this); }
  video_metadata& sensor_vert_fov(double v) { m_sensorVertFOV = v; return (*this); }

  video_metadata & frame_center(const lat_lon_t v) { m_frameCenter = v; return ( *this ); }

  video_metadata& is_valid( bool in );


  bool operator == ( const video_metadata &rhs ) const;
  bool operator != ( const video_metadata &rhs )  const { return ( !( this->operator == ( rhs ) ) ); }

protected:


private:
  // Time from the metadata
  vxl_uint_64 m_timeUTC;

  // platform position (in degrees)
  lat_lon_t m_platformLatLon;
  double m_platformAltitude; // meters ASL??

  // platform orientation (RPY) (in degrees)
  // Using a right handed coordinate system. Platform axis is along the X axis.
  // The Z axis is down, making the Y axis point to the right.
  double m_platformRoll;    // About the X axis. Positive roll is to the right
  double m_platformPitch;   // About the Y axis. Positive pitch is up.
  double m_platformYaw;     // About the Z axis. Positive yas is to the Right.

  // sensor orientation WRT the platform (in degrees)
  double m_sensorRoll;          // Relative to the platform
  double m_sensorPitch;         // Relative to the platform - Elevation
  double m_sensorYaw;           // Relative to the platform - Azimuth

  // Corner points of the associated image.
  // Corners start in upper-left and go clockwise.
  lat_lon_t m_corner_ul;
  lat_lon_t m_corner_ur;
  lat_lon_t m_corner_lr;
  lat_lon_t m_corner_ll;

  lat_lon_t m_frameCenter;

  double m_slantRange;
  double m_sensorHorizFOV;
  double m_sensorVertFOV;

  bool m_is_valid;

}; // end class video_metadata



inline vcl_ostream & operator<< (vcl_ostream & str, const vidtk::video_metadata::lat_lon_t & obj)
{ str << "[ " << obj.lat() << " / " << obj.lon() << " ]"; return (str); }

inline vcl_ostream & operator<< (vcl_ostream & str, const vidtk::video_metadata & obj)
{ return obj.to_stream (str); }

} // end namespace

#endif /* _TRACKERMETADATA_H_ */
