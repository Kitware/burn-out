/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <klv/klv_0601.h>
#include <klv/klv_0601_traits.h>
#include <klv/klv_data.h>
#include <logger/logger.h>
#include <typeinfo>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/function.hpp>
#include <boost/bind.hpp>


namespace
{

using namespace vidtk;
VIDTK_LOGGER("klv_0601");

/// A function type that converts raw byte streams to boost::any
typedef boost::function<boost::any (const vxl_byte*, std::size_t)> klv_decode_func_t;

/// Parse type T from a raw byte stream in MSB (most significant byte first) order
template <typename T>
boost::any klv_convert(const vxl_byte* data, std::size_t length)
{
  if (sizeof(T) != length)
  {
    LOG_WARN("Data type and length differ in size.");
  }

  T value = *(data++);
  for(std::size_t i=1; i<length; ++i, ++data)
  {
    value <<= 8;
    value |= *data;
  }
  return value;
}


/// Specialization for extracting strings from a raw byte stream
template <>
boost::any klv_convert<std::string>(const vxl_byte* data, std::size_t length)
{
  std::string value(reinterpret_cast<const char*>(data), length);
  return value;
}


/// Specialization for extracting STD 0102 LSD from raw byte stream
/// \note this is a place holder for now.
template <>
boost::any klv_convert<std_0102_lds>(const vxl_byte* data, std::size_t length)
{
  std::vector<vxl_byte> value(data, data+length);
  return value;
}


/// A function type that converts a boost::any to a double
typedef boost::function<double (const boost::any&)> klv_any_to_double_func_t;


/// Take a "convert T to double" function apply to a boost::any
/// This is used with boost bind to make a boost::any to double conversion function
template <typename T>
double klv_as_double(const boost::function<double (const T& val)>& func,
                     const boost::any& data)
{
  return func(boost::any_cast<T>(data));
}


/// A function type to format boost::any raw data in hex and write to the ostream
typedef boost::function<void (std::ostream& os, const boost::any&)> klv_any_format_hex_func_t;


/// Write boost::any (with underlying type T) in hex
template <typename T>
void format_hex(std::ostream& os, const boost::any& data)
{
  std::iostream::fmtflags f( os.flags() );
  os << std::hex << std::setfill ('0') << std::setw(sizeof(T)*2)
     << boost::any_cast<T>(data);
  os.flags( f );
}


/// Specialization for writing a byte in hex (so it doesn't print ASCII)
template <>
void format_hex<vxl_byte>(std::ostream& os, const boost::any& data)
{
  std::iostream::fmtflags f( os.flags() );
  os << std::hex << std::setfill ('0') << std::setw(2)
     << static_cast<unsigned int>(boost::any_cast<vxl_byte>(data));
  os.flags( f );
}


/// Specialization for writing a byte in hex (so it doesn't print ASCII)
template <>
void format_hex<vxl_int_8>(std::ostream& os, const boost::any& data)
{
  std::iostream::fmtflags f( os.flags() );
  os << std::hex << std::setfill ('0') << std::setw(2)
     << static_cast<unsigned int>(boost::any_cast<vxl_int_8>(data));
  os.flags( f );
}


/// Specialization for writing a string as a sequence of hex bytes
template <>
void format_hex<std::string>(std::ostream& os, const boost::any& data)
{
  std::string s = boost::any_cast<std::string>(data);
  std::iostream::fmtflags f( os.flags() );
  for(unsigned int k=0; k<s.size(); ++k)
  {
    os << std::hex << std::setfill ('0') << std::setw(2)
       << static_cast<unsigned int>(s[k]);
  }
  os.flags( f );
}


/// Specialization for writing a STD 0102 LDS in hex bytes
template <>
void format_hex<std_0102_lds>(std::ostream& os, const boost::any& data)
{
  std::iostream::fmtflags f( os.flags() );
  std::vector<vxl_byte> d = boost::any_cast<std::vector<vxl_byte> >(data);
  for(unsigned int k=0; k<d.size(); ++k)
  {
    os << std::hex << std::setfill ('0') << std::setw(2)
       << static_cast<unsigned int>(d[k]);
  }
  os.flags( f );
}


/// Store KLV 0601 traits for dynamic run-time lookup
/// Build an array of these structs, one for each 0601 tag,
/// using template metaprogramming.
struct klv_0601_dyn_traits
{
  std::string name;
  const std::type_info* type;
  unsigned int num_bytes;
  klv_decode_func_t decode_func;
  bool has_double;
  klv_any_to_double_func_t double_func;
  klv_any_format_hex_func_t any_hex_func;
};


/// Recursive template metaprogram to populate the run-time array of traits
template <klv_0601_tag tag>
struct construct_traits
{
  /// Populate the array element for this tag
  static inline std::vector<klv_0601_dyn_traits>&
  init(std::vector<klv_0601_dyn_traits>& data)
  {
    typedef typename klv_0601_traits<tag>::type type;
    klv_0601_dyn_traits& t = data[tag];
    t.name = klv_0601_traits<tag>::name();
    t.type =  &typeid(type);
    t.num_bytes = sizeof(type);
    t.decode_func = klv_decode_func_t(klv_convert<type>);
    t.has_double = klv_0601_convert<tag>::has_double;
    t.double_func = boost::bind(klv_as_double<type>,
                                klv_0601_convert<tag>::as_double, _1);
    t.any_hex_func = klv_any_format_hex_func_t(format_hex<type>);
    return construct_traits<klv_0601_tag(tag-1)>::init(data);
  }
};


/// The base case: unknown tag (with ID = 0)
template <>
struct construct_traits<vidtk::KLV_0601_UNKNOWN>
{
  static inline std::vector<klv_0601_dyn_traits>&
  init(std::vector<klv_0601_dyn_traits>& data)
  {
    klv_0601_dyn_traits& t = data[KLV_0601_UNKNOWN];
    t.name = "Unknown";
    t.type =  &typeid(void);
    t.num_bytes = 0;
    t.has_double = false;
    return data;
  }

};


/// Construct an array of traits for all known 0601 tags
std::vector<klv_0601_dyn_traits> init_traits_array()
{
  std::vector<klv_0601_dyn_traits> tmp(KLV_0601_ENUM_END);
  return construct_traits<klv_0601_tag(KLV_0601_ENUM_END-1)>::init(tmp);
}

static const std::vector<klv_0601_dyn_traits> traits_array = init_traits_array();

static const vxl_byte key_data[16] = {
  0x06, 0x0e, 0x2b, 0x34,
  0x02, 0x0B, 0x01, 0x01,
  0x0E, 0x01, 0x03, 0x01,
  0x01, 0x00, 0x00, 0x00 };

