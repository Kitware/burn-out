/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "three_frame_differencing.h"

#include <vil/algo/vil_threshold.h>
#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <vil/vil_transform.h>

#include <video_transforms/warp_image.h>
#include <video_transforms/jittered_image_difference.h>
#include <video_transforms/zscore_image.h>

#include <boost/thread/thread.hpp>

#include <vnl/vnl_int_2.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "three_frame_differencing_txx" );


template <typename InputType, typename OutputType>
bool
three_frame_differencing<InputType, OutputType>
::configure( const settings_t& settings )
{
  // Copy internal settings
  settings_ = settings;

  // If using some for of zscoring thresholding...
  if( settings_.diff_type != settings_t::ABSOLUTE )
  {
    if( settings_.z_threshold <= 0 )
    {
      LOG_ERROR( "Z threshold must be > 0!" );
      return false;
    }

    // Refine the min_zscore_stdev parameter based on the value of
    // min_pixel_difference_for_z_score. These two parameters are
    // redundant and one should be removed. They are both left in
    // for now due to backwards compatibility issues.
    if( settings_.min_pixel_difference_for_z_score > 0 )
    {
      float min_zscore_stdev_for_diff =
        settings_.min_pixel_difference_for_z_score / settings.z_threshold;

      if( settings_.min_zscore_stdev < min_zscore_stdev_for_diff )
      {
        settings_.min_zscore_stdev = min_zscore_stdev_for_diff;
      }
    }
  }

  return true;
}

template <typename InputType, typename OutputType>
bool
three_frame_differencing<InputType, OutputType>
::process_frames( const input_image_t& image1,
                  const input_image_t& image2,
                  const input_image_t& image3,
                  mask_image_t& fg_image )
{
  return process_frames( image1, image2, image3, fg_image, tmp_image1_ );
}

template <typename InputType, typename OutputType>
bool
three_frame_differencing<InputType, OutputType>
::process_frames( const input_image_t& image1,
                  const input_image_t& image2,
                  const input_image_t& image3,
                  diff_image_t& diff_image )
{
  // Check input vil_image_views
  if( !image1 || !image2 || !image3 )
  {
    LOG_ERROR( "Empty frame inputted!" );
    return false;
  }

  // Check input sizes
  if( image1.nplanes() != image2.nplanes() ||
      image1.nplanes() != image3.nplanes() ||
      image1.ni() != image2.ni() || image1.ni() != image3.ni() ||
      image1.nj() != image2.nj() || image1.nj() != image3.nj() )
  {
    LOG_ERROR( "Input dimensions do not match!" );
    return false;
  }

  // Perform differencing
  switch( settings_.operation_type )
  {
    case settings_t::UNSIGNED_ADAPTIVE_ZSCORE_SUM:
    case settings_t::UNSIGNED_ZSCORE_SUM:
    case settings_t::UNSIGNED_SUM:
    {
      unsigned_sum_diff( image1, image2, image3, diff_image );
      break;
    }
    case settings_t::SIGNED_SUM:
    {
      signed_sum_diff( image1, image2, image3, diff_image );
      break;
    }
    case settings_t::UNSIGNED_ADAPTIVE_ZSCORE_MIN:
    case settings_t::UNSIGNED_MIN:
    {
      unsigned_min_diff( image1, image2, image3, diff_image );
      break;
    }
    default:
    {
      LOG_ERROR( "Invalid differencing operator type received!" );
      return false;
    }
  }

  return true;
}

template <typename InputType, typename OutputType>
bool
three_frame_differencing<InputType, OutputType>
::process_frames( const input_image_t& image1,
                  const input_image_t& image2,
                  const input_image_t& image3,
                  mask_image_t& fg_image,
                  diff_image_t& diff_image )
{
  // Compute diff image
  if( !process_frames( image1, image2, image3, diff_image ) )
  {
    LOG_ERROR( "Unable to compute difference image!" );
    return false;
  }

  // Threshold diff image
  if( settings_.diff_type == settings_t::ABSOLUTE )
  {
    compute_absolute_fg_mask( diff_image, fg_image );
  }
  else
  {
    compute_zscore_fg_mask( diff_image, fg_image );
  }

  return true;
}

template< typename FloatType >
FloatType abs_value_functor( FloatType v )
{
  return std::fabs( v );
}

