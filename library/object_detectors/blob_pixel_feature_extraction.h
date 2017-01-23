/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_blob_pixel_feature_extractor_h_
#define vidtk_blob_pixel_feature_extractor_h_

#include <vil/vil_image_view.h>
#include <vil/algo/vil_blob.h>

#include <utilities/external_settings.h>

#include <vector>
#include <limits>
#include <string>

namespace vidtk
{

#define settings_macro( add_param ) \
  add_param( \
    enable_relative_size_measure, \
    bool, \
    true, \
    "Output a relative measure of the size of each blob in relation " \
    "to the image size?" ); \
  add_param( \
    size_hash_scale_factor, \
    double, \
    50.0, \
    "In computing the output size feature value, scale the size ratio " \
    "(blob area over image area) by this fixed amount to produce the " \
    "output feature value." ); \
  add_param( \
    size_hash_additive_factor, \
    double, \
    3.0, \
    "Add this amount to the scaled relative size ratio" ); \
  add_param( \
    size_hash_max_ratio, \
    double, \
    0.10, \
    "Maximum size ratio to report" ); \
  add_param( \
    size_hash_min_pixel_size, \
    unsigned, \
    10, \
    "Minimum size (in pixels) of blobs to extract features for" ); \
  add_param( \
    size_hash_min_size_value, \
    unsigned, \
    1, \
    "For blobs under the minimum size, the size feature value to " \
    "give them" ); \
  add_param( \
    enable_relative_density_measure, \
    bool, \
    true, \
    "Output a relative measure of the density of the blob compared " \
    "against its surrounding?" ); \
  add_param( \
    density_hash_scale_factor, \
    double, \
    5.0, \
    "In computing the output density feature value, scale the density " \
    "ratio (blob area over bbox area) by this fixed amount to produce " \
    "the output feature value" ); \
  add_param( \
    density_hash_additive_factor, \
    double, \
    0.0, \
    "Add this amount to the scaled relative density ratio" ); \
  add_param( \
    enable_relative_ratio_measure, \
    bool, \
    true, \
    "Output a relative measure of the aspect ratio of a bbox drawn " \
    "around each blob?" ); \
  add_param( \
    ratio_hash_scale_factor, \
    double, \
    -2.0, \
    "In computing the output ratio feature value, scale the size " \
    "ratio max (blob width over blob height, bob height over blob " \
    "width) by this fixed amount to produce the output feature value." ); \
  add_param( \
    ratio_hash_additive_factor, \
    double, \
    9.0, \
    "Add this amount to the scaled relative ratio value" ); \
  add_param( \
    ratio_hash_max_ratio, \
    double, \
    4.0, \
    "The maximum blob dimension ratio to report." ); \
  add_param( \
    minimum_size_pixels, \
    unsigned, \
    10, \
    "Minimum size threshold (in pixels) of blobs to extract features " \
    "from" ); \

init_external_settings( blob_pixel_features_settings, settings_macro );

#undef settings_macro


/// Extracts several pixel-level features from the given blobs, for each blob.
template< typename FeatureType >
void extract_blob_pixel_features( const std::vector<vil_blob_pixel_list>& blobs,
                                  const blob_pixel_features_settings& settings,
                                  const unsigned ni,
                                  const unsigned nj,
                                  vil_image_view<FeatureType>& output );


/// Extracts several pixel-level features from the given blobs, for each blob.
/// Only utilizes information from binary blobs in the given input image. The input
/// image can contain multiple planes, features will be generated for each plane
/// independently.
template< typename FeatureType >
void extract_blob_pixel_features( const vil_image_view<bool>& image,
                                  const blob_pixel_features_settings& settings,
                                  vil_image_view<FeatureType>& output );


} // end namespace vidtk


#endif // vidtk_blob_pixel_feature_extractor_h_
