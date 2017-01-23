/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "blob_pixel_feature_extraction.h"

#include <vil/vil_crop.h>
#include <vil/vil_convert.h>
#include <vgl/vgl_convex.h>
#include <vgl/vgl_area.h>
#include <vgl/vgl_box_2d.h>
#include <vil/algo/vil_threshold.h>

#include <vector>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER("blob_pixel_feature_extraction_txx");

// Given the total image area, and some blob size (in pixels) for some blob,
// generate a hashed integral value representing the size of the blob.
//
// Note:
// Due to the natures of these features (as a small relative value) there are several
// numerical constants/options involved in these operations. They were chosen such
// that the output features would have low integral values, but still have decent
// discriminative powers. Having the ranges of the values of each feature be relatively low
// was important to prevent the models from becoming overstrained due to a lack of
// training cases combined with each blob pixel feature value in the same blob having
// the same value.
template< typename FeatureType >
inline FeatureType calculate_size_hash( const blob_pixel_features_settings& settings,
                                        const unsigned blob_size,
                                        const unsigned image_area )
{
  if( blob_size < settings.size_hash_min_pixel_size )
  {
    return settings.size_hash_min_size_value;
  }

  double blob_to_image_ratio = static_cast<double>(blob_size) / image_area;

  if( blob_to_image_ratio > settings.size_hash_max_ratio )
  {
    blob_to_image_ratio = settings.size_hash_max_ratio;
  }

  return static_cast<FeatureType>( settings.size_hash_additive_factor +
                                   settings.size_hash_scale_factor * blob_to_image_ratio );
}

// Given the width and height of a bbox surrounding a blob, generate a small hashed
// integral value representing their ratios.
template< typename FeatureType >
inline FeatureType calculate_ratio_hash( const blob_pixel_features_settings& settings,
                                         const unsigned bbox_width,
                                         const unsigned bbox_height )
{
  double ratio = ( bbox_width < bbox_height ? bbox_height/bbox_width : bbox_width/bbox_height );

  if( ratio > settings.ratio_hash_max_ratio )
  {
    ratio = settings.ratio_hash_max_ratio;
  }

  return static_cast<FeatureType>( settings.ratio_hash_additive_factor +
                                   settings.ratio_hash_scale_factor * ratio );
}

// Given the area of a blob, and that of a bbox surrounding all pixels in it, generate
// a small hashed integral value representing it's local density.
template< typename FeatureType >
inline FeatureType calculate_density_hash( const blob_pixel_features_settings& settings,
                                           const unsigned blob_size,
                                           const unsigned bbox_area )
{
  double ratio = ( bbox_area != 0 ? static_cast<double>( blob_size ) / bbox_area : 0 );

  return static_cast<FeatureType>( settings.density_hash_additive_factor +
                                   settings.density_hash_scale_factor * ratio );
}

// Given a list of blobs and the input image resolution of the image they came
// from, generate features from the resultant blobs in accordance with the class description.
template< typename FeatureType >
void extract_blob_pixel_features( const std::vector<vil_blob_pixel_list>& blobs,
                                  const blob_pixel_features_settings& settings,
                                  const unsigned ni,
                                  const unsigned nj,
                                  vil_image_view<FeatureType>& output )
{
  const unsigned input_size = ni * nj;

  const bool output_size = settings.enable_relative_size_measure;
  const bool output_density = settings.enable_relative_density_measure;
  const bool output_ratio = settings.enable_relative_ratio_measure;

  unsigned feature_count = 0;
  feature_count += ( output_size ? 1 : 0 );
  feature_count += ( output_density ? 1 : 0 );
  feature_count += ( output_ratio ? 1 : 0 );

  output.set_size( ni, nj, feature_count );

  // Corner case, make sure there isn't one very big blob
  if( blobs.size() == 1 && blobs[0].size() > input_size / 2 )
  {
    return;
  }

  // Blob feature extraction
  std::vector<bool> process_blob( blobs.size(), true );
  std::vector<FeatureType> hashed_blob_size( blobs.size() );
  std::vector<FeatureType> hashed_blob_density( blobs.size() );
  std::vector<FeatureType> hashed_blob_ratio( blobs.size() );

  for( unsigned i = 0; i < blobs.size(); i++ )
  {
    // Minimum size threshold
    if( blobs[i].size() < settings.minimum_size_pixels )
    {
      process_blob[i] = false;
      continue;
    }

    // Calculate blob size key and the bbox surrounding it
    vgl_box_2d< unsigned > bbox;

    for( vil_blob_pixel_list::const_iterator p = blobs[i].begin(); p != blobs[i].end(); p++ )
    {
      bbox.add( vgl_point_2d<unsigned>( p->first, p->second ) );
    }

    bbox.set_max_x( bbox.max_x()+1 );
    bbox.set_max_y( bbox.max_y()+1 );

    const unsigned blob_size = blobs[i].size();

    if( output_size )
    {
      hashed_blob_size[i] = calculate_size_hash<FeatureType>( settings, blob_size, input_size );
    }
    if( output_density )
    {
      hashed_blob_density[i] = calculate_density_hash<FeatureType>( settings, blob_size, bbox.area() );
    }
    if( output_ratio )
    {
      hashed_blob_ratio[i] = calculate_ratio_hash<FeatureType>( settings, bbox.width(), bbox.height() );
    }
  }

  // Map blob features unto output image
  const std::ptrdiff_t pstep = output.planestep();

  for( unsigned i = 0; i < blobs.size(); i++ )
  {
    if( !process_blob[i] )
    {
      continue;
    }

    // Fill in features
    for( vil_blob_pixel_list::const_iterator p = blobs[i].begin(); p != blobs[i].end(); p++ )
    {
      FeatureType *output_ptr = &output( p->first, p->second, 0 );

      if( output_size )
      {
        *output_ptr = hashed_blob_size[i];
        output_ptr += pstep;
      }
      if( output_density )
      {
        *output_ptr = hashed_blob_density[i];
        output_ptr += pstep;
      }
      if( output_ratio )
      {
        *output_ptr = hashed_blob_ratio[i];
      }
    }
  }
}

template< typename FeatureType >
void extract_blob_pixel_features( const vil_image_view<bool>& image,
                                  const blob_pixel_features_settings& settings,
                                  vil_image_view<FeatureType>& output )
{
  // Compute total feature count
  unsigned count = 0;
  count += ( settings.enable_relative_size_measure ? 1 : 0 );
  count += ( settings.enable_relative_density_measure ? 1 : 0 );
  count += ( settings.enable_relative_ratio_measure ? 1 : 0 );

  // Reset output image
  output.set_size( image.ni(), image.nj(), count * image.nplanes() );
  output.fill( 0 );

  // Perform filtering
  vil_image_view<unsigned> labels;

  for( unsigned i = 0; i < image.nplanes(); i++ )
  {
    const vil_image_view<bool> input_plane = vil_plane( image, i );

    vil_image_view<FeatureType> output_plane( output.top_left_ptr() + i * count * output.planestep(),
                                              output.ni(),
                                              output.nj(),
                                              count,
                                              output.istep(),
                                              output.jstep(),
                                              output.planestep() );

    vil_blob_labels( input_plane, vil_blob_8_conn, labels );
    std::vector<vil_blob_pixel_list> blobs;
    vil_blob_labels_to_pixel_lists( labels, blobs );

    extract_blob_pixel_features( blobs, settings, image.ni(), image.nj(), output_plane );
  }
}

} // end namespace vidtk
