/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/timestamp_image.h>

#include <utilities/barcode_factory.h>
#include <vil/vil_image_view.h>
#include <vil/vil_crop.h>
#include <cstring> // memcpy
#include <cassert>
#include <vxl_config.h>
#include <logger/logger.h>

// implementation 0 means unimplemented.
// implementation 1 uses Unix style time.h and sys/time.h
#if defined(VCL_VC) || defined(WIN32)
# define IMPLEMENTATION 0
#else
# include <sys/time.h>
# include <time.h>
# define IMPLEMENTATION 1
#endif

VIDTK_LOGGER("timestamp_image_txx");

using vidtk::timestamp;

template< typename T>
timestamp
vidtk::get_timestamp( const vil_image_view<T> src_img,
                      int n_ts_rows )
{
  // bail for now on different architectures
  assert( sizeof(double)==8 );
  assert( VXL_LITTLE_ENDIAN );

  barcode_factory b;
  std::vector< unsigned char > td;

  // is there any data at all in the image?

  if (!b.decode( src_img, td, src_img.nj() - n_ts_rows )) {
    return timestamp();  // no data-- invalid timestamp
  }

  // does the decoded data fit a current-style timestamp?

  if (td.size() == sizeof(double)) {
    double t;
    std::memcpy( &t, &(td[0]), sizeof( t ) );
    return timestamp( t );
  }

#if IMPLEMENTATION == 1

  //
  // unfortunately, the first round of ayrshire/raeko/hdprint webcams
  // were stamped directly with the sec / usec fields of a timeval struct.
  // If the data doesn't fit a double, try it as this legacy case.
  //

  timeval tv;
  if (td.size() == sizeof(tv.tv_sec)+sizeof(tv.tv_usec)) {
    int index = 0;
    std::memcpy( &tv.tv_sec, &(td[index]), sizeof(tv.tv_sec) );
    index += sizeof(tv.tv_sec);
    std::memcpy( &tv.tv_usec, &(td[index]), sizeof(tv.tv_usec) );
    static bool emit_msg = true;
    if (emit_msg) {
      LOG_INFO( "get_timestamp: old-style timestamp detected");
      emit_msg = false;
    }

    return timestamp( (tv.tv_sec * 1.0e06) + tv.tv_usec );
  }

#endif

  // otherwise, it's not a timestamp.. return invalid timestamp
  return timestamp();

}


template< typename T>
timestamp
vidtk::get_timestamp( const vil_image_view<T> src_img,
                      vil_image_view<T>& dst_img,
                      int n_ts_rows,
                      bool strip_timestamp )
{

  // get the timestamp, exit without setting dst_img if no timestamp found

  timestamp ts = get_timestamp( src_img, n_ts_rows );
  if (!ts.has_time()) return ts;

  // if requested, strip the timestamp out of dst_img; otherwise, copy

  if (strip_timestamp) {
    if (dst_img.nj() != dst_img.ni() - n_ts_rows) {
      dst_img.set_size( src_img.ni(),
                        src_img.nj()-n_ts_rows,
                        src_img.nplanes() );
    }
    dst_img = vil_crop( src_img, 0, dst_img.ni(), 0, dst_img.nj() );
  } else {
    dst_img.deep_copy( src_img );
  }

  // all done

  return ts;
}


template< typename T>
bool
vidtk::add_timestamp( const vil_image_view<T> src_img,
                      vil_image_view<T>& dst_img,
                      bool add_padding,
                      int n_ts_rows )
{
#if IMPLEMENTATION == 1

  // set the timestamp to now

  timeval tv;
  if (gettimeofday( &tv, 0 )) return false;
  double t = (tv.tv_sec * 1.0e6) + tv.tv_usec;
  timestamp ts( t );
  return add_timestamp( src_img, dst_img, ts, add_padding, n_ts_rows );

#else

  LOG_ERROR( "Not implemented for this platform" );
  return false;

#endif
}


template< typename T>
bool
vidtk::add_timestamp( const vil_image_view<T> src_img,
                      vil_image_view<T>& dst_img,
                      const timestamp& ts,
                      bool add_padding,
                      int n_ts_rows )
{
  // bail for now on different architectures
  assert( sizeof(double)==8 );
  assert( VXL_LITTLE_ENDIAN );

  // if requested, add empty rows to hold the barcode; otherwise, copy

  if (add_padding) {
    if (dst_img.nj() != src_img.nj()+n_ts_rows) {
      dst_img.set_size( src_img.ni(),
                        src_img.nj()+n_ts_rows,
                        src_img.nplanes() );
      dst_img.fill(0);
    }
    vil_image_view<T> dst_crop =
      vil_crop( dst_img, 0, src_img.ni(), 0, src_img.nj() );
    dst_crop.deep_copy( src_img );
  } else {
    dst_img.deep_copy( src_img );
  }

  // convert the timestamp time double to chars

  double t = ts.time();

  std::vector<unsigned char> td( sizeof(t) );
  std::memcpy( &(td[0]), &t, sizeof(t) );

  // encode and return

  barcode_factory b;
  return b.encode( td, dst_img, dst_img.nj() - n_ts_rows );
}

#undef IMPLEMENTATION
