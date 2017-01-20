/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/ring_buffer_process.txx>

#include <vector>
#include <algorithm>
#include <vil/vil_image_view.h>
#include <utilities/timestamp.h>

template class vidtk::ring_buffer_process< vidtk::timestamp >;
template class vidtk::ring_buffer_process< vil_image_view<vxl_byte> >;
template class vidtk::ring_buffer_process< vil_image_view<vxl_uint_16> >;

namespace
{

template<class Data>
unsigned find_offset_in_vector( std::vector<Data> const& buff,
                                unsigned head,
                                Data const & datum )
{
  typename std::vector<Data>::const_iterator iter = std::find( buff.begin(),
                                                             buff.end(),
                                                             datum );
  unsigned offset;
  if( iter == buff.end() )
  {
    offset = unsigned(-1);
  }
  else
  {
    unsigned const idx = std::distance( buff.begin(), iter );
    unsigned const N = buff.size();
    offset = ( head + N - idx) % N;
  }

  return offset;
}

}

// Define specializations of offset_of for types we can handle.
template <>
unsigned
vidtk::ring_buffer_process< vidtk::timestamp >
::offset_of( vidtk::timestamp const& ts ) const
{
  return find_offset_in_vector( this->buffer_, this->head_, ts );
}
