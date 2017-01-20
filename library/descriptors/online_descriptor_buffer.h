/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_online_descriptor_buffer_h_
#define vidtk_online_descriptor_buffer_h_

#include <tracking_data/frame_data.h>

#include <boost/circular_buffer.hpp>

namespace vidtk
{

/// Internal frame buffer used for online_descriptor_generators.
class online_descriptor_buffer
{
public:

  explicit online_descriptor_buffer( unsigned buffer_capacity );
  virtual ~online_descriptor_buffer() {}

  /// Size of the buffer.
  virtual unsigned size() const;

  /// Get a frame at a specified index where 0 is the oldest frame in the buffer.
  virtual frame_data_sptr const& get_frame( unsigned index ) const;

  /// Insert a frame into the buffer.
  virtual void insert_frame( const frame_data_sptr& frame );

private:

  // Internal circular buffer
  boost::circular_buffer< frame_data_sptr > data_;

};


} // end namespace vidtk

#endif
