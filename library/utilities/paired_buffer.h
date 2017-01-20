/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_paired_buffer_h_
#define vidtk_paired_buffer_h_

#include <boost/circular_buffer.hpp>
#include <utility>
#include <vector>

// For convenience
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <vxl_config.h>

namespace vidtk
{

template< class keyT, class datumT >
class key_datum_pair
{
public:
  key_datum_pair(){}
  key_datum_pair( keyT const& k, datumT const& d )
    : key_( k ),
      datum_( d )
  {}

  bool operator<( key_datum_pair< keyT, datumT > const& other ) const
  {
    return this->key_ < other.key_;
  }

  bool operator==( key_datum_pair< keyT, datumT > const& other ) const
  {
    return this->key_ == other.key_;
  }

  keyT key_;
  datumT datum_;
};

template< class keyT, class datumT >
class paired_buffer
{
public:
  typedef key_datum_pair< keyT, datumT > key_datum_t;
  typedef boost::circular_buffer_space_optimized< key_datum_t > buffer_t;

  paired_buffer();
  paired_buffer( unsigned length );

  bool initialize();

  bool reset();

  void set_length( unsigned len );

  datumT const& find_datum( keyT const& key,
                            bool & found,
                            bool is_sorted = false ) const;

  datumT & find_datum( keyT const& key,
                       bool & found,
                       bool is_sorted = false );

  bool find_datum( keyT const& key,
                   typename buffer_t::const_iterator & iter,
                   bool is_sorted = false ) const;

  bool replace_items( std::vector< key_datum_t > const& );

  buffer_t buffer_;

private:
  unsigned length_;

  // use when signalling "not found" in find_datum()
  // see comments in find_datum() to explain why it's mutable
  mutable datumT default_return_value_;
};

typedef paired_buffer< timestamp, vil_image_view< vxl_byte > > ts_byte_img_buff_t;


} // namespace vidtk

#endif // vidtk_paired_buffer_h_
