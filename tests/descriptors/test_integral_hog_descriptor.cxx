/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_image_view.h>

#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <testlib/testlib_test.h>

#include <descriptors/integral_hog_descriptor.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void test_integral_hog_descriptor()
{
  vil_image_view< vxl_byte > image( 50, 50, 1 );
  image.fill( 0 );
  for( unsigned i = 20; i < 30; i++ )
  {
    image( 25, i ) = 200;
    image( i, 25 ) = 200;
  }

  custom_ii_hog_model hog_model;
  hog_model.cells_.push_back( vgl_box_2d< double >( 0.0, 0.5, 0.0, 0.5 ) );
  hog_model.cells_.push_back( vgl_box_2d< double >( 0.5, 1.0, 0.0, 0.5 ) );
  hog_model.cells_.push_back( vgl_box_2d< double >( 0.0, 0.5, 0.5, 1.0 ) );
  hog_model.cells_.push_back( vgl_box_2d< double >( 0.5, 1.0, 0.5, 1.0 ) );
  hog_model.blocks_.resize( 2 );
  hog_model.blocks_[0].push_back( 0 );
  hog_model.blocks_[0].push_back( 1 );
  hog_model.blocks_[1].push_back( 2 );
  hog_model.blocks_[1].push_back( 3 );

  TEST( "Is custom hog model valid?", hog_model.is_valid(), true );

  integral_hog_descriptor< vxl_byte > ii_computer( image );

  vgl_box_2d< unsigned > region1( 10, 40, 10, 40 );
  vgl_box_2d< unsigned > region2( 30, 40, 30, 40 );

  vcl_vector< double > output_hog_descriptor1;
  vcl_vector< double > output_hog_descriptor2;
  vcl_vector< double > output_cell_descriptor1;

  ii_computer.generate_hog_descriptor( region1, hog_model, output_hog_descriptor1 );
  ii_computer.generate_hog_descriptor( region2, hog_model, output_hog_descriptor2 );
  ii_computer.cell_descriptor( region1, output_cell_descriptor1 );

  TEST( "Cell output descriptor size", output_cell_descriptor1.size(), 8 );
  TEST( "HoG1 output descriptor size", output_hog_descriptor1.size(), 32 );
  TEST( "HoG2 output descriptor size", output_hog_descriptor2.size(), 32 );
}

} // end anonymous namespace

int test_integral_hog_descriptor( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_integral_hog_descriptor" );

  test_integral_hog_descriptor();

  return testlib_test_summary();
}
