/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/ring_buffer_process.txx>

#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <utilities/timestamp.h>
#include <vcl_algorithm.h>

template class vidtk::ring_buffer_process< vidtk::timestamp >;
template class vidtk::ring_buffer_process< vil_image_view<vxl_byte> >;


namespace
{

template<class Data>
unsigned find_offset_in_vector( vcl_vector<Data> const& buff,
                                unsigned head, 
                                Data const & datum )
{
  typename vcl_vector<Data>::const_iterator iter = vcl_find( buff.begin(),
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
