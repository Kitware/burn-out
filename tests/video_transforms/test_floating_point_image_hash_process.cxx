/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_algorithm.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <testlib/testlib_test.h>
#include <pipeline_framework/async_pipeline.h>

#include <video_transforms/floating_point_image_hash_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void test_fp_hash()
{
  vil_image_view<double> input( 20, 20 );

  input.fill( 500.0 );
  input( 10, 10 ) = 25.5;
  input( 10, 11 ) = 10.0;
  input( 10, 12 ) = 49.9;

  floating_point_image_hash_process<double,vxl_byte> hash_proc("hasher");

  config_block blk = hash_proc.params();
  blk.set( "scale_factor", "0.25" );
  blk.set( "max_input_value", "400" );
  hash_proc.set_params( blk );

  hash_proc.set_input_image( input );
  hash_proc.step();

  vil_image_view<vxl_byte> output = hash_proc.hashed_image();

  TEST( "Hashed image value 1", output(1,1), 100 );
  TEST( "Hashed image value 2", output(10,10), 6 );
  TEST( "Hashed image value 3", output(10,11), 3 );
  TEST( "Hashed image value 4", output(10,12), 12 );
}

} // end anonymous namespace

int test_floating_point_image_hash_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "floating_point_image_hash_process" );

  test_fp_hash();

  return testlib_test_summary();
}
