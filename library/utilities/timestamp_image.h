/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_TIMESTAMP_IMAGE_H
#define INCL_TIMESTAMP_IMAGE_H

/// \brief Read and write vidtk::timestamps from images
///
/// Reading: timestamps are expected to be at the bottom of the images,
/// in a block which defaults to 16 pixels high.  The timestamp can be
/// optionally removed from the image; the stripped image is returned in
/// a separate image.  ("Old-style" direct encoding of the timeval struct
/// are detected and handled.)
///
/// Writing: timestamps are added at the bottom of the image, in a block
/// which defaults to 16 pixels high.  If add_padding is false, the
/// timecode overwrites the existing pixels and the image size is unchanged.
/// If add_padding is true, an appropriately-sized empty zone is added to
/// the bottom of the image and the barcode is written there.
///

#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>


namespace vidtk
{

  // Get the timestamp from the image.  Returns a default timestamp if
  // no timestamp can be found.

  template< typename T>
  timestamp
  get_timestamp( const vil_image_view<T> src_img,
		 int timestamp_rows_from_bottom = 16 );


  // Get the timestamp from the image.  If no timestamp is found, return
  // default timestamp and do nothing to dst_img.  Otherwise, copy src_img
  // to dst_img and optionally remove the timestamp from dst_img.

  template< typename T>
  timestamp
  get_timestamp( const vil_image_view<T> src_img,
		 vil_image_view<T>& dst_img,
		 int timestamp_rows_from_bottom = 16,
		 bool strip_timestamp = true );


  // Add the current time to the image; return the timestamped image
  // in dst_img.  Fail if the image is too narrow to hold the barcode.

  template< typename T>
  bool
  add_timestamp( const vil_image_view<T> src_img,
		 vil_image_view<T>& dst_img,
		 bool add_padding = true,
		 int timestamp_rows_from_bottom = 16 );


  // Add the given timestamp to the image; returned the timestamped image
  // in dst_img.  Fail if the image is too narrow to hold the barcode.

  template< typename T>
  bool
  add_timestamp( const vil_image_view<T> src_img,
		 vil_image_view<T>& dst_img,
		 const timestamp& ts,
		 bool add_padding = true,
		 int timestamp_rows_from_bottom = 16 );
}

#endif
