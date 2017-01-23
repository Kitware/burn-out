/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <klv/klv_data.h>

#include <algorithm>
#include <iomanip>



namespace vidtk {

klv_data
::klv_data()
:     key_offset_( 0 ),
    key_len_( 0 ),
    value_offset_( 0 ),
    value_len_ ( 0 )
{ }


klv_data
::klv_data(container_t const& raw_packet,
         std::size_t key_offset, std::size_t key_len,
         std::size_t value_offset, std::size_t value_len)
  : raw_data_( raw_packet ),
    key_offset_( key_offset ),
    key_len_( key_len ),
    value_offset_( value_offset ),
    value_len_ ( value_len )
{ }


klv_data
::~klv_data()
{ }

std::size_t
klv_data
::key_size() const
{
  return this->key_len_;
}


std::size_t
klv_data
::value_size() const
{
  return this->value_len_;
}


std::size_t
klv_data
::klv_size() const
{
  return this->raw_data_.size();
}


klv_data::const_iterator_t
klv_data
::klv_begin() const
{
  return this->raw_data_.begin();
}


klv_data::const_iterator_t
klv_data
::klv_end() const
{
  return this->raw_data_.end();
}


klv_data::const_iterator_t
klv_data
::key_begin() const
{
  return this->raw_data_.begin() + key_offset_;
}


klv_data::const_iterator_t
klv_data
::key_end() const
{
  return this->raw_data_.begin() + key_offset_ + key_len_;

}


klv_data::const_iterator_t
klv_data
::value_begin() const
{
  return this->raw_data_.begin() + value_offset_;
}


klv_data::const_iterator_t
klv_data
::value_end() const
{
  return this->raw_data_.begin() + value_offset_ + value_len_;
}


std::ostream & operator<<( std::ostream& str, klv_data const& obj )
{
  std::ostream::fmtflags f( str.flags() );
  int i = 0;
  for (klv_data::const_iterator_t it = obj.klv_begin();
       it != obj.klv_end(); it++)
  {
    str << std::hex << std::setfill( '0' ) << std::setw( 2 ) << int(*it);
    if (i % 4 == 3) str << " "; i++;
  }

  str.flags( f );
  return str;
}

} // end namespace vidtk
