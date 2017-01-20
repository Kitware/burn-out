/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_KLV_0104_H_
#define VIDTK_KLV_0104_H_

#include <klv/klv_key.h>
#include <vxl_config.h>
#include <vector>
#include <string>
#include <cstddef>
#include <map>
#include <exception>

#include <boost/any.hpp>


namespace vidtk
{

class klv_0104
{
public:

  static klv_uds_key key();

  /// Test to see if a 0104 key
  static bool is_key( klv_uds_key const& key);

  static klv_0104 *inst();

  enum tag {  PLATFORM_DESIGNATION = 0,
              PLATFORM_DESIGNATION_ALT,
              STREAM_ID,
              ITEM_DESIGNATOR_ID,
              CLASSIFICATION,
              SECURITY_CLASSIFICATION,
              IMAGE_SOURCE_SENSOR,
              SENSOR_HORIZONTAL_FOV,
              SENSOR_VERTICAL_FOV,
              SENSOR_TYPE,
              IMAGE_COORDINATE_SYSTEM,
              TARGET_WIDTH,
              PLATFORM_HEADING,
              PLATFORM_PITCH_ANGLE,
              PLATFORM_ROLL_ANGLE,
              SENSOR_LATITUDE,
              SENSOR_LONGITUDE,
              SENSOR_ALTITUDE,
              FRAME_CENTER_LATITUDE,
              FRAME_CENTER_LONGITUDE,
              UPPER_LEFT_CORNER_LAT,
              UPPER_LEFT_CORNER_LON,
              UPPER_RIGHT_CORNER_LAT,
              UPPER_RIGHT_CORNER_LON,
              LOWER_RIGHT_CORNER_LAT,
              LOWER_RIGHT_CORNER_LON,
              LOWER_LEFT_CORNER_LAT,
              LOWER_LEFT_CORNER_LON,
              SLANT_RANGE,
              ANGLE_TO_NORTH,
              OBLIQUITY_ANGLE,
              START_DATE_TIME_UTC,
              UNIX_TIMESTAMP,
              PLATFORM_TRUE_AIRSPEED,
              PLATFORM_INDICATED_AIRSPEED,
              PLATFORM_CALL_SIGN,
              FOV_NAME,
              WIND_DIRECTION,
              WIND_SPEED,
              PREDATOR_UAV_UMS,
              PREDATOR_UAV_UMS_V2,
              SENSOR_RELATIVE_ROLL_ANGLE,
              MISSION_ID,
              PLATFORM_TAIL_NUMBER,
              MISSION_NUMBER,
              SENSOR_ROLL_ANGLE,
              UNKNOWN //must be last
  };


  class klv_exception : public std::exception
  {
  public:

    klv_exception(const std::string &str) : name(str) {}
    virtual const char* what() const throw()
    {
      return name.c_str();
    }
    virtual ~klv_exception() throw() {}

    std::string name;
  };

  /// Lookup the cooresponding tag with this key
  tag get_tag(const klv_uds_key &key) const;

  /// Extract the appropriate data type from raw bytes as a boost::any
  boost::any get_value(tag tg, const vxl_byte* data, std::size_t length);

  /// Cast the boost::any to appropriate data type
  template <class T>
  T get_value(tag tg, const boost::any &data) const;

  /// Get the value of the data in the format of a string for any type
  std::string get_string(tag tg, const boost::any &data) const;

  /// Get the name of the tag as a string
  std::string get_tag_name(tag tg) const;

private:

  /// Class to store the tag name and a base class for different
  /// types of values that can come from the klv
  class traits_base
  {
  public:

    virtual ~traits_base() {}

    virtual std::string to_string(const boost::any&) const = 0;
    virtual boost::any convert(const vxl_byte*, std::size_t) = 0;

    std::string name_;

  protected:

    traits_base(const std::string &name) :  name_(name) {}

  };


  /// Provides interpretation of raw data to boost::any and can also
  /// convert the any to a string
  template <class T>
  class traits : public traits_base
  {
  public:

    traits(const std::string &name) : traits_base(name) {}

    std::string to_string(const boost::any &data) const;

    /// Parse type T from a raw byte stream in MSB (most significant byte first) order
    boost::any convert(const vxl_byte* data, std::size_t length);
  };

  klv_0104();
  ~klv_0104();

  static klv_0104 *inst_;

  std::map<klv_uds_key, tag> key_to_tag_;
  std::vector<traits_base *> traitsvec_;
};


} // end namespace vidtk


#endif // VIDTK_KLV_0104_H_
