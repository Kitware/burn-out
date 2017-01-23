/*ckwg +5
 * Copyright 2010-2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/queue_process.txx>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <limits>
#include <tracking_data/shot_break_flags.h>


template class vidtk::queue_process< vidtk::timestamp >;
template class vidtk::queue_process< vil_image_view<vxl_byte> >;
template class vidtk::queue_process< vil_image_view<vxl_uint_16> >;
template class vidtk::queue_process< vgl_h_matrix_2d<double> >;
template class vidtk::queue_process< vidtk::image_to_image_homography >;
template class vidtk::queue_process< vil_image_view<bool> >;
template class vidtk::queue_process< vidtk::shot_break_flags >;

namespace vidtk
{

// Define specializations of offset_of for types we can handle.

template <>
unsigned
queue_process< vidtk::timestamp >
::index_nearest_to( vidtk::timestamp const & D ) const
{
  unsigned nearest_index = unsigned(-1);
  double min_distance = std::numeric_limits<double>::infinity();

  for( unsigned i = 0; i < queue_.size(); i++ )
  {
    if( queue_[i].has_time() && D.has_time() )
    {
      double distance = std::abs(queue_[i].time() - D.time());
      if( distance < min_distance )
      {
        nearest_index = i;
        min_distance = distance;
      }
    }
    else
    {
      LOG_ERROR( this->name() << " : Found inconsistent timestamp while searching." );
      return unsigned(-1);
    }
  }

  return  nearest_index;
}

} // namespace vidtk