template < typename InputType, typename OutputType >
void
three_frame_differencing<InputType, OutputType>
::compute_absolute_fg_mask( const diff_image_t& diff_image,
                            mask_image_t& fg_image )
{
  if( settings_.operation_type == settings_t::SIGNED_SUM )
  {
    vil_threshold_outside( diff_image, fg_image, -settings_.threshold, settings_.threshold );
  }
  else
  {
    vil_threshold_above( diff_image, fg_image, settings_.threshold );
  }
}

template < typename InputType, typename OutputType >
void
three_frame_differencing<InputType, OutputType>
::compute_zscore_fg_mask( diff_image_t& diff_image,
                          mask_image_t& fg_image )
{
  float min_zscore_stdev;

  switch( settings_.operation_type )
  {
    case settings_t::UNSIGNED_ZSCORE_SUM:
    case settings_t::UNSIGNED_ADAPTIVE_ZSCORE_SUM:
    case settings_t::UNSIGNED_ADAPTIVE_ZSCORE_MIN:
    {
      min_zscore_stdev = 0;
      break;
    }
    default:
    {
      min_zscore_stdev = settings_.min_zscore_stdev;
      break;
    }
  }

  switch( settings_.operation_type )
  {
    case settings_t::SIGNED_SUM:
    {
      switch( settings_.diff_type )
      {
        case settings_t::ZSCORE:
        {
          zscore_threshold_outside( diff_image,
                                    fg_image,
                                    -settings_.z_threshold,
                                    settings_.z_threshold,
                                    settings_.subsample_step,
                                    min_zscore_stdev );
          break;
        }
        case settings_t::ADAPTIVE_ZSCORE:
        {
          zscore_local_box( diff_image,
                            static_cast<unsigned>( settings_.z_radius ),
                            min_zscore_stdev );

          vil_threshold_outside( diff_image,
                                 fg_image,
                                 -settings_.z_threshold,
                                 settings_.z_threshold );
          break;
        }
        default:
        {
          break;
        }
      }
      break;
    }
    default:
    {
      switch( settings_.diff_type )
      {
        case settings_t::ZSCORE:
        {
          zscore_threshold_above( diff_image,
                                  fg_image,
                                  settings_.z_threshold,
                                  settings_.subsample_step,
                                  min_zscore_stdev );
          break;
        }
        case settings_t::ADAPTIVE_ZSCORE:
        {
          zscore_local_box( diff_image,
                            static_cast<unsigned>( settings_.z_radius ),
                            min_zscore_stdev );

          vil_threshold_above( diff_image, fg_image, settings_.z_threshold );
          break;
        }
        default:
        {
          break;
        }
      }
      break;
    }
  }
}

#ifdef TFD_FRAME_DEBUGGING
vil_image_view<vxl_byte> debug_image;
#endif

template < typename InputType, typename OutputType >
void
three_frame_differencing<InputType, OutputType>
::suppress_nan_regions_from_diff( const input_image_t& image,
                                  diff_image_t& diff_image )
{
  unsigned ni = image.ni();
  unsigned nj = image.nj();
  unsigned np = image.nplanes();

  int const NaN = settings_.nan_value;

  for( unsigned j = 0; j < nj; ++j )
  {
    for( unsigned i = 0; i < ni; ++i )
    {
      unsigned p;

      for( p = 0; p < np; ++p )
      {
        if( image(i,j,p) != NaN )
        {
          break;
        }
      }

      // if all the components == NaN, then the loop will reach the
      // end and p==np.
      if( p == np )
      {
#ifdef TFD_FRAME_DEBUGGING
        debug_image(i,j) = 255;
#endif
        diff_image(i,j) = 0;
      }
    }
  }
}

