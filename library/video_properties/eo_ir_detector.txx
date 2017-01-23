/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "eo_ir_detector.h"

#include <utilities/point_view_to_region.h>

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <cstdlib>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER("eo_ir_detector_txx");


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
template < class PixType >
eo_ir_detector< PixType >
::eo_ir_detector()
{
  this->configure( eo_ir_detector_settings() );
}


template < class PixType >
eo_ir_detector< PixType >
::eo_ir_detector( eo_ir_detector_settings const& settings )
{
  this->configure( settings );
}


// ----------------------------------------------------------------
/** Configure with new parameters.
 *
 *
 */
template < class PixType >
bool
eo_ir_detector< PixType >
::configure( eo_ir_detector_settings const& settings )
{
  if( settings.eo_multiplier <= 0.0f )
  {
    LOG_ERROR( "EO multiplier must be greater than 0" );
    return false;
  }

  settings_ = settings;
  return true;
}


// ----------------------------------------------------------------
/** Is the image an IR image?
 *
 *
 */
template < class PixType >
bool
eo_ir_detector< PixType >
::is_ir( vil_image_view< PixType > const& img ) const
{
  if( img.nplanes() != 3 )
  {
    LOG_WARN( "Expected to have 3 planes. Assuming EO." );
    return false;
  }

  // Scale image to remove border effects
  vil_image_view< PixType > filtered_img;

  if( settings_.scale_factor < 1.0 )
  {
    vgl_box_2d< int > region_to_scan( 0, img.ni(), 0, img.nj() );
    region_to_scan.scale_about_centroid( settings_.scale_factor );
    filtered_img = point_view_to_region( img, region_to_scan );
  }
  else
  {
    filtered_img = img;
  }

  vxl_uint_32 res_ir = 0;
  vxl_uint_32 res_eo = 0;

  // Sample .5% of the image for EO and IR to save computation
  const unsigned int i_ss = filtered_img.ni() /
    static_cast<unsigned int>( filtered_img.ni() * settings_.eo_ir_sample_size );
  const unsigned int j_ss = filtered_img.nj() /
    static_cast<unsigned int>( filtered_img.nj() * settings_.eo_ir_sample_size );

  const vxl_sint_32 rg_diff = settings_.rg_diff;
  const vxl_sint_32 rb_diff = settings_.rb_diff;
  const vxl_sint_32 gb_diff = settings_.gb_diff;

  for( unsigned int i = 0; i < filtered_img.ni(); i+=i_ss )
  {
    for( unsigned int j = 0; j < filtered_img.nj(); j+=j_ss )
    {
      vxl_sint_32 r = static_cast<vxl_sint_32>( filtered_img(i,j,0) );
      vxl_sint_32 g = static_cast<vxl_sint_32>( filtered_img(i,j,1) );
      vxl_sint_32 b = static_cast<vxl_sint_32>( filtered_img(i,j,2) );

      // If the pixels are all close to the same color they are ir
      if( (std::abs(r-g) < rg_diff) && (std::abs(r-b) < rb_diff) && (std::abs(g-b) < gb_diff) )
      {
        res_ir++;
      }
      else
      {
        res_eo++;
      }
    }
  }

  // If there many more IR (gray) pixels than EO (color_ pixels in this frame?
  bool result = res_ir > res_eo * settings_.eo_multiplier;

  LOG_DEBUG( "\nImage Type: " << ( result ? "IR" : "EO" ) << "\n"
    << "Total Sample Size: " << res_ir+res_eo << " out of " << filtered_img.size()/3 << "\n"
    << "Percent Sampled: "<< settings_.eo_ir_sample_size << "%\n"
    << "EO Threshold: " << static_cast<vxl_uint_32>( settings_.eo_multiplier ) << "\n"
    << "PXLs IR: "<< res_ir << "\n"
    << "PXLs EO: "<< res_eo );

  return result;
}


// ----------------------------------------------------------------
/** Is the image an EO image?
 *
 *
 */
template < class PixType >
bool
eo_ir_detector< PixType >
::is_eo( vil_image_view< PixType > const & img ) const
{
  return !this->is_ir( img );
}

} // end namespace
