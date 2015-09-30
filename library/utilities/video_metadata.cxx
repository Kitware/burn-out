/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <utilities/video_metadata.h>
#include <vcl_iomanip.h>


namespace vidtk
{


const double video_metadata::lat_lon_t::INVALID = 444.;


// ----------------------------------------------------------------
/** Add two points.
 *
 * This method implements the offset operator.  This facilitates
 * resolving offset coordinates.
 */
  video_metadata::lat_lon_t video_metadata::lat_lon_t::
  operator+ ( const video_metadata::lat_lon_t &rhs ) const
{
  lat_lon_t res;

  res.lat( this->lat() + rhs.lat() );
  res.lon( this->lon() + rhs.lon() );

  return res;
}

vcl_ostream &
video_metadata::lat_lon_t::
ascii_serialize(vcl_ostream& str) const
{
  str << m_lat << " "
      << m_lon;
  return str;
}

vcl_istream &
video_metadata::lat_lon_t::
ascii_deserialize(vcl_istream& str)
{
  str >> m_lat
      >> m_lon;
  return str;
}

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
video_metadata::video_metadata()
  : m_timeUTC(),
    m_platformLatLon(),
    m_platformAltitude(0),
    m_platformRoll(0),
    m_platformPitch(0),
    m_platformYaw(0),

    m_sensorRoll(0),
    m_sensorPitch(0),
    m_sensorYaw(0),

    m_corner_ul(),
    m_corner_ur(),
    m_corner_lr(),
    m_corner_ll(),

    m_frameCenter(),

    m_slantRange(0),
    m_sensorHorizFOV(0),
    m_sensorVertFOV(0),

    m_is_valid(true)
{

}


video_metadata::~video_metadata()
{

}


bool video_metadata::has_valid_corners() const
{
  return m_corner_ul.is_valid() && m_corner_ur.is_valid()
      && m_corner_lr.is_valid() && m_corner_ll.is_valid();
}


bool video_metadata::has_valid_frame_center() const
{
  return m_frameCenter.is_valid();
}


// -----------------------------------------------------------------
/** Format this object on a stream.
 *
 * This method formats this object on the specified stream.
 *
 * @param[in] str - output stream
 * @return The input stream is returned.
 */
vcl_ostream & video_metadata::
to_stream (vcl_ostream & str) const
{
  str << "video_metadata @ " << this << "  (metadata "
      << ( is_valid() ? ""  : "not") << " valid)"
      << vcl_endl

      << "TimeUTC: " << m_timeUTC << vcl_endl
      << "Platform location: " << m_platformLatLon << "  at: " << m_platformAltitude << " (ASL)"
      << vcl_endl

      << "Platform [R:" << m_platformRoll << " P:" << m_platformPitch << " W:" << m_platformYaw << "]"
      << vcl_endl

      << "Sensor WRT platform [R:" << m_sensorRoll << " P:" << m_sensorPitch << " W:" << m_sensorYaw << "]"
      << vcl_endl

      << "Corners: (ul) " << m_corner_ul
      << "  (ur) " << m_corner_ur
      << "  (lr) " << m_corner_lr
      << "  (ll) " << m_corner_ll
      << vcl_endl

      << "Slant range: " << m_slantRange << "   horiz FOV: " << m_sensorHorizFOV
      << "   vert FOV: " << m_sensorVertFOV
      << vcl_endl

      << "Frame center: " << m_frameCenter
      << vcl_endl

    ;

    return str;
}

vcl_ostream&
video_metadata::
ascii_serialize(vcl_ostream& str) const
{
  str << vcl_setprecision(20);
  str << m_timeUTC << " ";
  m_platformLatLon.ascii_serialize(str) << " ";
  str << m_platformAltitude << " "
      << m_platformRoll << " "
      << m_platformPitch << " "
      << m_platformYaw << " "
      << m_sensorRoll << " "
      << m_sensorPitch << " "
      << m_sensorYaw << " ";
  m_corner_ul.ascii_serialize(str) << " ";
  m_corner_ur.ascii_serialize(str) << " ";
  m_corner_lr.ascii_serialize(str) << " ";
  m_corner_ll.ascii_serialize(str) << " ";
  str << m_slantRange << " "
      << m_sensorHorizFOV << " "
      << m_sensorVertFOV << " ";
  m_frameCenter.ascii_serialize(str);

  return str;
}


vcl_istream&
video_metadata::
ascii_deserialize(vcl_istream& str)
{
  str >> m_timeUTC;
  m_platformLatLon.ascii_deserialize(str);
  str >> m_platformAltitude
      >> m_platformRoll
      >> m_platformPitch
      >> m_platformYaw
      >> m_sensorRoll
      >> m_sensorPitch
      >> m_sensorYaw;
  m_corner_ul.ascii_deserialize(str);
  m_corner_ur.ascii_deserialize(str);
  m_corner_lr.ascii_deserialize(str);
  m_corner_ll.ascii_deserialize(str);
  str >> m_slantRange
      >> m_sensorHorizFOV
      >> m_sensorVertFOV;
  m_frameCenter.ascii_deserialize(str);

  return str;
}


bool video_metadata::
is_valid() const
{
  return m_is_valid;
}


video_metadata& video_metadata::is_valid( bool in )
{
  m_is_valid = in;
  return *this;
}


bool video_metadata::
has_valid_time() const
{
  return m_timeUTC != 0;
}


// ----------------------------------------------------------------
/** Equality operator.
 *
 * Two metadata objects are the same of all individual components are
 * equal.  With the exception of both objects are invalid, then they
 * are equal regardless of the value of the other fields.
 */
bool video_metadata::
operator == ( const video_metadata &rhs ) const
{

#define TEST_ONE_FIELD(F)  if (this->F != rhs.F) return false

  TEST_ONE_FIELD ( is_valid() );

  // If they have the same valid state and one is invalid, then both
  // are invalid. Treat as equal.
  if (! this->is_valid() || ! rhs.is_valid()) return true;

  TEST_ONE_FIELD ( timeUTC() );
  TEST_ONE_FIELD ( platform_location() );
  TEST_ONE_FIELD ( platform_altitude() );

  TEST_ONE_FIELD ( platform_roll() );
  TEST_ONE_FIELD ( platform_pitch() );
  TEST_ONE_FIELD ( platform_yaw() );

  TEST_ONE_FIELD ( sensor_roll() );
  TEST_ONE_FIELD ( sensor_pitch() );
  TEST_ONE_FIELD ( sensor_yaw() );

  TEST_ONE_FIELD ( corner_ul() );
  TEST_ONE_FIELD ( corner_ur() );
  TEST_ONE_FIELD ( corner_ll() );
  TEST_ONE_FIELD ( corner_lr() );

  TEST_ONE_FIELD ( slant_range() );
  TEST_ONE_FIELD ( sensor_horiz_fov() );
  TEST_ONE_FIELD ( sensor_vert_fov() );

#undef TEST_ONE_FIELD

  // Finally !!!
  return true;
}


} // end namespace
