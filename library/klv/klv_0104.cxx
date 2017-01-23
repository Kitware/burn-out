/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <klv/klv_0104.h>
#include <klv/klv_data.h>
#include <sstream>
#include <iostream>
#include <map>
#include <iomanip>
#include <boost/assign.hpp>


namespace
{

using namespace vidtk;

static const vxl_byte key_data[16] = {
  0x06, 0x0E, 0x2B, 0x34,
  0x02, 0x01, 0x01, 0x01,
  0x0E, 0x01, 0x01, 0x02,
  0x01, 0x01, 0x00, 0x00
};

static const klv_uds_key klv_0104_uds_key(key_data);

}  //end anonymous namespace

namespace vidtk
{

using namespace boost::assign;

klv_0104 *klv_0104::inst_ = 0;

klv_0104 *klv_0104::inst()
{
  return inst_ ? inst_ : inst_ = new klv_0104;
}

klv_0104::klv_0104()
{
  //Fill out mapping between global keys and tags
#if ( __cplusplus >= 201103L || __GXX_EXPERIMENTAL_CXX0X__ || (defined(_MSC_VER) && _MSC_VER >= 1800))
  boost::assign::insert(key_to_tag_)
#else
  key_to_tag_ =  boost::assign::map_list_of
#endif
    (klv_uds_key(0x060e2b3401010101UL,0x0101200100000000UL), PLATFORM_DESIGNATION)
    (klv_uds_key(0x060e2b3401010103UL,0x0101210100000000UL), PLATFORM_DESIGNATION_ALT)
    (klv_uds_key(0x060e2b3401010103UL,0x0103040200000000UL), STREAM_ID)
    (klv_uds_key(0x060e2b3401010103UL,0x0103060100000000UL), ITEM_DESIGNATOR_ID)
    (klv_uds_key(0x060e2b3402010101UL,0x0208020000000000UL), CLASSIFICATION)
    (klv_uds_key(0x060e2b3401010103UL,0x0208020100000000UL), SECURITY_CLASSIFICATION)
    (klv_uds_key(0x060e2b3401010101UL,0x0420010201010000UL), IMAGE_SOURCE_SENSOR)
    (klv_uds_key(0x060e2b3401010102UL,0x0420020101080000UL), SENSOR_HORIZONTAL_FOV)
    (klv_uds_key(0x060e2b3401010107UL,0x04200201010a0100UL), SENSOR_VERTICAL_FOV)
    (klv_uds_key(0x060E2B3401010101UL,0x0420030100000000UL), SENSOR_TYPE)
    (klv_uds_key(0x060e2b3401010101UL,0x0701010100000000UL), IMAGE_COORDINATE_SYSTEM)
    (klv_uds_key(0x060e2b3401010101UL,0x0701090201000000UL), TARGET_WIDTH)
    (klv_uds_key(0x060e2b3401010107UL,0x0701100106000000UL), PLATFORM_HEADING)
    (klv_uds_key(0x060e2b3401010107UL,0x0701100105000000UL), PLATFORM_PITCH_ANGLE)
    (klv_uds_key(0x060e2b3401010107UL,0x0701100104000000UL), PLATFORM_ROLL_ANGLE)
    (klv_uds_key(0x060e2b3401010103UL,0x0701020102040200UL), SENSOR_LATITUDE)
    (klv_uds_key(0x060e2b3401010103UL,0x0701020102060200UL), SENSOR_LONGITUDE)
    (klv_uds_key(0x060E2B3401010101UL,0x0701020102020000UL), SENSOR_ALTITUDE)
    (klv_uds_key(0x060e2b3401010101UL,0x0701020103020000UL), FRAME_CENTER_LATITUDE)
    (klv_uds_key(0x060e2b3401010101UL,0x0701020103040000UL), FRAME_CENTER_LONGITUDE)
    (klv_uds_key(0x060E2B3401010103UL,0x0701020103070100UL), UPPER_LEFT_CORNER_LAT)
    (klv_uds_key(0x060E2B3401010103UL,0x07010201030B0100UL), UPPER_LEFT_CORNER_LON)
    (klv_uds_key(0x060E2B3401010103UL,0x0701020103080100UL), UPPER_RIGHT_CORNER_LAT)
    (klv_uds_key(0x060E2B3401010103UL,0x07010201030C0100UL), UPPER_RIGHT_CORNER_LON)
    (klv_uds_key(0x060E2B3401010103UL,0x0701020103090100UL), LOWER_RIGHT_CORNER_LAT)
    (klv_uds_key(0x060E2B3401010103UL,0x07010201030D0100UL), LOWER_RIGHT_CORNER_LON)
    (klv_uds_key(0x060E2B3401010103UL,0x07010201030A0100UL), LOWER_LEFT_CORNER_LAT)
    (klv_uds_key(0x060E2B3401010103UL,0x07010201030E0100UL), LOWER_LEFT_CORNER_LON)
    (klv_uds_key(0x060e2b3401010101UL,0x0701080101000000UL), SLANT_RANGE)
    (klv_uds_key(0x060E2B3401010101UL,0x0701100102000000UL), ANGLE_TO_NORTH)
    (klv_uds_key(0x060E2B3401010101UL,0x0701100103000000UL), OBLIQUITY_ANGLE)
    (klv_uds_key(0x060e2b3401010101UL,0x0702010201010000UL), START_DATE_TIME_UTC)
    (klv_uds_key(0x060e2b3401010103UL,0x0702010101050000UL), UNIX_TIMESTAMP)
    (klv_uds_key(0x060e2b3401010101UL,0x0e0101010a000000UL), PLATFORM_TRUE_AIRSPEED)
    (klv_uds_key(0x060e2b3401010101UL,0x0e0101010b000000UL), PLATFORM_INDICATED_AIRSPEED)
    (klv_uds_key(0x060e2b3401010101UL,0x0e01040101000000UL), PLATFORM_CALL_SIGN)
    (klv_uds_key(0x060e2b3401010101UL,0x0e01020202000000UL), FOV_NAME)
    (klv_uds_key(0x060E2B3402010101UL,0x0e0101010D000000UL), WIND_DIRECTION)
    (klv_uds_key(0x060E2B3401010101UL,0x0e0101010E000000UL), WIND_SPEED)
    (klv_uds_key(0x060E2B3401010101UL,0x0e01010201010000UL), PREDATOR_UAV_UMS)
    (klv_uds_key(0x060E2B3402010101UL,0x0e01010201010000UL), PREDATOR_UAV_UMS_V2)
    (klv_uds_key(0x060E2B3401010101UL,0x0e01010206000000UL), SENSOR_RELATIVE_ROLL_ANGLE)
    (klv_uds_key(0x060e2b3401010101UL,0x0e01040103000000UL), MISSION_ID)
    (klv_uds_key(0x060e2b3401010101UL,0x0e01040102000000UL), PLATFORM_TAIL_NUMBER)
    (klv_uds_key(0x060e2b3401010101UL,0x0105050000000000UL), MISSION_NUMBER)
    (klv_uds_key(0x060e2b3401010101UL,0x0701100101000000UL), SENSOR_ROLL_ANGLE);

  //UNKNOWN is the last entry of the enum and thus the number of tags
  traitsvec_.resize(UNKNOWN);

  //Mapping between tag index and traits, double is used for floats and doubles
  traitsvec_[PLATFORM_DESIGNATION] = new traits<std::string>("Platform designation");
  traitsvec_[PLATFORM_DESIGNATION_ALT] = new traits<std::string>("Platform designation (alternate key)");
  traitsvec_[STREAM_ID] = new traits<std::string>("Stream ID");
  traitsvec_[ITEM_DESIGNATOR_ID] = new traits<std::string>("Item Designator ID (16 bytes)");
  traitsvec_[CLASSIFICATION] = new traits<std::string>("Classification");
  traitsvec_[SECURITY_CLASSIFICATION] = new traits<std::string>("Security Classification");
  traitsvec_[IMAGE_SOURCE_SENSOR] = new traits<std::string>("Image Source sensor");
  traitsvec_[SENSOR_HORIZONTAL_FOV] = new traits<double>("Sensor horizontal field of view");
  traitsvec_[SENSOR_VERTICAL_FOV] = new traits<double>("Sensor vertical field of view");
  traitsvec_[SENSOR_TYPE] = new traits<std::string>("Sensor type");
  traitsvec_[IMAGE_COORDINATE_SYSTEM] = new traits<std::string>("Image Coordinate System");
  traitsvec_[TARGET_WIDTH] = new traits<double>("Target Width");
  traitsvec_[PLATFORM_HEADING] = new traits<double>("Platform heading");
  traitsvec_[PLATFORM_PITCH_ANGLE] = new traits<double>("Platform pitch angle");
  traitsvec_[PLATFORM_ROLL_ANGLE] = new traits<double>("Platform roll angle");
  traitsvec_[SENSOR_LATITUDE] = new traits<double>("Sensor latitude");
  traitsvec_[SENSOR_LONGITUDE] = new traits<double>("Sensor longitude");
  traitsvec_[SENSOR_ALTITUDE] = new traits<double>("Sensor Altitude");
  traitsvec_[FRAME_CENTER_LATITUDE] = new traits<double>("Frame center latitude");
  traitsvec_[FRAME_CENTER_LONGITUDE] = new traits<double>("Frame center longitude");
  traitsvec_[UPPER_LEFT_CORNER_LAT] = new traits<double>("Upper left corner latitude");
  traitsvec_[UPPER_LEFT_CORNER_LON] = new traits<double>("Upper left corner longitude");
  traitsvec_[UPPER_RIGHT_CORNER_LAT] = new traits<double>("Upper right corner latitude");
  traitsvec_[UPPER_RIGHT_CORNER_LON] = new traits<double>("Upper right corner longitude");
  traitsvec_[LOWER_RIGHT_CORNER_LAT] = new traits<double>("Lower right corner latitude");
  traitsvec_[LOWER_RIGHT_CORNER_LON] = new traits<double>("Lower right corner longitude");
  traitsvec_[LOWER_LEFT_CORNER_LAT] = new traits<double>("Lower left corner latitude");
  traitsvec_[LOWER_LEFT_CORNER_LON] = new traits<double>("Lower left corner longitude");
  traitsvec_[SLANT_RANGE] = new traits<double>("Slant range");
  traitsvec_[ANGLE_TO_NORTH] = new traits<double>("Angle to north");
  traitsvec_[OBLIQUITY_ANGLE] = new traits<double>("Obliquity angle");
  traitsvec_[START_DATE_TIME_UTC] = new traits<std::string>("Start Date Time - UTC");
  traitsvec_[UNIX_TIMESTAMP] = new traits<vxl_uint_64>("Unix timestamp");
  traitsvec_[PLATFORM_TRUE_AIRSPEED] = new traits<double>("Platform true airspeed");
  traitsvec_[PLATFORM_INDICATED_AIRSPEED] = new traits<double>("Platform indicated airspeed");
  traitsvec_[PLATFORM_CALL_SIGN] = new traits<std::string>("Platform call sign");
  traitsvec_[FOV_NAME] = new traits<std::string>("Field of view name");
  traitsvec_[WIND_DIRECTION] = new traits<std::string>("Wind Direction");
  traitsvec_[WIND_SPEED] = new traits<double>("Wind Speed");
  traitsvec_[PREDATOR_UAV_UMS] = new traits<std::string>("Predator UAV Universal Metadata Set");
  traitsvec_[PREDATOR_UAV_UMS_V2] = new traits<std::string>("Predator UAV Universal Metadata Set v2.0");
  traitsvec_[SENSOR_RELATIVE_ROLL_ANGLE] = new traits<double>("Sensor Relative Roll Angle");
  traitsvec_[MISSION_ID] = new traits<std::string>("Mission ID");
  traitsvec_[PLATFORM_TAIL_NUMBER] = new traits<std::string>("Platform tail number");
  traitsvec_[MISSION_NUMBER] = new traits<std::string>("Episode Number");
  traitsvec_[SENSOR_ROLL_ANGLE] = new traits<double>("Sensor Roll Angle");
}

klv_0104::~klv_0104()
{
  for (unsigned int i = 0; i < traitsvec_.size(); i++)
    delete traitsvec_[i];
}

template <class T>
std::string klv_0104::traits<T>::to_string(const boost::any &data) const
{
  T var = boost::any_cast<T>(data);
  std::stringstream ss;
  ss << var;
  return ss.str();
}

template <class T>
boost::any klv_0104::traits<T>::convert(const vxl_byte* data, std::size_t length)
{
  if (sizeof(T) != length)
    std::cerr << "Warning: length=" << length << ", sizeof(type)=" << sizeof(T) << " ";

  union
  {
    T val;
    char bytes[sizeof(T)];
  } converter;

  for (int i = sizeof(T)-1; i >= 0; i--, data++)
  {
    converter.bytes[i] = *data;
  }

  return converter.val;
}

/// Specialization for extracting strings from a raw byte stream
template <>
boost::any klv_0104::traits<std::string>::convert(const vxl_byte* data, std::size_t length)
{
  std::string value(reinterpret_cast<const char*>(data), length);
  return value;
}

///Handle real values as floats or doubles but return double
template <>
boost::any klv_0104::traits<double>::convert(const vxl_byte* data, std::size_t length)
{
  double val;
  if (length == sizeof(float))
  {
    union
    {
      float val;
      char bytes[sizeof(float)];
    } converter;

    for (int i = sizeof(float)-1; i >= 0; i--, data++)
    {
      converter.bytes[i] = *data;
    }

    val = static_cast<double>(converter.val);
  }
  else if (length == sizeof(double))
  {
    union
    {
      double val;
      char bytes[sizeof(double)];
    } converter;

    for (int i = sizeof(double)-1; i >= 0; i--, data++)
    {
      converter.bytes[i] = *data;
    }

    val = converter.val;
  }
  else
  {
    throw klv_exception("Length does not correspond to double or float.");
    return 0;
  }

  return val;
}

klv_0104::tag klv_0104::get_tag(const klv_uds_key &k) const
{
  std::map<klv_uds_key, tag>::const_iterator itr = key_to_tag_.find(k);
  if (itr == key_to_tag_.end())
    return UNKNOWN;

  return itr->second;
}

boost::any klv_0104::get_value(tag tg, const vxl_byte* data, std::size_t length)
{
  return traitsvec_[tg]->convert(data, length);
}

template <class T>
T klv_0104::get_value(tag tg, const boost::any &data) const
{
  traits<T> * t = dynamic_cast<traits<T> *>(traitsvec_[tg]);
  if (!t)
  {
    throw klv_exception(t->name_ + "' type mismatch");
  }

  return boost::any_cast<T>(data);
}

std::string klv_0104::get_string(tag tg, const boost::any &data) const
{
  return traitsvec_[tg]->to_string(data);
}

std::string klv_0104::get_tag_name(tag tg) const
{
  return traitsvec_[tg]->name_;
}

klv_uds_key klv_0104::key()
{
  return klv_0104_uds_key;
}

bool klv_0104::is_key( klv_uds_key const& key)
{
  return (key == klv_0104_uds_key);
}

template double klv_0104::get_value<double>(tag tg, const boost::any &data) const;
template vxl_uint_64 klv_0104::get_value<vxl_uint_64>(tag tg, const boost::any &data) const;
template std::string klv_0104::get_value<std::string>(tag tg, const boost::any &data) const;

} // end namespace vidtk
