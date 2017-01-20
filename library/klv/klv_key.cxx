/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <klv/klv_key.h>
#include <klv/klv_data.h>
#include <algorithm>
#include <iomanip>


namespace vidtk
{

template <unsigned int LEN>
klv_key<LEN>
::klv_key()
{
  std::fill(key_, key_+LEN, 0);
}


template <unsigned int LEN>
klv_key<LEN>
::klv_key(const vxl_byte data[LEN])
{
  std::copy(data, data+LEN, key_);
}


template <unsigned int LEN>
bool klv_key<LEN>
::operator ==(const klv_key& rhs) const
{
  for(unsigned int i=0; i<LEN; ++i)
  {
    if( key_[i] != rhs.key_[i] )
    {
      return false;
    }
  }
  return true;
}


/// Less than operator
template <unsigned int LEN>
bool klv_key<LEN>
::operator <(const klv_key& rhs) const
{
  for(unsigned int i=0; i<LEN; ++i)
  {
    if( key_[i] != rhs.key_[i] )
    {
      return key_[i] < rhs.key_[i];
    }
  }
  return false;
}


template < unsigned int LEN >
std::ostream&
operator<<( std::ostream& os, const klv_key< LEN >& key )
{
  std::ostream::fmtflags f( os.flags() );

  for ( unsigned int i = 0; i < LEN; ++i )
  {
    os << std::hex << std::setfill( '0' ) << std::setw( 2 ) << int(key[i]);
    if ( (i % 4) == 3) os << " ";
  }

  os.flags( f );
  return os;
}




//============================================================================

/// All UDS keys start with this 4 byte prefix
const vxl_byte klv_uds_key
::prefix[] = { 0x06, 0x0e, 0x2b, 0x34 };


/// The UDS 4 byte prefix represted as a uint32 (MSB first)
const vxl_uint_32 klv_uds_key
::prefix_uint32 = ( static_cast< vxl_uint_32 > ( klv_uds_key::prefix[0] ) << 24 ) |
                  ( static_cast< vxl_uint_32 > ( klv_uds_key::prefix[1] ) << 16 ) |
                  ( static_cast< vxl_uint_32 > ( klv_uds_key::prefix[2] ) << 8 )  |
                    static_cast< vxl_uint_32 > ( klv_uds_key::prefix[3] );

// ----------------------


// Create key from raw klv data
klv_uds_key
::klv_uds_key(klv_data const& raw_packet)
{
  unsigned int i = 0;
  vidtk::klv_data::const_iterator_t it = raw_packet.key_begin();
  for ( ; (it != raw_packet.key_end()) && (i < 16); it++, i++)
  {
    this->key_[i] = *it;
  }
}


klv_uds_key
::klv_uds_key( const vxl_byte data[16] )
  : klv_key< 16 > ( data )
{
}


klv_uds_key
::klv_uds_key( const vxl_uint_16 data[8] )
{
  for ( unsigned int i = 0; i < 8; ++i )
  {
    key_[2 * i]     = static_cast< vxl_byte > ( data[i] >> 8 );
    key_[2 * i + 1] = static_cast< vxl_byte > ( data[i] );
  }
}


klv_uds_key
::klv_uds_key( const vxl_uint_32 data[4] )
{
  for ( unsigned int i = 0; i < 4; ++i )
  {
    key_[4 * i]     = static_cast< vxl_byte > ( data[i] >> 24 );
    key_[4 * i + 1] = static_cast< vxl_byte > ( data[i] >> 16 );
    key_[4 * i + 2] = static_cast< vxl_byte > ( data[i] >> 8 );
    key_[4 * i + 3] = static_cast< vxl_byte > ( data[i] );
  }
}


klv_uds_key
::klv_uds_key( const vxl_uint_64 data[2] )
{
  for ( unsigned int i = 0; i < 8; ++i )
  {
    key_[i]     = static_cast< vxl_byte > ( data[0] >> ( 7 - i ) * 8 );
    key_[i + 8] = static_cast< vxl_byte > ( data[1] >> ( 7 - i ) * 8 );
  }
}


klv_uds_key
::klv_uds_key( vxl_uint_64 d1, vxl_uint_64 d2 )
{
  for ( unsigned int i = 0; i < 8; ++i )
  {
    key_[i]   = static_cast< vxl_byte > ( d1 >> ( 7 - i ) * 8 );
    key_[i + 8] = static_cast< vxl_byte > ( d2 >> ( 7 - i ) * 8 );
  }
}


klv_uds_key
::klv_uds_key( vxl_uint_32 d1, vxl_uint_32 d2,
                 vxl_uint_32 d3, vxl_uint_32 d4 )
{
  for ( unsigned int i = 0; i < 4; ++i )
  {
    key_[i]    = static_cast< vxl_byte > ( d1 >> ( 3 - i ) * 8 );
    key_[i + 4]  = static_cast< vxl_byte > ( d2 >> ( 3 - i ) * 8 );
    key_[i + 8]  = static_cast< vxl_byte > ( d3 >> ( 3 - i ) * 8 );
    key_[i + 12] = static_cast< vxl_byte > ( d4 >> ( 3 - i ) * 8 );
  }
}


/// Check if this is a valid 16-byte SMPTE-administered Universal Label
bool klv_uds_key
::is_valid() const
{
  if( !this->is_prefix_valid() )
  {
    return false;
  }

  // UL Designator bytes have 0 in most significant bit
  for(int i = 4; i < 8; ++i)
  {
    if( key_[i] & 0x80 )
    {
      return false;
    }
  }

  switch (this->category())
  {
    case CATEGORY_SINGLE:
      return this->single_type() != SINGLE_INVALID;
    case CATEGORY_GROUP:
      return this->group_type() != GROUP_INVALID;
    case CATEGORY_WRAPPER:
      return this->wrapper_type() != WRAPPER_INVALID;
    case CATEGORY_LABEL:
    case CATEGORY_PRIVATE:
      return true;
    case CATEGORY_INVALID:
    default:
      return false;
  }
  return true;
}


/// Return true if this key has the required 4 byte prefix
bool
klv_uds_key
::is_prefix_valid() const
{
  return key_[0] == prefix[0] &&
         key_[1] == prefix[1] &&
         key_[2] == prefix[2] &&
         key_[3] == prefix[3];
}


/// Return the category represented by this key
klv_uds_key::category_t
klv_uds_key
::category() const
{
  return (key_[4] > 0x05) ? CATEGORY_INVALID
                          : static_cast<category_t>(key_[4]);
}


/// Return the type of single item (aka dictionary) used.
/// Only valid for keys with CATEGORY_SINGLE
klv_uds_key::single_t
klv_uds_key
::single_type() const
{
  if (this->category() != CATEGORY_SINGLE || key_[5] > 0x04)
  {
    return SINGLE_INVALID;
  }
  return static_cast<single_t>(key_[5]);
}


/// Return the type of grouping used.
/// Only valid for keys with CATEGORY_GROUP
klv_uds_key::group_t
klv_uds_key
::group_type() const
{
  // group type encoded in the lower 3 bits
  vxl_byte g = key_[5] & 0x07;
  if (this->category() != CATEGORY_GROUP || g > 0x05)
  {
    return GROUP_INVALID;
  }
  return static_cast<group_t>(g);
}


/// Return the type of wrapper used.
/// Only valid for keys with CATEGORY_WRAPPER
klv_uds_key::wrapper_t
klv_uds_key
::wrapper_type() const
{
  if (this->category() != CATEGORY_WRAPPER || key_[5] > 0x02)
  {
    return WRAPPER_INVALID;
  }
  return static_cast<wrapper_t>(key_[5]);
}


/// Return the number of bytes used to represent length of each group item.
/// Valid only for GROUP_GLOBAL_SET, GROUP_LOCAL_SET, GROUP_VARIABLE_PACK
/// \note return value of 0 indicates BER encoding in either long or short
///       form that can contain variable numbers of bytes
std::size_t
klv_uds_key
::group_item_length_size() const
{
  vxl_byte s = (key_[5] & 0x60) >> 5;
  // the two-bit number from bits 6 and 7 map to 0, 1, 2, 4
  s = (s == 3) ? 4 : s;
  switch(this->group_type())
  {
    case GROUP_GLOBAL_SET:
    case GROUP_LOCAL_SET:
    case GROUP_VARIABLE_PACK:
      return s;
    default:
      break;
  }
  return 0;
}


/// Return the number of bytes used to represent the local tags.
/// Valid only for GROUP_LOCAL_SET
/// \note return value of 0 indicates OID BER encoding
///       that can contain variable numbers of bytes
std::size_t
klv_uds_key
::group_item_tag_size() const
{
  if (this->group_type() != GROUP_LOCAL_SET)
  {
    return 0;
  }
  // the two-bit number from bits 4 and 5 map to the following values
  const size_t map[] = {1, 0, 2, 4};
  return map[(key_[5] & 0x18) >> 3];
}


//============================================================================


klv_lds_key
::klv_lds_key(vxl_byte data)
{
  key_[0] = data;
}


klv_lds_key
::klv_lds_key(const vxl_byte data[1])
{
  key_[0] = *data;
}


//============================================================================


#define INSTANTIATE_KLV_KEY(NUM) \
template class klv_key<NUM>; \
template std::ostream& operator <<(std::ostream& os, const klv_key<NUM>& key)

INSTANTIATE_KLV_KEY(1);
INSTANTIATE_KLV_KEY(16);

} // end namespace vidtk
