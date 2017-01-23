/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/dsift_bow_descriptor.h>
#include <utilities/vxl_to_vl_converters.h>

extern "C"
{
  #include <vl/dsift.h>
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <stdexcept>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "dsift_bow_descriptor" );

// Helper function for settings (returns dim per each feature descriptor)
int dsift_settings::feature_descriptor_size() const
{
  return this->spatial_bins *
         this->spatial_bins *
         this->orientation_bins;
}

// Constructor
dsift_bow_descriptor::dsift_bow_descriptor( const dsift_settings& options )
{
  bool success = this->configure( options );

  if( !success )
  {
    throw std::runtime_error( "Error while configuring descriptor computer" );
  }
}

bool dsift_bow_descriptor::configure( const dsift_settings& options )
{
  settings_ = options;

  if( options.bow_filename.size() > 0 )
  {
    return this->load_vocabulary( options.bow_filename );
  }

  LOG_WARN( "No BoW model file specified" );
  return false;
}

bool dsift_bow_descriptor::configure( const std::string& filename )
{
  return this->load_vocabulary( filename );
}

// Compute dsift features using vlfeat, image must be contiguous
void compute_vl_dsift(const vl_compatible_view<float>& input,
                      const dsift_settings& options,
                      std::vector<float>& output)
{
  VlDsiftFilter *filter = vl_dsift_new( input.width(), input.height() );

  VlDsiftDescriptorGeometry_ descriptorSetting;
  descriptorSetting.numBinX = descriptorSetting.numBinY = options.spatial_bins;
  descriptorSetting.numBinT = options.orientation_bins;
  descriptorSetting.binSizeX = descriptorSetting.binSizeY = options.patch_size / options.spatial_bins;

  vl_dsift_set_geometry( filter, &descriptorSetting );
  vl_dsift_set_steps( filter, options.step_size, options.step_size );

  vl_dsift_process( filter, input.data_ptr() );

  int num_points = vl_dsift_get_keypoint_num( filter );
  int feature_dim = options.feature_descriptor_size();

  output.resize( num_points * feature_dim);

  std::memmove( &output[0], vl_dsift_get_descriptors( filter ), sizeof(float) * num_points * feature_dim );

  vl_dsift_delete( filter );
}

// Calculate a vector of raw dsift features
void dsift_bow_descriptor::compute_features( const vil_image_view<float>& image,
                                             std::vector<float>& features ) const
{
  LOG_ASSERT( image.nplanes() == 1 || image.nplanes() == 3, "Input must contain 1 or 3 channels" );

  if( image.ni() < 2 || image.nj() < 2 )
  {
    LOG_ERROR( "Input image dimensions must be at least 2 pixels" );
    return;
  }

  vl_compatible_view<float> vl_image( image );

  // Run DSIFT and extract a vector of features.
  compute_vl_dsift( vl_image, settings_, features );

}

// Map a vector of raw dsift features into a bow model
void dsift_bow_descriptor::compute_bow_descriptor( const std::vector<float>& features,
                                                   std::vector<double>& output ) const
{
  this->vocabulary_->map_to_model( features,
                                   settings_.bow_settings,
                                   output );
}

// Map a vector of raw dsift features into a bow model
void dsift_bow_descriptor::compute_bow_descriptor( const vil_image_view<float>& image,
                                                   std::vector<double>& output ) const
{
  std::vector<float> features;
  this->compute_features( image,features );
  this->compute_bow_descriptor( features,output );
}

} // end namespace vidtk