template < typename InputType, typename OutputType >
void
three_frame_differencing<InputType, OutputType>
::unsigned_sum_diff( const input_image_t& A,
                     const input_image_t& B,
                     const input_image_t& C,
                     diff_image_t& diff_image )
{
  const unsigned ni = A.ni();
  const unsigned nj = A.nj();

  diff_image.set_size( ni, nj );

  jittered_image_difference_params jp;
  jp.set_delta( settings_.jitter_delta[0], settings_.jitter_delta[1] );

  if( ( settings_.operation_type == settings_t::UNSIGNED_ZSCORE_SUM ) ||
      ( settings_.operation_type == settings_t::UNSIGNED_ADAPTIVE_ZSCORE_SUM ) )
  {
    jp.set_signed_flag( true );
  }

  // NOTE: please respect the order in which the subtraction is done. We found
  // when jitter_delta is not zero, incorrect order produces ghosts. Detailed
  // reason is too long to put here. The order below is investigated and tested.
  vil_image_view<OutputType> AminusC;
  vil_image_view<OutputType> CminusB;
  vil_image_view<OutputType> AminusB;

  if( settings_.use_threads )
  {
    boost::thread wthread0( &jittered_image_difference2<InputType,OutputType>, &A, &C, &AminusC, &jp );
    boost::thread wthread1( &jittered_image_difference2<InputType,OutputType>, &C, &B, &CminusB, &jp );

    jittered_image_difference( A, B, AminusB, jp );

    wthread0.join();
    wthread1.join();
  }
  else
  {
    jittered_image_difference( A, C, AminusC, jp );
    jittered_image_difference( C, B, CminusB, jp );
    jittered_image_difference( A, B, AminusB, jp );
  }

  if( settings_.operation_type == settings_t::UNSIGNED_ADAPTIVE_ZSCORE_SUM )
  {
    zscore_local_box( AminusC, settings_.z_radius, settings_.min_zscore_stdev );
    zscore_local_box( CminusB, settings_.z_radius, settings_.min_zscore_stdev );
    zscore_local_box( AminusB, settings_.z_radius, settings_.min_zscore_stdev );
  }
  else if( settings_.operation_type == settings_t::UNSIGNED_ZSCORE_SUM )
  {
    zscore_global( AminusC, settings_.subsample_step, settings_.min_zscore_stdev );
    zscore_global( CminusB, settings_.subsample_step, settings_.min_zscore_stdev );
    zscore_global( AminusB, settings_.subsample_step, settings_.min_zscore_stdev );
  }

  if( jp.signed_diff_ )
  {
    vil_transform( AminusC, abs_value_functor<OutputType> );
    vil_transform( CminusB, abs_value_functor<OutputType> );
    vil_transform( AminusB, abs_value_functor<OutputType> );
  }

#ifdef TFD_FRAME_DEBUGGING
  vil_save( A, "A.png" );
  vil_save( B, "B.png" );
  vil_save( C, "C.png" );
  vil_image_view< vxl_byte > tmp;
  vil_convert_stretch_range( AminusC, tmp );
  vil_save( tmp, "AminusC.tiff" );
  vil_convert_stretch_range( CminusB, tmp );
  vil_save( tmp, "CminusB.tiff" );
  vil_convert_stretch_range( AminusB, tmp );
  vil_save( tmp, "AminusB.tiff" );
#endif

  vil_math_image_sum( AminusC, CminusB, diff_image );

#ifdef TFD_FRAME_DEBUGGING
  vil_convert_stretch_range( diff_image, tmp );
  vil_save( tmp, "AminusCpCminusB.tiff" );
#endif

  vil_math_image_abs_difference( diff_image, AminusB, diff_image );

#ifdef TFD_FRAME_DEBUGGING
  vil_convert_stretch_range( diff_image, tmp );
  vil_save( tmp, "diff_image.tiff" );
#endif

  // Some pixels
#ifdef TFD_FRAME_DEBUGGING
  debug_image.set_size( ni, nj );
  debug_image.fill( 0 );
#endif

  suppress_nan_regions_from_diff( A, diff_image );

#ifdef TFD_FRAME_DEBUGGING
  vil_save( debug_image, "changed_pixels_a.tiff" );
  debug_image.set_size( ni, nj );
  debug_image.fill( 0 );
#endif

  suppress_nan_regions_from_diff( B, diff_image );

#ifdef TFD_FRAME_DEBUGGING
  vil_save( debug_image, "changed_pixels_b.tiff" );
  vil_convert_stretch_range (diff_image, tmp);
  vil_save( tmp, "diff_imageno_nan.tiff" );
#endif
}

