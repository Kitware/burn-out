/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/barcode_factory.h>
#include <vcl_limits.h>

namespace
{

//
// Holds the properties of a barcode for a specific data type
// (in particular, the "on", "off", and "middle"xc values depend on the data type)
//

template <typename T>
struct barcode_properties
{
  int bpc;              // blocks per column
  int block_w, block_h; // block dimensions
  int start_row;        // vertical offset of barcode in image
  T on_val;             // value of an "on" barcode pixel
  T off_val;            // value of an "off" barcode pixel
  T middle_val;         // midpoint of image's pixel range

  barcode_properties( int b, int s, int w, int h,
		    double on_frac, double off_frac ):
    bpc(b), block_w(w), block_h(h), start_row(s)
  {
    T maxval = vcl_numeric_limits<T>::max();
    on_val = static_cast<T>( on_frac * maxval );
    off_val = static_cast<T>( off_frac * maxval );
    middle_val = static_cast<T>( 0.5 * maxval );
  }

  // given a bit index, numbered from the upper-left corner of the encoding
  // zone (not the data zone, but also including the check columns), return
  // a vector of alternating (i,j) coordinates for all the pixels of the
  // bit block for that bit index.

  vcl_vector<unsigned>
  get_bit_coordinates( int bit_index ) const
  {
    vcl_vector<unsigned> ij_vals;
    // how many columns in are we?
    int c_base = bit_index / this->bpc;
    // how many rows down are we?
    int r_base = bit_index - (c_base * this->bpc);
    // convert to pixels
    int i_base = c_base * this->block_w;
    int j_base = (r_base * this->block_h) + this->start_row;
    // compute (i,j) pairs
    for (int i=0; i<this->block_w; i++) {
      for (int j=0; j<this->block_h; j++) {
	ij_vals.push_back( i_base + i );
	ij_vals.push_back( j_base + j );
      }
    }

    return ij_vals;
  }
};

//
// write a bit into the image at the given bit index.
//

template< typename T >
void
write_bit( vil_image_view<T> img,
	   const barcode_properties<T>& props,
	   int bit_index,
	   int bit )
{
  T v = (bit) ? props.on_val : props.off_val;
  vcl_vector<unsigned> coords =
    props.get_bit_coordinates( bit_index );

  for (unsigned pix=0; pix<coords.size(); pix += 2) {
    for (unsigned p=0; p<img.nplanes(); p++) {
      img( coords[pix], coords[pix+1], p ) = v;
    }
  }
}

//
// read a bit from the image by summing up the values across the bit block
// and declaring the bit "on" if the average is above the middle value,
// off otherwise.
//

template< typename T >
bool
read_bit( const vil_image_view<T> img,
	  const barcode_properties<T>& props,
	  int bit_index )
{
  double sum_vals = 0.0;
  vcl_vector<unsigned> coords =
    props.get_bit_coordinates( bit_index );

  int c=0;
  for (unsigned pix=0; pix<coords.size(); pix += 2) {
    for (unsigned p=0; p<img.nplanes(); p++) {
      sum_vals += img( coords[pix], coords[pix+1], p );
      c++;
    }
  }

  return ( (sum_vals/c) > props.middle_val );
}


//
// Write a byte into the image, LSB first.
//

template< typename T >
void
write_byte( vil_image_view<T> img,
	    const barcode_properties<T>& props,
	    int& bit_index,
	    unsigned char data )
{

  int bit_mask = 1;
  for (unsigned i=0; i<8; i++) {
    int bit = data & bit_mask;
    write_bit( img, props, i+bit_index, bit );
    bit_mask *= 2;
  }
  bit_index += 8;
}

//
// Read a byte from the image, LSB first.
//

template< typename T >
unsigned char
read_byte( const vil_image_view<T> img,
	   const barcode_properties<T>& props,
	   int& bit_index )
{
  unsigned char byte = 0;
  int bit_mask = 1;
  for (unsigned i=0; i<8; i++) {
    bool bit = read_bit( img, props, i+bit_index );
    if (bit) byte += bit_mask;
    bit_mask *= 2;
  }
  bit_index += 8;
  return byte;
}

//
// Insert the given number of check columns.  Check columns are alternating
// columns of on/off pixels.
//

template< typename T >
void
insert_check_pattern( const vil_image_view<T> img,
		      const barcode_properties<T> props,
		      int ncols )
{
  int bit_index = 0;
  for (int c=0; c<ncols; c++) {
    bool bit_val = (c % 2 == 0);
    for (int r=0; r<props.bpc; r++) {
      write_bit( img, props, bit_index, bit_val );
      bit_index++;
      bit_val = !bit_val;
    }
  }
}

//
// Verify that we can find the check columns in the image.
//

template< typename T >
bool
verify_check_pattern( const vil_image_view<T> img,
		      const barcode_properties<T> props,
		      int col )
{
  int bit_index = (col * props.bpc);
  bool bit_val = (col % 2 == 0);
  for (int r=0; r<props.bpc; r++) {
    bool found_bit = read_bit( img, props, bit_index );
    if (found_bit != bit_val) return false;
    bit_index++;
    bit_val = !bit_val;
  }
  return true;
}

} // anonymous namespace


template< typename T >
bool
vidtk::barcode_factory::
encode( const vcl_vector<unsigned char>& data,
	vil_image_view<T> img,
	int start_row,
	int end_row ) const
{
  if (start_row == -1) start_row = 0;
  if (end_row == -1) end_row = img.nj()-1;

  int rows = end_row - start_row + 1;
  int bpc = rows / this->block_h_;    // blocks per column

  barcode_properties<T> props( bpc, start_row, this->block_w_, this->block_h_,
			       this->on_val_, this->off_val_ );

  // will we fit in the image?

  unsigned int nbits = (data.size()+1) * 8;    // +1 for data size byte
  unsigned int columns_needed = nbits / bpc;
  columns_needed += this->check_columns_;
  if ((columns_needed * this->block_w_) > img.ni()) return false;

  if (data.size() > 255) return false;

  // write the check pattern

  insert_check_pattern( img, props, this->check_columns_ );

  // write the data size

  int bit_index = this->check_columns_ * bpc;
  write_byte( img, props, bit_index, static_cast<unsigned char>( data.size() ));

  // write the data

  for (unsigned i=0; i<data.size(); i++) {
    write_byte( img, props, bit_index, data[i] );
  }

  return true;
}


template< typename T>
bool
vidtk::barcode_factory::
decode( const vil_image_view<T> img,
	vcl_vector<unsigned char>& data,
	int start_row,
	int end_row ) const
{
  if (start_row == -1) start_row = 0;
  if (end_row == -1) end_row = img.nj()-1;

  int rows = end_row - start_row + 1;
  int bpc = rows / this->block_h_;    // blocks per column

  barcode_properties<T> props( bpc, start_row, this->block_w_, this->block_h_,
			       this->on_val_, this->off_val_ );

  // verify the columns of the check pattern

  for (unsigned i=0; i<this->check_columns_; i++) {
    if (!verify_check_pattern( img, props, i )) return false;
  }

  // read the data size

  int bit_index = this->check_columns_ * bpc;
  int data_size = read_byte( img, props, bit_index );

  // read out the data

  data.clear();
  for (int i=0; i<data_size; i++) {
    data.push_back( read_byte( img, props, bit_index ));
  }

  return true;
}

