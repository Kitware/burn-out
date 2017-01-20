/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_BARCODE_FACTORY_H
#define INCL_BARCODE_FACTORY_H

/// \brief Encode/decode unsigned chars into/out of an image.
///
/// The class is initialized with the dimensions (HxW) of the blocks to use
/// to build the barcode; it can then {encode, decode} vectors of unsigned char
/// {into, out from} vil_image_views.
///
/// Data is encoded top-down, left-to-right (assuming UL origin) as
/// (a) a set of columns of alternating on/off patterns (the "check columns")
/// (b) the length of the data (0-255)
/// (c) the data
///
/// (b) and (c) are encoded LSB-first.
///
/// Encoding fails if the data is too long or the image is too narrow to
/// hold all the bits.
///
/// Decoding fails if it can't find the check columns.
///
/// It would be cool to integrate this somehow into vsl.
///

#include <vector>
#include <vil/vil_image_view.h>

namespace vidtk
{

class barcode_factory
{
public:

  barcode_factory( int block_h = 4,          // height in pixels of bit block
                   int block_w = 4,          // width in pixels of bit block
                   int check_c = 2,          // columns of check pattern bits
                   double off_val = 0.2,     // "off" value (% of max value)
                   double on_val = 0.8 ):    // "on" value (% of max value)
    block_h_( block_h ), block_w_( block_w ), check_columns_( check_c ),
    off_val_( off_val ), on_val_( on_val )
  {}
  ~barcode_factory() {}

  template< typename T >
  bool encode( const std::vector<unsigned char>& data,
               vil_image_view<T> img,
               int start_row = -1,          // -1 defaults to first row
               int end_row = -1 ) const;    // -1 defaults to last row


  template< typename T >
  bool decode( const vil_image_view<T> img,
               std::vector<unsigned char>& data,
               int start_row = -1,          // -1 defaults to first row
               int end_row = -1 ) const;    // -1 defaults to last row


private:

  unsigned int block_h_, block_w_;  // pixel dimensions of a bit block
  unsigned int check_columns_;      // how many check pattern columns
  double off_val_, on_val_;         // percentage of max val for off and off


  barcode_factory( const barcode_factory& ); // no cpctor
  barcode_factory& operator=( const barcode_factory& ); // no op=

};

} // namespace vidtk

#endif