template < typename InputType, typename OutputType >
void
three_frame_differencing<InputType, OutputType>
::signed_sum_diff( const input_image_t& A,
                   const input_image_t& B,
                   const input_image_t& C,
                   diff_image_t& diff_image )
{
  const unsigned ni = A.ni();
  const unsigned nj = A.nj();

  diff_image.set_size( ni, nj );

  jittered_image_difference_params jps, jpu;

  jps.set_delta( settings_.jitter_delta[0], settings_.jitter_delta[1] );
  jps.set_signed_flag( true );

  jpu.set_delta( settings_.jitter_delta[0], settings_.jitter_delta[1] );
  jpu.set_signed_flag( false );

  // NOTE: please respect the order in which the subtraction is done. We found
  // when jitter_delta is not zero, incorrect order produces ghosts. Detailed
  // reason is too long to put here. The order below is investigated and tested.
  vil_image_view<OutputType> CminusA;
  vil_image_view<OutputType> CminusB;
  vil_image_view<OutputType> AminusB;

  if( settings_.use_threads )
  {
    boost::thread wthread0( &jittered_image_difference2<InputType,OutputType>, &C, &A, &CminusA, &jps );
    boost::thread wthread1( &jittered_image_difference2<InputType,OutputType>, &C, &B, &CminusB, &jpu );

    jittered_image_difference( A, B, AminusB, jpu );

    wthread0.join();
    wthread1.join();
  }
  else
  {
    jittered_image_difference( C, A, CminusA, jps );
    jittered_image_difference( C, B, CminusB, jpu );
    jittered_image_difference( A, B, AminusB, jpu );
  }

  // X = ( C - A ) + | C - B | - | A - B |
  vil_math_image_sum( CminusA, CminusB, tmp_image1_ );
  vil_math_image_difference( tmp_image1_, AminusB, tmp_image1_ );

  // Y = ( C - A ) - | C - B | + | A - B |
  vil_math_image_difference( CminusA, CminusB, tmp_image2_ );
  vil_math_image_sum( tmp_image2_, AminusB, tmp_image2_ );

  // DIFF = ( Y + X - |Y| + |X| )
  vil_math_image_sum( tmp_image1_, tmp_image2_, diff_image );

  vil_transform( tmp_image1_, abs_value_functor<OutputType> );
  vil_transform( tmp_image2_, abs_value_functor<OutputType> );

  vil_math_image_difference( tmp_image1_, tmp_image2_, tmp_image1_ );

  vil_math_image_sum( diff_image, tmp_image1_, diff_image );

  suppress_nan_regions_from_diff( A, diff_image );
  suppress_nan_regions_from_diff( B, diff_image );
}

template < typename InputType, typename OutputType >
void
three_frame_differencing<InputType, OutputType>
::unsigned_min_diff( const input_image_t& A,
                     const input_image_t& B,
                     const input_image_t& C,
                     diff_image_t& diff_image )
{
  const unsigned ni = A.ni();
  const unsigned nj = A.nj();

  const bool zscore_filter = ( settings_.operation_type == settings_t::UNSIGNED_ADAPTIVE_ZSCORE_MIN );

  diff_image.set_size( ni, nj );

  jittered_image_difference_params jp;
  jp.set_delta( settings_.jitter_delta[0], settings_.jitter_delta[1] );
  jp.set_signed_flag( zscore_filter );

  vil_image_view<OutputType> CminusB;
  vil_image_view<OutputType> CminusA;

  if( settings_.use_threads )
  {
    boost::thread wthread0( &jittered_image_difference2<InputType,OutputType>, &C, &A, &CminusA, &jp );

    jittered_image_difference( C, B, CminusB, jp );

    wthread0.join();
  }
  else
  {
    jittered_image_difference( C, A, CminusA, jp );
    jittered_image_difference( C, B, CminusB, jp );
  }

  if( zscore_filter )
  {
    zscore_local_box( CminusA, settings_.z_radius, settings_.min_zscore_stdev );
    zscore_local_box( CminusB, settings_.z_radius, settings_.min_zscore_stdev );

    vil_transform( CminusA, abs_value_functor<OutputType> );
    vil_transform( CminusB, abs_value_functor<OutputType> );
  }

#ifdef TFD_FRAME_DEBUGGING
  vil_save( A, "A.png" );
  vil_save( B, "B.png" );
  vil_save( C, "C.png" );
  vil_image_view< vxl_byte > tmp;
  vil_convert_stretch_range( CminusA, tmp );
  vil_save( tmp, "CminusA.tiff" );
  vil_convert_stretch_range( CminusB, tmp );
  vil_save( tmp, "CminusB.tiff" );
#endif

  vil_math_image_min( CminusA, CminusB, diff_image );

  suppress_nan_regions_from_diff( A, diff_image );
  suppress_nan_regions_from_diff( B, diff_image );
}

} // end namespace vidtk
