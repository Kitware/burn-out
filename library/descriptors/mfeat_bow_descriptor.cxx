/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/mfeat_bow_descriptor.h>
#include <utilities/vxl_to_vl_converters.h>

extern "C"
{
  #include <vl/covdet.h>
  #include <vl/mathop.h>
  #include <vl/sift.h>
  #include <vl/mser.h>
}

#include <vil/vil_convert.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <limits>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "mfeat_bow_descriptor" );


// Parameters detailing the image patch size used to extract descriptors (in pixels)
static const int mfeat_patch_side = 31;
static const double mfeat_patch_step = 0.5;
static const double mfeat_patch_step_inv = 1.0 / mfeat_patch_step;
static const float mfeat_eps = std::numeric_limits<float>::epsilon();


// Constructor
mfeat_bow_descriptor::mfeat_bow_descriptor( const mfeat_computer_settings& options )
{
  bool success = this->configure( options );

  if( !success )
  {
    throw std::runtime_error( "Error while configuring descriptor computer" );
  }
}

bool mfeat_bow_descriptor::configure( const mfeat_computer_settings& options )
{
  settings_ = options;

  if( options.bow_filename.size() > 0 )
  {
    return this->load_vocabulary( options.bow_filename );
  }

  LOG_WARN( "No BoW model file specified" );
  return false;
}

bool mfeat_bow_descriptor::configure( const std::string& filename )
{
  return this->load_vocabulary( filename );
}

// Transpose a vlfeat descriptor (modeled off of code in vl/covdet.h)
static void flip_vl_descriptor( const float * const src,
                                float * const dst )
{
  const int orientations = mfeat_orientations_per_cell;
  const int cell_dim = mfeat_cells_per_dim;
  const int jstep = orientations * cell_dim;

  for( int j = 0; j < cell_dim; j++ )
  {
    int jp = cell_dim - 1 - j;
    for( int i = 0; i < cell_dim; i++ )
    {
      int o = orientations * i + jstep * j;
      int op = orientations * i + jstep * jp;

      dst[op] = src[o];
      for( int t = 1; t < orientations; t++ )
      {
        dst[orientations - t + op] = src[t + o];
      }
    }
  }
}

// Extract descriptors around indicated feature points from some detector
void extract_vl_sift_features( const VlCovDet *detector,
                               const VlCovDetFeature * const features,
                               const vl_size feature_count,
                               std::vector<float>& output )
{
  if( feature_count == 0 )
  {
    output.clear();
    return;
  }

  LOG_ASSERT( detector != NULL, "Detector is invalid" );
  LOG_ASSERT( features != NULL, "Detector is invalid" );

  VlSiftFilt *sift = vl_sift_new(16, 16, 1, 3, 0);
  vl_size dimension = mfeat_descriptor_bins;

  vl_sift_set_magnif(sift, 3.0);

  output.resize( feature_count * mfeat_descriptor_bins );

  float temp_desc[mfeat_descriptor_bins];
  float *desc = &output[0];
  float *patch = new float[mfeat_patch_side * mfeat_patch_side];
  float *patch_xy = new float[2 * mfeat_patch_side * mfeat_patch_side];

  for( vl_index i = 0; i < static_cast<vl_index>(feature_count); i++ )
  {
    vl_covdet_extract_patch_for_frame(const_cast<VlCovDet*>(detector),
                                      patch,
                                      15,
                                      7.5,
                                      1.0,
                                      features[i].frame);

    vl_imgradient_polar_f( patch_xy,
                           patch_xy + 1,
                           2,
                           2 * mfeat_patch_side,
                           patch,
                           mfeat_patch_side,
                           mfeat_patch_side,
                           mfeat_patch_side);

    vl_sift_calc_raw_descriptor( sift,
                                 patch_xy,
                                 temp_desc,
                                 mfeat_patch_side,
                                 mfeat_patch_side,
                                 15.0,
                                 15.0,
                                 mfeat_patch_step_inv,
                                 VL_PI / 2 );

    flip_vl_descriptor( temp_desc, desc );
    desc += dimension;
  }

  // Deallocate all resources
  vl_sift_delete( sift );
  delete[] patch;
  delete[] patch_xy;
}

// Extract various covariant features from the input image and generate
// descriptors around them.
void extract_vl_cov_features( const vl_compatible_view<float>& input,
                              const mfeat_computer_settings& options,
                              const VlCovDetMethod detector_method,
                              std::vector<float>& output )
{
  // Run detection via one of the vlfeat IP detectors
  VlCovDet *covdet = vl_covdet_new(detector_method);

  vl_covdet_set_transposed(covdet, VL_TRUE);
  vl_covdet_set_first_octave(covdet, 0);
  vl_covdet_put_image(covdet, input.data_ptr(), input.height(), input.width());
  vl_covdet_detect(covdet) ;

  if( options.boundary_margin > 0 )
  {
    vl_covdet_drop_features_outside(covdet, options.boundary_margin);
  }
  if( options.estimate_affine_shape )
  {
    vl_covdet_extract_affine_shape(covdet) ;
  }
  if( options.estimate_orientation )
  {
    vl_covdet_extract_orientations(covdet);
  }

  // Extract SIFT or HoG descriptors around each keypoint
  VlCovDetFeature const *features =
    reinterpret_cast<VlCovDetFeature const *>(vl_covdet_get_features(covdet));
  vl_size num_features = vl_covdet_get_num_features(covdet);
  extract_vl_sift_features( covdet, features, num_features, output );

  // Deallocate all resources
  vl_covdet_delete( covdet );
}

