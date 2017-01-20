/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_KLV_KEY_H_
#define VIDTK_KLV_KEY_H_

#include <vxl_config.h>
#include <cstddef>
#include <iostream>


namespace vidtk
{

class klv_data;

/// A class to represent a KLV key
template <unsigned int LEN>
class klv_key
{
public:
  klv_key();
  virtual ~klv_key() { }

  klv_key(const vxl_byte data[LEN]);

  /// The number of bytes in the key
  static std::size_t size() { return LEN; }

  /// Access a byte of the key
  inline vxl_byte operator[](unsigned int i) const
  {
    return (i < LEN) ? key_[i] : 0;
  }

  /// Compare keys for equality
  bool operator ==(const klv_key& rhs) const;

  /// Less than operator
  bool operator <(const klv_key& rhs) const;

protected:
  vxl_byte key_[LEN];
};


/// Ouput stream operator for \a klv_key
/// formats output as a hex string
template <unsigned int LEN>
std::ostream& operator <<(std::ostream& os, const klv_key<LEN>& key);


// ----------------------------------------------------------------
/** A UDS (Universal Data Set) key with 16 byte length
 *
 *
 */
class klv_uds_key :
  public klv_key< 16 >
{
public:
  klv_uds_key() { }

  virtual ~klv_uds_key() { }

  klv_uds_key( klv_data const& raw_packet );
  explicit klv_uds_key( const vxl_byte data[16] );
  explicit klv_uds_key( const vxl_uint_16 data[8] );
  explicit klv_uds_key( const vxl_uint_32 data[4] );
  explicit klv_uds_key( const vxl_uint_64 data[2] );
  explicit klv_uds_key( vxl_uint_64 d1, vxl_uint_64 d2 );
  explicit klv_uds_key( vxl_uint_32 d1, vxl_uint_32 d2, vxl_uint_32 d3, vxl_uint_32 d4 );

  /// Check if this is a valid 16-byte SMPTE-administered Universal Label
  bool is_valid() const;

  /// Return true if this key has the required 4 byte prefix
  bool is_prefix_valid() const;

  /// Categories of KLV types (represented by byte 5)
  enum category_t { CATEGORY_INVALID = 0x00,
                    CATEGORY_SINGLE  = 0x01,
                    CATEGORY_GROUP   = 0x02,
                    CATEGORY_WRAPPER = 0x03,
                    CATEGORY_LABEL   = 0x04,
                    CATEGORY_PRIVATE = 0x05 };

  /// Sub-categories of KLV single items (represented by byte 6)
  enum single_t { SINGLE_INVALID  = 0x00,
                  SINGLE_METADATA = 0x01,
                  SINGLE_ESSENCE  = 0x02,
                  SINGLE_CONTROL  = 0x03,
                  SINGLE_TYPE     = 0x04 };

  /// Sub-categories of KLV group items (represented by byte 6)
  enum group_t { GROUP_INVALID       = 0x00,
                 GROUP_UNIVERSAL_SET = 0x01,
                 GROUP_GLOBAL_SET    = 0x02,
                 GROUP_LOCAL_SET     = 0x03,
                 GROUP_VARIABLE_PACK = 0x04,
                 GROUP_FIXED_PACK    = 0x05 };

  /// Sub-categories of KLV wrapper items (represented by byte 6)
  enum wrapper_t { WRAPPER_INVALID = 0x00,
                   WRAPPER_SIMPLE  = 0x01,
                   WRAPPER_COMPLEX = 0x02 };

  /// Return the category represented by this key
  category_t category() const;

  /// Return the type of single item (aka dictionary) used.
  /// Only valid for keys with CATEGORY_SINGLE
  single_t single_type() const;

  /// Return the type of grouping used.
  /// Only valid for keys with CATEGORY_GROUP
  group_t group_type() const;

  /// Return the type of wrapper used.
  /// Only valid for keys with CATEGORY_WRAPPER
  wrapper_t wrapper_type() const;

  /// Return the number of bytes used to represent length of each group item.
  /// Valid only for GROUP_GLOBAL_SET, GROUP_LOCAL_SET, GROUP_VARIABLE_PACK
  /// \note return value of 0 indicates BER encoding in either long or short
  ///       form that can contain variable numbers of bytes
  std::size_t group_item_length_size() const;

  /// Return the number of bytes used to represent the local tags.
  /// Valid only for GROUP_LOCAL_SET
  /// \note return value of 0 indicates OID BER encoding
  ///       that can contain variable numbers of bytes
  std::size_t group_item_tag_size() const;

  /// @todo Provide method for comparing buffer to key
  /// Usage is to compare against a std::dequeue

  /// All UDS keys start with this 4 byte prefix
  static const vxl_byte prefix[4];

  /// The UDS 4 byte prefix represted as a uint32 (MSB first)
  static const vxl_uint_32 prefix_uint32;
};


// ----------------------------------------------------------------
/// A LDS (Local Data Set) key with 1 byte length
class klv_lds_key
  : public klv_key<1>
{
public:
  klv_lds_key() {}
  virtual ~klv_lds_key() {}

  klv_lds_key(vxl_byte data);
  klv_lds_key(const vxl_byte data[1]);

  /// Operator to cast to a vxl_byte
  operator vxl_byte() const { return key_[0]; }
};


} // end namespace vidtk


#endif // VIDTK_KLV_KEY_H_
