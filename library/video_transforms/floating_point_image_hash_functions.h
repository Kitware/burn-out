/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_floating_point_image_to_hash_
#define vidtk_floating_point_image_to_hash_


//  floating_point_image_hash_functions.h
//
//  Contains a number of functions for circumstances in which we
//  may want to convert some image containing floating or double precision
//  typed numbers into some integral quanity in another image. For example,
//  one application of this is when we put labels on intervals of doubles,
//  and create a new image containing the label of the pixel's original double value.
//  This can be used for optimization purposes in a few (limited) circumstances.


#include <cassert>
#include <vil/vil_image_view.h>

namespace vidtk
{

// Convert a floating point image to an intergral type by multiplying
// it by a scaling factor in addition to thresholding it in one operation.
// Performs rounding.
template< typename FloatType, typename IntType >
void scale_float_img_to_interval( const vil_image_view<FloatType>& src,
                                  vil_image_view<IntType>& dst,
                                  const FloatType& scale,
                                  const FloatType& max_input_value )
{
  // Resize, cast and copy
  unsigned ni = src.ni(), nj = src.nj(), np = src.nplanes();
  dst.set_size( ni, nj, np );

  std::ptrdiff_t sistep=src.istep(), sjstep=src.jstep(), spstep=src.planestep();
  std::ptrdiff_t distep=dst.istep(), djstep=dst.jstep(), dpstep=dst.planestep();

  IntType max_hashed_value = static_cast<IntType>( max_input_value * scale + 0.5 );

  const FloatType* splane = src.top_left_ptr();
  IntType* dplane = dst.top_left_ptr();
  for (unsigned p=0;p<np;++p,splane += spstep,dplane += dpstep)
  {
    const FloatType* srow = splane;
    IntType* drow = dplane;
    for (unsigned j=0;j<nj;++j,srow += sjstep,drow += djstep)
    {
      const FloatType* spixel = srow;
      IntType* dpixel = drow;
      for (unsigned i=0;i<ni;++i,spixel+=sistep,dpixel+=distep)
      {
        if( *spixel <= max_input_value )
        {
          *dpixel = static_cast<IntType>(*spixel * scale + 0.5);
        }
        else
        {
          *dpixel = max_hashed_value;
        }
      }
    }
  }
}

// Convert an integral value from the above operation back into its
// estimated original double form
template< typename FloatType, typename IntType >
FloatType convert_interval_scale_to_float( const IntType& hash, const FloatType& scale )
{
  assert( scale > 0 );
  return static_cast< FloatType >( hash ) / scale;
}


}

#endif // floating_point_image_to_hash_h_
