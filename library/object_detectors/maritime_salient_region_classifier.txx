/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "maritime_salient_region_classifier.h"

#include <logger/logger.h>

#include <utilities/point_view_to_region.h>

#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/vil_math.h>

#include <vil/algo/vil_threshold.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_gauss_filter.h>
#include <vil/algo/vil_blob.h>

#include <cmath>
#include <limits>
#include <fstream>
#include <sstream>
#include <limits>

namespace vidtk
{

VIDTK_LOGGER( "maritime_salient_region_classifier" );


// Default negative value for the classification output image, when we
// know the pixel is definitly not foreground.
const float negative_classifier_value = -0.01f;


// Constructor
template< typename PixType, typename FeatureType >
maritime_salient_region_classifier< PixType, FeatureType >
::maritime_salient_region_classifier( const maritime_classifier_settings& params )
{
  settings_ = params;
  frame_counter_ = 0;
  frames_since_last_break_ = 0;

  if( params.use_ooi_pixel_classifier_ &&
      !ooi_classifier_.load_from_file( params.pixel_classifier_filename_ ) )
  {
    LOG_FATAL( "Unable to load classifier from file: " << params.pixel_classifier_filename_ );
  }
}


// Process a new frame
template< typename PixType, typename FeatureType >
void
maritime_salient_region_classifier< PixType, FeatureType >
::process_frame( const input_image_t& input_image,
                 const weight_image_t& /*saliency_map*/,
                 const mask_image_t& saliency_mask,
                 const feature_array_t& features,
                 output_image_t& output_weights,
                 mask_image_t& output_mask,
                 const image_border_t& border,
                 const mask_image_t& obstruction_mask )
{
  // Confirm required inputs
  LOG_ASSERT( input_image.ni() == saliency_mask.ni(), "Input and sal image sizes must match" );
  LOG_ASSERT( input_image.nj() == saliency_mask.nj(), "Input and sal image sizes must match" );

  // Set output image size
  output_weights = output_image_t( input_image.ni(), input_image.nj() );
  output_mask.deep_copy( saliency_mask );

  // Determine eo/ir flag for every frame, only run on eo for now.
  if( settings_.only_run_on_eo_ )
  {
    vil_image_view< PixType > inner_image = input_image;

    if( border.area() > 0 )
    {
      inner_image = point_view_to_region ( input_image, border );
    }

    if( !eo_ir_detector_.is_eo( inner_image ) )
    {
      output_weights.fill( negative_classifier_value );
      output_mask.fill( false );
      return;
    }
  }

  // Increment frame counters
  frame_counter_++;
  frames_since_last_break_++;

  // Cancel out obstruction detections in saliency image and count detections
  unsigned obstruction_mask_count = 0, saliency_mask_count = 0;

  if( obstruction_mask.size() > 0 )
  {
    LOG_ASSERT( input_image.ni() == obstruction_mask.ni(),
                "Input and obstruction mask image sizes must match" );
    LOG_ASSERT( input_image.nj() == obstruction_mask.nj(),
                "Input and obstruction mask image sizes must match" );

    for( unsigned j = 0; j < obstruction_mask.nj(); j++ )
    {
      for( unsigned i = 0; i < obstruction_mask.ni(); i++ )
      {
        if( obstruction_mask(i,j) )
        {
          obstruction_mask_count++;
          output_mask(i,j) = false;
          output_weights(i,j) = negative_classifier_value;
        }
      }
    }
  }

  for( unsigned j = 0; j < obstruction_mask.nj(); j++ )
  {
    for( unsigned i = 0; i < obstruction_mask.ni(); i++ )
    {
      if( saliency_mask(i,j) )
      {
        saliency_mask_count++;
      }
    }
  }

  // Simple bad frame suppression
  const unsigned image_area = input_image.size();

  double percent_mask_pixels = static_cast<double>( obstruction_mask_count ) / image_area;
  double percent_salient_pixels = static_cast<double>( saliency_mask_count ) / image_area;
  double percent_all_pixels = percent_salient_pixels + percent_mask_pixels;

  if( percent_mask_pixels > settings_.bad_frame_obstruction_threshold_ ||
      percent_salient_pixels > settings_.bad_frame_saliency_threshold_ ||
      percent_all_pixels > settings_.bad_frame_all_fg_threshold_ )
  {
    output_weights.fill( negative_classifier_value );
    output_mask.fill( false );
    frames_since_last_break_ = 0;
    return;
  }

  // Perform optional secondary OOI classification for every pixel which
  // was marked part of our initial OOI.
  if( settings_.use_ooi_pixel_classifier_ )
  {
    weight_image_t classified_image = weight_image_t( input_image.ni(), input_image.nj() );

    feature_array_t cropped_features = features;
    mask_image_t cropped_mask = saliency_mask;
    weight_image_t cropped_classified = classified_image;

    if( border.area() > 0 )
    {
      for( unsigned i = 0; i < cropped_features.size(); i++ )
      {
        cropped_features[i] = point_view_to_region( features[i], border );
      }
      cropped_mask = point_view_to_region( saliency_mask, border );
      cropped_classified = point_view_to_region( classified_image, border );
    }

    ooi_classifier_.classify_images( cropped_features,
                                     cropped_mask,
                                     cropped_classified );

    // Merge in OOI results
    for( unsigned j = 0; j < classified_image.nj(); j++ )
    {
      for( unsigned i = 0; i < classified_image.ni(); i++ )
      {
        if( saliency_mask( i,j ) )
        {
          const double value = classified_image(i,j);

          if( value < 0 )
          {
            output_mask(i,j) = false;
            output_weights(i,j) = negative_classifier_value;
          }
          else
          {
            output_weights(i,j) += static_cast<float>(value);
          }
        }
      }
    }
  }
}


} // end namespace vidtk
