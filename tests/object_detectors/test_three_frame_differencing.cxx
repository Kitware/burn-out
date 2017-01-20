/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <vil/vil_image_view.h>

#include <algorithm>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <object_detectors/three_frame_differencing_process.h>

namespace
{

using namespace vidtk;

void test_three_frame_differencing_process()
{
  three_frame_differencing< vxl_byte, float > differencer;
  frame_differencing_settings settings;

  // Create sample test inputs
  vil_image_view<vxl_byte> input_images[3];

  input_images[0].set_size( 20, 20, 1 );
  input_images[1].set_size( 20, 20, 1 );
  input_images[2].set_size( 20, 20, 1 );

  input_images[0].fill( 0 );
  input_images[1].fill( 0 );
  input_images[2].fill( 0 );

  input_images[0]( 5, 5 ) = 255;
  input_images[1]( 10, 10 ) = 255;
  input_images[2]( 15, 15 ) = 255;

  std::vector< frame_differencing_settings::operation_type_t > modes_to_test;

  modes_to_test.push_back( frame_differencing_settings::UNSIGNED_SUM );
  modes_to_test.push_back( frame_differencing_settings::SIGNED_SUM );
  modes_to_test.push_back( frame_differencing_settings::UNSIGNED_MIN );
  modes_to_test.push_back( frame_differencing_settings::UNSIGNED_ZSCORE_SUM );
  modes_to_test.push_back( frame_differencing_settings::UNSIGNED_ADAPTIVE_ZSCORE_SUM );
  modes_to_test.push_back( frame_differencing_settings::UNSIGNED_ADAPTIVE_ZSCORE_MIN );

  for( unsigned i = 0; i < modes_to_test.size(); i++ )
  {
    std::string identifier = boost::lexical_cast< std::string >( i );

    std::string test1_id = "Difference image value 1, test " + identifier;
    std::string test2_id = "Difference image value 2, test " + identifier;
    std::string test3_id = "Difference image value 3, test " + identifier;

    std::string test4_id = "Mask value 1, test " + identifier;
    std::string test5_id = "Mask value 2, test " + identifier;
    std::string test6_id = "Mask value 3, test " + identifier;

    vil_image_view< float > diff_image;
    vil_image_view< bool > fg_mask;

    settings.operation_type = modes_to_test[i];

    differencer.process_frames( input_images[0],
                                input_images[1],
                                input_images[2],
                                fg_mask,
                                diff_image );

    TEST_NEAR( test1_id.c_str(), diff_image( 5, 5 ), 0.0f, 0.001f );
    TEST_NEAR( test2_id.c_str(), diff_image( 10, 10 ), 0.0f, 0.001f );

    TEST( test3_id.c_str(), diff_image( 15, 15 ) > 0.1, true );

    TEST( test4_id.c_str(), fg_mask( 5, 5 ), false );
    TEST( test5_id.c_str(), fg_mask( 10, 10 ), false );
    TEST( test6_id.c_str(), fg_mask( 15, 15 ), true );
  }
}

} // end anonymous namespace

int test_three_frame_differencing( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "three_frame_differencing" );

  test_three_frame_differencing_process();

  return testlib_test_summary();
}
