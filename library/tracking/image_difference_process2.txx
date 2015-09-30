/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "image_difference_process2.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vil/algo/vil_threshold.h>
#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <video/warp_image.h>
#include <video/jittered_image_difference.h>
#include <video/zscore_image.h>


namespace vidtk
{

template <class PixType>
image_difference_process2<PixType>
::image_difference_process2( vcl_string const& name )
  : process( name, "image_difference_process2" ),
    compute_unprocessed_mask_( false )
{
  config_.add_parameter(
    "z_threshold",
    "2.5",
    "The difference threshold at which a *z_score* difference is considered indicative "
    "of a object difference" );

  config_.add_parameter(
    "threshold",
    "20",
    "The difference threshold at which a difference is considered indicative "
    "of a object difference" );

  config_.add_parameter(
    "nan_value",
    "-1",
    "The special value used to indicate that a pixel is invalid (outside "
    "the valid region when warping, etc). If all the components of a pixel "
    "have this value, then the pixel is considered to be invalid." );

  config_.add_parameter(
    "jitter_delta",
    "0 0",
    "The image differencing will occur using these delta_i and delta_j "
    "jitter parameters.  The jitter essentially compensates for "
    "registration errors.  A jitter of \"1 2\" allows +/-1 pixel error "
    "in the i-axis and +/-2 pixel error in the j-axis.");

  config_.add_parameter( 
    "diff_type", 
    "absolute",
    "Choose between absolute, zscore, or adaptive_zscore differencing." );

  config_.add_parameter(
    "z_radius",
    "200",
    "Radius of the box in which to compute locally adaptive z-scores."
    "For each pixel the zscore statistics are computed seperately within a box"
    "that is 2*radius+1 on a side and centered on the pixel.");

  config_.add_parameter(
    "compute_unprocessed_mask",
    "false",
    "Whether to produce a valid unprocessed pixels mask or not. The mask "
    "records pixels (w/ true value) that are not supposed to produce valid "
    "motion segmentation." );
}


template <class PixType>
image_difference_process2<PixType>
::~image_difference_process2()
{
}


template <class PixType>
config_block
image_difference_process2<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
image_difference_process2<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "threshold", threshold_ );
    blk.get( "z_threshold", z_threshold_ );
    blk.get( "z_radius", z_radius_ );
    blk.get( "nan_value", nan_value_ );
    blk.get( "jitter_delta", jitter_delta_ );

    vcl_string diff_type;
    blk.get( "diff_type", diff_type );
    if( diff_type  == "absolute" )
    {
      diff_type_ =  DIFF_ABSOLUTE;
    }
    else if( diff_type == "zscore" )
    {
      diff_type_ = DIFF_ZSCORE;
    }
    else if( diff_type == "adaptive_zscore" )
    {
      diff_type_ = DIFF_ADAPTIVE_ZSCORE;
    }
    else
    {
      throw unchecked_return_value( "unknown image difference type \"" + diff_type + "\"" );
    }

    blk.get( "compute_unprocessed_mask", compute_unprocessed_mask_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
image_difference_process2<PixType>
::initialize()
{
  fg_image_is_valid_ = false;
  fg_image_zscore_is_valid_ = false;
  return true;
}


template <class PixType>
bool
image_difference_process2<PixType>
::reset()
{
  return initialize();
}

template <class PixType>
bool
image_difference_process2<PixType>
::step()
{
  // Check input vil_image_views
  if( !stab_frame_1_ || !stab_frame_2_ || !stab_frame_3_ ) {
    log_error("Empty frame inputted");
    return false;
  }

  // Check input sizes
  if( stab_frame_1_.ni() != stab_frame_2_.ni() ||
      stab_frame_1_.ni() != stab_frame_3_.ni() ||
      stab_frame_1_.nj() != stab_frame_2_.nj() ||
      stab_frame_1_.nj() != stab_frame_3_.nj() ||
      stab_frame_1_.nplanes() != stab_frame_2_.nplanes() ||
      stab_frame_1_.nplanes() != stab_frame_3_.nplanes() ) {

    log_error("Input sizes do not match");
    return false;
  }

  // Reset outputs so we can output using only shadow copies
  diff_image_ = vil_image_view< float >();
  fg_image_abs_ = vil_image_view< bool >();
  fg_image_zscore_ = vil_image_view< bool >();

  // Compute difference image and difference mask
  compute_difference_image( stab_frame_1_, stab_frame_2_, stab_frame_3_ );

  // Reset input images
  stab_frame_1_ = vil_image_view< PixType >();
  stab_frame_2_ = vil_image_view< PixType >();
  stab_frame_3_ = vil_image_view< PixType >();
  return true;
}

template < class PixType >
void
image_difference_process2<PixType >
::compute_zscore() const
{
  switch( diff_type_ )
  {
    case DIFF_ZSCORE:
    {
      // Threshold on demand
      if( ! fg_image_zscore_is_valid_ )
      {
        zscore_threshold_above( diff_image_, fg_image_zscore_, z_threshold_ );
        fg_image_zscore_is_valid_ = true;
      }
      break;
    }
    case DIFF_ADAPTIVE_ZSCORE:
    {
      // Threshold on demand
      if( ! fg_image_zscore_is_valid_ )
      {
        diff_zscore_.deep_copy(diff_image_);
        zscore_local_box( diff_zscore_, z_radius_ );
        vil_threshold_above( diff_zscore_, fg_image_zscore_, z_threshold_ );
        fg_image_zscore_is_valid_ = true;
      }
      break;
    }
    default:
      break;
  }
}


template < class PixType >
vil_image_view<bool> const&
image_difference_process2<PixType >
::fg_image() const
{
  if( diff_type_ == DIFF_ABSOLUTE )
  {
    // Threshold on demand
    if( ! fg_image_is_valid_ )
    {
      vil_threshold_above( diff_image_, fg_image_abs_, threshold_ );
      fg_image_is_valid_ = true;
    }
    return fg_image_abs_;
  }
  else
  {
    if( ! fg_image_zscore_is_valid_ )
    {
      compute_zscore();
    }
    return fg_image_zscore_;
  }
}


template < class PixType >
vil_image_view<bool> const&
image_difference_process2<PixType >
::fg_image_zscore() const
{
  // Threshold on demand
  if( ! fg_image_zscore_is_valid_ )
  {
    compute_zscore();
  }

  return fg_image_zscore_;
}


template < class PixType >
vil_image_view<float> const&
image_difference_process2<PixType >
::diff_image_zscore() const
{
  if( ! fg_image_zscore_is_valid_ )
  {
    compute_zscore();
  }

  return diff_zscore_;
}

template < class PixType >
vil_image_view<float> const&
image_difference_process2<PixType >
::diff_image() const
{
  return diff_image_;
}


template < class PixType >
void
image_difference_process2<PixType >
::set_first_frame( vil_image_view<PixType> const& image )
{
  stab_frame_1_ = image;
}

template < class PixType >
void
image_difference_process2<PixType >
::set_second_frame( vil_image_view<PixType> const& image )
{
  stab_frame_2_ = image;
}

template < class PixType >
void
image_difference_process2<PixType >
::set_third_frame( vil_image_view<PixType> const& image )
{
  stab_frame_3_ = image;
}

vil_image_view<vxl_byte> debug_image;

template < class PixType >
void
image_difference_process2<PixType >
::compute_difference_image( vil_image_view<PixType> const& A,
                            vil_image_view<PixType> const& B,
                            vil_image_view<PixType> const& C )
{
  assert( A.ni() == B.ni() && A.ni() == C.ni() );
  assert( A.nj() == B.nj() && A.nj() == C.nj() );
  assert( A.nplanes() == B.nplanes() && A.nplanes() == C.nplanes() );

  unsigned ni = A.ni();
  unsigned nj = A.nj();

  diff_image_.set_size( ni, nj );

  jittered_image_difference_params jp;
  jp.set_delta( jitter_delta_[0], jitter_delta_[1] );

  vil_image_view<float> CminusA;
  vil_image_view<float> CminusB;
  vil_image_view<float> AminusB;
  jittered_image_difference( C, A, CminusA, jp );
  jittered_image_difference( C, B, CminusB, jp );
  jittered_image_difference( A, B, AminusB, jp );

//   vil_image_view< vxl_byte > tmp;
//   vil_convert_stretch_range (CminusA, tmp);
//   vil_save( tmp, "CminusA.tiff" );
//   vil_convert_stretch_range (CminusB, tmp);
//   vil_save( tmp, "CminusB.tiff" );
//   vil_convert_stretch_range (AminusB, tmp);
//   vil_save( tmp, "AminusB.tiff" );

  vil_math_image_sum( CminusA, CminusB, diff_image_ );
//   vil_convert_stretch_range (diff_image_, tmp);
//   vil_save( tmp, "CminusApCminusB.tiff" );
  vil_math_image_abs_difference( diff_image_, AminusB, diff_image_ );
//   vil_convert_stretch_range (diff_image_, tmp);
//   vil_save( tmp, "diff_image_.tiff" );

  // Some pixels
  debug_image.set_size(ni,nj);
  debug_image.fill(0);
  suppress_nan_regions_from_diff( A );
//   vil_save( debug_image, "changed_pixels_a.tiff" );
  debug_image.set_size(ni,nj);
  debug_image.fill(0);
  suppress_nan_regions_from_diff( B );
//   vil_save( debug_image, "changed_pixels_b.tiff" );
//   vil_convert_stretch_range (diff_image_, tmp);
//   vil_save( tmp, "diff_image_no_nan.tiff" );

  // Mark the thresholded image as stale.
  fg_image_is_valid_ = false;
  fg_image_zscore_is_valid_ = false;
}



template < class PixType >
void
image_difference_process2<PixType >
::suppress_nan_regions_from_diff( vil_image_view<PixType> const& A )
{
  assert( A.ni() == diff_image_.ni() && A.nj() == diff_image_.nj() );

  unsigned ni = A.ni();
  unsigned nj = A.nj();
  unsigned np = A.nplanes();
  int const NaN = nan_value_;
  for( unsigned j = 0; j < nj; ++j )
  {
    for( unsigned i = 0; i < ni; ++i )
    {
      unsigned p;
      for( p = 0; p < np; ++p )
      {
        if( A(i,j,p) != NaN )
        {
          break;
        }
      }
      // if all the components == NaN, then the loop will reach the
      // end and p==np.
      if( p == np )
      {
        debug_image(i,j) = 255;
        diff_image_(i,j) = 0;
      }
    }
  }
}

template < class PixType >
vil_image_view<bool> const& 
image_difference_process2<PixType >
::unprocessed_mask() const
{
  return unprocessed_mask_;
}

} // end namespace vidtk
