/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <utilities/video_metadata.h>

#include <iomanip>
#include <string>
#include <iostream>


namespace vidtk
{

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
video_metadata
::video_metadata()
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

    m_horizontal_gsd(0),
    m_vertical_gsd(0),

    m_is_valid(true)
{
}


video_metadata
::~video_metadata()
{
}


bool
video_metadata
::has_valid_corners() const
{
  // First, all corners must be valid.
  if ( ! m_corner_ul.is_valid() ) { return false; }
  if ( ! m_corner_ur.is_valid() ) { return false; }
  if ( ! m_corner_lr.is_valid() ) { return false; }
  if ( ! m_corner_ll.is_valid() ) { return false; }

  // Second, no two corners can be the same
  if ( m_corner_ul == m_corner_ur ) { return false; }
  if ( m_corner_ul == m_corner_lr ) { return false; }
  if ( m_corner_ul == m_corner_ll ) { return false; }

  if ( m_corner_ur == m_corner_lr ) { return false; }
  if ( m_corner_ur == m_corner_ll ) { return false; }

  if ( m_corner_lr == m_corner_ll ) { return false; }

  return true; // finally - valid corners
}


bool
video_metadata
::has_valid_frame_center() const
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
std::ostream&
video_metadata
::to_stream (std::ostream & str) const
{
  std::streamsize old_prec = str.precision();
  str.precision(22);

  str << "video_metadata @ " << this << "  (metadata "
      << ( is_valid() ? ""  : "not") << " valid)"
      << std::endl

      << "TimeUTC: " << m_timeUTC << std::endl
      << "Platform location: " << m_platformLatLon << "  at: " << m_platformAltitude << " (ASL)"
      << std::endl

      << "Platform [R:" << m_platformRoll << " P:" << m_platformPitch << " W:" << m_platformYaw << "]"
      << std::endl

      << "Sensor WRT platform [R:" << m_sensorRoll << " P:" << m_sensorPitch << " W:" << m_sensorYaw << "]"
      << std::endl

      << "Corners: (ul) " << m_corner_ul
      << "  (ur) " << m_corner_ur
      << "  (lr) " << m_corner_lr
      << "  (ll) " << m_corner_ll
      << std::endl

      << "Slant range: " << m_slantRange << "   horiz FOV: " << m_sensorHorizFOV
      << "   vert FOV: " << m_sensorVertFOV
      << std::endl

      << "Frame center: " << m_frameCenter
      << std::endl
    ;

  str.precision( old_prec );

  return str;
}


bool
video_metadata
::is_valid() const
{
  return m_is_valid;
}


bool
video_metadata
::has_valid_slant_range() const
{
  return m_slantRange != 0.0;
}


video_metadata&
video_metadata
::is_valid( bool in )
{
  m_is_valid = in;
  return *this;
}


bool
video_metadata
::has_valid_time() const
{
  return m_timeUTC != 0;
}


bool
video_metadata
::has_valid_gsd() const
{
  return ( has_valid_horizontal_gsd() || has_valid_vertical_gsd() );
}


bool
video_metadata
::has_valid_horizontal_gsd() const
{
  return ( m_horizontal_gsd > 0 );
}


bool
video_metadata
::has_valid_vertical_gsd() const
{
  return ( m_vertical_gsd > 0 );
}


bool
video_metadata
::has_valid_img_to_wld_homog() const
{
  return m_img_to_wld_homog.is_valid();
}


bool
video_metadata
::has_valid_camera_mode() const
{
  return !m_camera_mode.empty();
}


// ----------------------------------------------------------------
/** Equality operator.
 *
 * Two metadata objects are the same of all individual components are
 * equal.  With the exception of both objects are invalid, then they
 * are equal regardless of the value of the other fields.
 */
bool
video_metadata
::operator == ( const video_metadata &rhs ) const
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

  TEST_ONE_FIELD ( frame_center() );

#undef TEST_ONE_FIELD

  // Finally !!!
  return true;
}


} // end namespace
