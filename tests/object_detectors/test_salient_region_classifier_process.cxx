/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <vector>

#include <vil/vil_image_view.h>
#include <vil/vil_save.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

#include <testlib/testlib_test.h>
#include <pipeline_framework/async_pipeline.h>
#include <boost/lexical_cast.hpp>

#include <object_detectors/groundcam_salient_region_classifier.h>
#include <object_detectors/maritime_salient_region_classifier.h>
#include <object_detectors/salient_region_classifier_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;


void test_salient_region_classifier_process( const std::string& data_dir )
{
  // Create sample test inputs
  //
  // Set 3 pixels as part of our initial fg image. One should be removed due
  // to the scene obstruction mask, one should be removed via the secondary
  // classifier, and one should remain part of our fg mask.
  vil_image_view<vxl_byte> sample_input_image( 20, 20, 3 );
  sample_input_image.fill( 128 );

  vil_image_view<double> sample_saliency_map( 20, 20, 1 );
  sample_saliency_map.fill( -0.1 );
  sample_saliency_map( 10, 10 ) = 0.1;
  sample_saliency_map( 10, 12 ) = 0.1;
  sample_saliency_map( 10, 14 ) = 0.1;

  vil_image_view<bool> sample_saliency_mask( 20, 20, 1 );
  sample_saliency_mask.fill( false );
  sample_saliency_mask( 10, 10 ) = true;
  sample_saliency_mask( 10, 12 ) = true;
  sample_saliency_mask( 10, 14 ) = true;

  vil_image_view<bool> sample_obstruction_mask( 20, 20, 1 );
  sample_obstruction_mask.fill( false );
  sample_obstruction_mask( 10, 10 ) = true;

  std::vector< vil_image_view<vxl_byte> > sample_feature_array( 3 );
  for( unsigned i = 0; i < 3; i++ )
  {
    sample_feature_array[i].set_size( 20, 20 );
    sample_feature_array[i].fill( 0 );
  }
  sample_feature_array[1]( 10, 10 ) = 255;
  sample_feature_array[1]( 10, 12 ) = 255;

  image_border sample_border( 1, 19, 1, 19 );

  vil_image_view<float> output_weight_image;
  vil_image_view<bool> output_mask;

  // Test stand-alone groundcam saliency detector function without mask
  {
    groundcam_classifier_settings settings;
    settings.use_pixel_classifier_ = true;
    settings.pixel_classifier_filename_ = data_dir + "/simple_hashed_img_classifier.clfr";

    groundcam_salient_region_classifier< vxl_byte, vxl_byte > classifier( settings );

    classifier.process_frame( sample_input_image,
                              sample_saliency_map,
                              sample_saliency_mask,
                              sample_feature_array,
                              output_weight_image,
                              output_mask );

    TEST( "GC wo Mask Value 1", output_mask( 0, 0 ), false );
    TEST( "GC wo Mask Value 2", output_mask( 10, 10 ), true );
    TEST( "GC wo Mask Value 3", output_mask( 10, 12 ), true );
    TEST( "GC wo Mask Value 4", output_mask( 10, 14 ), false );
  }

  // Test stand-alone groundcam saliency detector function with mask
  {
    groundcam_classifier_settings settings;
    settings.use_pixel_classifier_ = true;
    settings.pixel_classifier_filename_ = data_dir + "/simple_hashed_img_classifier.clfr";

    groundcam_salient_region_classifier< vxl_byte, vxl_byte > classifier( settings );

    classifier.process_frame( sample_input_image,
                              sample_saliency_map,
                              sample_saliency_mask,
                              sample_feature_array,
                              output_weight_image,
                              output_mask,
                              sample_border,
                              sample_obstruction_mask );

    TEST( "GC w Mask Value 1", output_mask( 0, 0 ), false );
    TEST( "GC w Mask Value 2", output_mask( 10, 10 ), false );
    TEST( "GC w Mask Value 3", output_mask( 10, 12 ), true );
    TEST( "GC w Mask Value 4", output_mask( 10, 14 ), false );
  }

  // Test stand-alone maritime saliency detector function with mask
  {
    maritime_classifier_settings settings;
    maritime_salient_region_classifier< vxl_byte, vxl_byte > classifier( settings );

    classifier.process_frame( sample_input_image,
                              sample_saliency_map,
                              sample_saliency_mask,
                              sample_feature_array,
                              output_weight_image,
                              output_mask,
                              sample_border,
                              sample_obstruction_mask );

    TEST( "MT w Mask Value 1", output_mask( 0, 0 ), false );
    TEST( "MT w Mask Value 2", output_mask( 10, 10 ), false );
    TEST( "MT w Mask Value 3", output_mask( 10, 12 ), true );
    TEST( "MT w Mask Value 4", output_mask( 10, 14 ), true );
  }
}

} // end anonymous namespace

int test_salient_region_classifier_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument" << std::endl;
    return EXIT_FAILURE;
  }

  std::string dir = argv[1];

  testlib_test_start( "salient_region_classifier_process" );

  test_salient_region_classifier_process( dir );

  return testlib_test_summary();
}