 static const klv_uds_key klv_0601_uds_key(key_data);

} // end anonymous namespace


//=============================================================================
// Public vidtk function implementations below
//=============================================================================
namespace vidtk
{

/** Globally available 0601 key.
 *
 */
klv_uds_key klv_0601_key()
{
  return klv_0601_uds_key;
}


bool is_klv_0601_key( klv_uds_key const& key)
{
  return (key == klv_0601_uds_key);
}


// ----------------------------------------------------------------
/** Compute 0601 checksum.
 *
 * Verify the 0601 block checksum against the expected checksum.  The
 * supplied data is a klv UDS packet which may contain multiple 0601
 * LDS data packets.
 *
 * The checksum is a running 16-bit sum through the entire UDS packet
 * starting with the 16 byte Local Data Set key and ending with
 * summing the length field of the checksum data item.
 *
 * Packet layout
 *
 *  |__| ... |__|__|__|__|
 *            ^  ^  ^--^----- 16 bit checksum
 *            |  |
 *            |   ----------- length of checksum in bytes (= 2)
 *             -------------- checksum packet type code (= 1)
 *
 * @param[in] data raw data to checksum
 *
 * @return \b True if checksum matches expected; \b False otherwise.
 * \b False is also returned if checksum tag is not found.
 */
bool
klv_0601_checksum( klv_data const& data )
{

#if defined(KLV_0601_CHECKSUM_DISABLE)
  ///@todo DARTS checksum work-around
  // DARTS produces videos and streams which have invalid checksums.
  // This is a "temporary" work-around until DARTS gets fixed.
#if defined(WIN32) && !defined(__CYGWIN__)
#pragma message("Checksum calculation disabled")
#else
#warning "Checksum calculation disabled"
#endif
  return true;
#else

  vidtk::klv_data::const_iterator_t eit = data.klv_end();


  // if checksum tag is not where expected then terminate early
  if ( ( *(eit - 4) != 0x01 ) && //
       ( *(eit - 3) != 0x02 ) ) // cksum length
  {
    return false;
  }

  // Retrieve checksum from raw data
  vxl_uint_16 cksum = ( *(eit - 2) << 8 ) | ( *(eit - 1) );

  vxl_uint_16 bcc(0);
  size_t len = data.klv_size() - 2;
  vidtk::klv_data::const_iterator_t cit = data.klv_begin();

  // Sum each 16-bit chunk within the buffer into a checksum
  for (unsigned i = 0; i < len; i++)
  {
    bcc += *cit << (8 * ((i + 1) % 2));
    cit++;
  }

  return bcc == cksum;

#endif
}


/// Return a string representation of the name of a KLV 0601 tag
std::string
klv_0601_tag_to_string(klv_0601_tag t)
{
  return traits_array[t].name;
}


/// Extract the appropriate data type from raw bytes as a boost::any
boost::any
klv_0601_value(klv_0601_tag t, const vxl_byte* data, std::size_t length)
{
  return traits_array[t].decode_func(data, length);
}


/// Return the tag data as a double
double
klv_0601_value_double(klv_0601_tag t, const boost::any& data)
{
  return traits_array[t].double_func(data);
}


/// Format the tag data as a string
std::string
klv_0601_value_string(klv_0601_tag t, const boost::any& data)
{
  const klv_0601_dyn_traits& traits = traits_array[t];
  if(traits.type == &typeid(std::string))
  {
    return boost::any_cast<std::string>(data);
  }
  if(traits.has_double)
  {
    std::stringstream ss;
    ss << std::setprecision(traits.num_bytes * 3)
       << traits.double_func(data);
    return ss.str();
  }
  if( t == KLV_0601_UNIX_TIMESTAMP )
  {
    std::stringstream ss;
    typedef klv_0601_traits<KLV_0601_UNIX_TIMESTAMP>::type time_type;
    ss << boost::any_cast<time_type>(data);
    return ss.str();
  }
  return "Unknown";
}


/// Format the tag data as a hex string
std::string
klv_0601_value_hex_string(klv_0601_tag t, const boost::any& data)
{
  std::stringstream ss;
  traits_array[t].any_hex_func(ss, data);
  return ss.str();
}

} // end namespace vidtk