void extract_vl_mser_features( const vl_compatible_view<vl_uint8>& byte_input,
                               const vl_compatible_view<float>& float_input,
                               const mfeat_computer_settings& /*options*/,
                               std::vector<float>& output )
{
  LOG_ASSERT( byte_input.channels() == 1, "Input must contain 1 plane" );
  LOG_ASSERT( float_input.channels() == 1, "Input must contain 1 plane" );
  LOG_ASSERT( byte_input.width() == float_input.width(), "Input sizes must be the same" );
  LOG_ASSERT( byte_input.height() == float_input.height(), "Input sizes must be the same" );

  const int dims[2] = { byte_input.width(), byte_input.height() };
  VlMserFilt *filter = vl_mser_new( 2, dims );

  vl_mser_process( filter, byte_input.data_ptr() );
  vl_mser_ell_fit( filter );

  const int ellipse_count = vl_mser_get_ell_num( filter );
  const VlFrameEllipse* ellipses =
    reinterpret_cast<const VlFrameEllipse*>(vl_mser_get_ell( filter ) );
  VlCovDetFeature* features = new VlCovDetFeature[ ellipse_count ];

  // Converts vlfeat ellipses into vlfeat oriented ellipse format (required
  // for feature extraction without rewritting external code)
  for( int i = 0; i < ellipse_count; i++ )
  {
    const VlFrameEllipse& frame_ellipse = ellipses[i];
    VlFrameOrientedEllipse& feature_ellipse = features[i].frame;
    const float sqr_a22 = mfeat_eps + std::sqrt( feature_ellipse.a22 );
    const float a11_a22 = frame_ellipse.e11 * frame_ellipse.e22;
    const float a12_p2 = feature_ellipse.a22 * feature_ellipse.a22;
    feature_ellipse.x = frame_ellipse.x;
    feature_ellipse.y = frame_ellipse.y;
    feature_ellipse.a11 = std::sqrt( a11_a22 - a12_p2 ) / sqr_a22;
    feature_ellipse.a12 = 0;
    feature_ellipse.a21 = frame_ellipse.e12 / sqr_a22;
    feature_ellipse.a22 = sqr_a22;
  }

  // Create a dummy DoG detector solely to use it's internal base image for
  // sift feature extraction
  VlCovDet *covdet = vl_covdet_new( VL_COVDET_METHOD_DOG );
  vl_covdet_set_transposed( covdet, VL_TRUE );
  vl_covdet_set_first_octave( covdet, 0 );
  vl_covdet_put_image( covdet, float_input.data_ptr(), float_input.height(), float_input.width() );

  // Extract features
  extract_vl_sift_features( covdet, features, ellipse_count, output );

  vl_mser_delete( filter );
  vl_covdet_delete( covdet );
  delete[] features;
}

// Calculate a vector of raw mfeat features
void mfeat_bow_descriptor::compute_features( const vil_image_view<float>& image,
                                             std::vector<float>& features ) const
{
  LOG_ASSERT( image.nplanes() == 1 || image.nplanes() == 3, "Input must contain 1 or 3 channels" );
  LOG_ASSERT( image.ni() > 0 && image.nj() > 0, "Input image size must be non-zero" );

  // Convert input into correct format
  vl_compatible_view<float> vl_float_image( image );

  // Run mfeat detectors
  std::vector< VlCovDetMethod > detectors;
  if( settings_.compute_hessian )
  {
    detectors.push_back( VL_COVDET_METHOD_HESSIAN );
  }
  if( settings_.compute_harris_laplace )
  {
    detectors.push_back( VL_COVDET_METHOD_HARRIS_LAPLACE );
  }
  if( settings_.compute_dog )
  {
    detectors.push_back( VL_COVDET_METHOD_DOG );
  }
  if( settings_.compute_hessian_laplace )
  {
    detectors.push_back( VL_COVDET_METHOD_HESSIAN_LAPLACE );
  }
  if( settings_.compute_multiscale_hessian )
  {
    detectors.push_back( VL_COVDET_METHOD_MULTISCALE_HESSIAN );
  }
  if( settings_.compute_multiscale_harris )
  {
    detectors.push_back( VL_COVDET_METHOD_MULTISCALE_HARRIS );
  }

  for( unsigned i = 0; i < detectors.size(); i++ )
  {
    std::vector< float > new_features;

    extract_vl_cov_features( vl_float_image,
                             settings_,
                             detectors[i],
                             new_features );

    features.insert( features.end(), new_features.begin(), new_features.end() );
  }

  // MSER is not supported by covdet, so we need to compute it independently
  if( settings_.compute_mser )
  {
    std::vector< float > new_features;

    vil_image_view< vl_uint8 > stretched_image;
    vil_convert_stretch_range( image, stretched_image );
    vl_compatible_view< vl_uint8 > vl_byte_image( stretched_image );

    extract_vl_mser_features( vl_byte_image,
                              vl_float_image,
                              settings_,
                              new_features );

    features.insert( features.end(), new_features.begin(), new_features.end() );
  }
}

// Map a vector of raw mfeat features into a bow model
void mfeat_bow_descriptor::compute_bow_descriptor( const std::vector<float>& features,
                                                   std::vector<double>& output ) const
{
  this->vocabulary_->map_to_model( features,
                                   settings_.bow_settings,
                                   output );
}

// Map a vector of raw mfeat features into a bow model
void mfeat_bow_descriptor::compute_bow_descriptor( const vil_image_view<float>& image,
                                                   std::vector<double>& output ) const
{
  std::vector<float> features;
  this->compute_features(image,features);
  this->compute_bow_descriptor(features,output);
}

} // end namespace vidtk
