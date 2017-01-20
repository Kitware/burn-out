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

#include <video_transforms/nearest_neighbor_inpaint.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void test_inpainting()
{
  vil_image_view<vxl_byte> input_intensity( 20, 20 );
  vil_image_view<bool> input_mask( 20, 20 );

  input_intensity.fill( 100 );
  input_mask.fill( false );

  input_intensity( 10, 10 ) = 0;

  input_mask( 10, 9 ) = true;
  input_mask( 10, 10 ) = true;
  input_mask( 10, 11 ) = true;

  input_intensity( 15, 16 ) = 255;
  input_intensity( 17, 16 ) = 255;
  input_intensity( 16, 17 ) = 255;
  input_intensity( 16, 15 ) = 255;

  input_mask( 16, 16 ) = true;

  nn_inpaint( input_intensity, input_mask );

  TEST( "Inpainted value 1", input_intensity(10,9), 100 );
  TEST( "Inpainted value 2", input_intensity(10,10), 100 );
  TEST( "Inpainted value 3", input_intensity(10,11), 100 );
  TEST( "Inpainted value 4", input_intensity(16,16), 255 );
  TEST( "Non-inpainted value 1", input_intensity(1,1), 100 );
  TEST( "Non-inpainted value 2", input_intensity(19,19), 100 );
  TEST( "Non-inpainted value 3", input_intensity(15,16), 255 );
  TEST( "Non-inpainted value 4", input_intensity(10,8), 100 );
}

} // end anonymous namespace

int test_nearest_neighbor_inpaint( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "nearest_neighbor_inpaint" );

  test_inpainting();

  return testlib_test_summary();
}
