/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "online_descriptor_buffer.h"

namespace vidtk
{

online_descriptor_buffer
::online_descriptor_buffer( unsigned buffer_capacity )
{
  data_.set_capacity( buffer_capacity );
}

unsigned
online_descriptor_buffer
::size() const
{
  return data_.size();
}

frame_data_sptr const&
online_descriptor_buffer
::get_frame( unsigned index ) const
{
  return data_[ index ];
}

void
online_descriptor_buffer
::insert_frame( const frame_data_sptr& frame )
{
  data_.push_back( frame );
}

} // end namespace vidtk
