/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "jittered_image_difference.h"

#include <cassert>
#include <cmath>
#include <algorithm>

#include <vil/vil_crop.h>
#include <vil/vil_math.h>

namespace vidtk
{

// Functor for merging jittered difference images when the inputs are
// already assumed to be unsigned.
template<class AccumT>
AccumT min_float_unsigned( AccumT a, AccumT b )
{
  return a < b ? a : b;
}

// Functor for merging jittered difference images when the inputs are
// possibly signed. Selects the response with the lowest magnitude.
template<class AccumT>
AccumT min_float_signed( AccumT a, AccumT b )
{
  return std::fabs( a ) < std::fabs( b ) ? a : b;
}

// Take the maximum value over several input planes.
template<class aT, class maxT>
void math_max_over_planes( const vil_image_view<aT>& src,
                           vil_image_view<maxT>& dest )
{
  if( src.nplanes() == 1 && src.is_a() == dest.is_a() )
  {
    dest.deep_copy( src );
    return;
  }

  dest.set_size( src.ni(), src.nj(), 1 );

  for( unsigned j=0; j < src.nj(); ++j )
  {
    for( unsigned i=0; i < src.ni(); ++i )
    {
      maxT max = static_cast<maxT>( 0 );
      for( unsigned p=0; p < src.nplanes(); ++p )
      {
        if( src(i,j,p) > max )
        {
          max = static_cast<maxT>( src(i,j,p) );
        }
      }

      dest(i,j) = max;
    }
  }
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
    vil_image_view<AccumT> working;

    if( param.signed_diff_ )
    {
      vil_math_image_difference( srcA, srcB, working );
    }
    else
    {
      vil_math_image_abs_difference( srcA, srcB, working );
    }

    if( srcA.nplanes() == 1 )
    {
      diff = working;
    }
    else
    {
      math_max_over_planes( working, diff );
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

      if( param.signed_diff_ )
      {
        vil_math_image_difference( cropA, cropB, working2 );
      }
      else
      {
        vil_math_image_abs_difference( cropA, cropB, working2 );
      }

      if( working2.nplanes() == 1 )
      {
        working = working2;
      }
      else
      {
        math_max_over_planes( working2, working );
      }

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

        if( param.signed_diff_ )
        {
          std::transform( working.begin(),
                          working.end(),
                          cropD.begin(),
                          cropD.begin(),
                          min_float_signed<AccumT> );
        }
        else
        {
          std::transform( working.begin(),
                          working.end(),
                          cropD.begin(),
                          cropD.begin(),
                          min_float_unsigned<AccumT> );
        }
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
