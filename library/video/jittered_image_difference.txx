/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "jittered_image_difference.h"
#include <vcl_cassert.h>
#include <vcl_algorithm.h>
#include <vil/vil_crop.h>
#include <vil/vil_math.h>

namespace vidtk
{

inline
float min_float(float a, float b)
{
  return a < b ? a : b;
}

template<class SrcT, class AccumT>
void
jittered_image_difference(
  vil_image_view<SrcT> const& srcA,
  vil_image_view<SrcT> const& srcB,
  vil_image_view<AccumT>& diff,
  jittered_image_difference_params const& param )
{
  assert( srcA.ni() == srcB.ni() );
  assert( srcA.nj() == srcB.nj() );
  assert( srcA.nplanes() == srcB.nplanes() );
  assert( srcA.ni() >= 2*param.delta_i_+1 );
  assert( srcA.nj() >= 2*param.delta_j_+1 );

  diff.set_size( srcA.ni(), srcA.nj() );

  if( param.delta_i_ == 0 && param.delta_j_ == 0 )
  {
    // when jitter values are zero this function is just an image difference
    // followed by 3 unnecessary deep copies.  It is more effecient to
    // bypass the deep copies when the are not needed.
    if( srcA.nplanes() == 1 )
    {
      vil_math_image_abs_difference( srcA, srcB, diff );
    }
    else
    {
      vil_image_view<AccumT> working;
      vil_math_image_abs_difference( srcA, srcB, working );
      vil_math_mean_over_planes( working, diff );
    }
    return;
  }

  diff.fill( 0 );

  unsigned const crop_ni = srcA.ni() - 2*param.delta_i_-1;
  unsigned const crop_nj = srcA.nj() - 2*param.delta_j_-1;

  vil_image_view<SrcT> cropA = vil_crop( srcA,
                                         param.delta_i_, crop_ni,
                                         param.delta_j_, crop_nj );


  vil_image_view<AccumT> cropD;
  cropD.set_size( crop_ni, crop_nj );

  vil_image_view<AccumT> working;
  working.set_size( crop_ni, crop_nj );

  vil_image_view<AccumT> working2;
  bool first_time = true;

  for( unsigned dj = 0; dj <= 2*param.delta_j_; ++dj )
  {
    for( unsigned di = 0; di <= 2*param.delta_i_; ++di )
    {
      vil_image_view<SrcT> cropB = vil_crop( srcB,
                                             di, crop_ni,
                                             dj, crop_nj );
      vil_math_image_abs_difference( cropA, cropB, working2 );
      vil_math_mean_over_planes( working2, working );
      if( first_time )
      {
        cropD.deep_copy( working );
        first_time = false;
      }
      else
      {
        assert( working.size() == cropD.size() );
        assert( working.is_contiguous() );
        assert( cropD.is_contiguous() );
        assert( cropD.istep() == working.istep() );
        vcl_transform( working.begin(), working.end(),
                       cropD.begin(),
                       cropD.begin(),
                       min_float );
      }
    }
  }

  // Copy the result into the output image
  vil_crop( diff,
            param.delta_i_, crop_ni,
            param.delta_j_, crop_nj ).deep_copy( cropD );

}


} // end namespace vidtk


#define INSTANTIATE_JITTERED_IMAGE( SrcT, AccumT )                      \
  template                                                              \
  void vidtk::jittered_image_difference(                                \
    vil_image_view< SrcT > const& srcA,                                 \
    vil_image_view< SrcT > const& srcB,                                 \
    vil_image_view< AccumT >& diff,                                     \
    vidtk::jittered_image_difference_params const& param )
