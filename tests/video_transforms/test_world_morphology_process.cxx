/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>
#include <pipeline_framework/sync_pipeline.h>
#include <utilities/homography.h>
#include <video_transforms/world_morphology_process.h>
#include <object_detectors/homography_to_scale_image_process.h>

#include <vil/vil_image_view.h>
#include <vul/vul_file.h>
#include <vcl_vector.h>
#include <vcl_fstream.h>

#include <vxl_config.h>

namespace {

using namespace vidtk;

// Perform a very simple test where we check if 2 seperate pairs of dots
// in a binary image become connected when performing an opening operation
// when using a constant GSD
void test_basic_constant_gsd_experiment()
{
  sync_pipeline p;

  // Configure mini pipeline
  world_morphology_process<bool> morph_proc( "morphology" );

  p.add( &morph_proc );

  config_block c = p.params();
  c.set( "morphology:mode", "CONSTANT_GSD" );
  c.set( "morphology:closing_radius", "4.0" );

  // Create example inputs
  vil_image_view<bool> test_input( 1000, 1000 );
  test_input.fill( false );
  test_input( 99, 500 ) = true;
  test_input( 101, 500 ) = true;
  test_input( 899, 500 ) = true;
  test_input( 901, 500 ) = true;

  morph_proc.set_source_image( test_input );

  // Run pipeline
  TEST( "const gsd pipeline: set_params()", p.set_params( c ), true );
  TEST( "const gsd pipeline: intialize()", p.initialize(), true );
  TEST( "const gsd pipeline: execute()", p.execute(), process::SUCCESS );

  // Check outputs
  vil_image_view<bool> out = morph_proc.image();

  TEST( "const gsd output: is_valid", !out, false );
  TEST( "const gsd output: original value 1", out(99,500), true );
  TEST( "const gsd output: original value 2", out(901,500), true );
  TEST( "const gsd output: inside value 1", out(100,500), true );
  TEST( "const gsd output: inside value 2", out(900,500), true );
}

// Perform a very simple test where we check if 2 seperate pairs of dots
// in a binary image become connected when performing an opening operation
// when setting an input homography.
void test_basic_variable_gsd_experiment()
{
  sync_pipeline p;

  // Configure mini pipeline
  world_morphology_process<bool> morph_proc( "morphology" );
  homography_to_scale_image_process<unsigned> scale_image_proc( "scale_image_gen" );

  p.add( &morph_proc );
  p.add( &scale_image_proc );

  config_block c = p.params();

  c.set( "scale_image_gen:mode", "HASHED_GSD_IMAGE" );
  c.set( "scale_image_gen:algorithm", "GRID" );
  c.set( "scale_image_gen:hashed_gsd_scale_factor", "100" );
  c.set( "scale_image_gen:hashed_max_gsd", "0.15" );

  c.set( "morphology:mode", "HASHED_GSD_IMAGE" );
  c.set( "morphology:closing_radius", "0.15" );
  c.set( "morphology:hashed_gsd_scale_factor", "100" );

  p.connect( scale_image_proc.hashed_gsd_image_port(),
             morph_proc.set_hashed_gsd_image_port() );

  // Create example inputs
  vil_image_view<bool> test_input( 1000, 1000 );
  test_input.fill( false );
  test_input( 500, 99 ) = true;
  test_input( 500, 101 ) = true;
  test_input( 500, 899 ) = true;
  test_input( 500, 901 ) = true;

  image_to_plane_homography test_homog;
  homography::transform_t transform;

  double mat[9] = { 0.00026480879, 0.0014405038, -0.71608763,
                    0.00035294478, -0.0017736213, 0.69800384,
                    -2.0657324e-006, 2.9985304e-005, -0.0019978884 };

  transform.set( mat );
  test_homog.set_transform( transform );
  test_homog.set_valid( true );

  morph_proc.set_source_image( test_input );
  scale_image_proc.set_image_to_world_homography( test_homog );
  scale_image_proc.set_resolution( homography_to_scale_image_process<unsigned>::resolution_t( 1000, 1000 ) );

  // Run pipeline
  TEST( "morph_sp pipeline: set_params()", p.set_params( c ), true );
  TEST( "morph_sp pipeline: intialize()", p.initialize(), true );
  TEST( "morph_sp pipeline: execute()", p.execute(), process::SUCCESS );

  // Check outputs
  vil_image_view<bool> out = morph_proc.image();

  TEST( "morph_sp output: is_valid", !out, false );
  TEST( "morph_sp output: original value 1", out(500,99), true );
  TEST( "morph_sp output: original value 2", out(500,901), true );
  TEST( "morph_sp output: inside value 1", out(500,100), false );
  TEST( "morph_sp output: inside value 2", out(500,900), true );
}

}

int test_world_morphology_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "world_morphology_process" );

  test_basic_constant_gsd_experiment();
  test_basic_variable_gsd_experiment();

  return testlib_test_summary();
}
