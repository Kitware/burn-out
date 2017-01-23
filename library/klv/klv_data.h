/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_KLV_DATA_H_
#define VIDTK_KLV_DATA_H_

#include <vxl_config.h>
#include <cstddef>
#include <iostream>
#include <vector>


namespace vidtk
{


// ----------------------------------------------------------------
/** A container for a raw KLV packet.
 *
 * This class represents a single KLV packet, that includes Key,
 * Length, and Value components.
 *
 * An object of this class is immutable. Once it is created, it can
 * not be changed, only querried.
 */
class klv_data
{
public:
  typedef std::vector< vxl_byte > container_t;
  typedef container_t::const_iterator const_iterator_t;


  klv_data();

  /** Build a new object from raw packet. A raw packet is supplied
   * with the offsets to the intresting parts. The raw data is copied
   * into this object so the caller can dispose of its copy as
   * desired.
   *
   * What's not captured is the length of the length, but that can be
   * determined if needed.
   */
  klv_data(container_t const& raw_packet,
           size_t key_offset, size_t key_len,
           size_t value_offset, size_t value_len);

  virtual ~klv_data();

  /// The number of bytes in the key
  std::size_t key_size() const;

  /// Number of bytes in the value portion
  std::size_t value_size() const;

  /// Number of bytes in whole packet
  std::size_t klv_size() const;

  /// Iterators for raw packet
  const_iterator_t klv_begin() const;
  const_iterator_t klv_end() const;

  /// Iterators for key
  const_iterator_t key_begin() const;
  const_iterator_t key_end() const;

  /// Iterators for value
  const_iterator_t value_begin() const;
  const_iterator_t value_end() const;

protected:
  std::vector<vxl_byte> raw_data_;
  std::size_t key_offset_;
  std::size_t key_len_;
  std::size_t value_offset_;
  std::size_t value_len_;
};


/// Output operator
std::ostream& operator<< (std::ostream& str, klv_data const& obj);

} // end namespace vidtk


#endif // VIDTK_KLV_DATA_H_
