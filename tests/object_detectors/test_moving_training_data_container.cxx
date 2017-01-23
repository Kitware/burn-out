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

#include <object_detectors/moving_training_data_container.h>

namespace
{

using namespace vidtk;

void test_moving_training_data_container()
{
  const std::size_t len = 5;

  const double f1[ len ] = { 0.1, 0.2, 0.3, 0.0, 0.4 };
  const double f2[ len ] = { 0.2, 0.1, 0.3, 0.0, 0.4 };
  const double f3[ len ] = { 0.1, 0.1, 0.3, 0.1, 0.4 };
  const double f4[ len ] = { 0.1, 0.2, 0.2, 0.3, 0.2 };

  std::vector< double > fv1, fv2, fv3, fv4;

  fv1.assign( f1, f1 + len );
  fv2.assign( f2, f2 + len );
  fv3.assign( f3, f3 + len );
  fv4.assign( f4, f4 + len );

  // Test FIFO
  {
    moving_training_data_settings settings;

    settings.max_entries = 10;
    settings.features_per_entry = len;
    settings.insert_mode = moving_training_data_settings::FIFO;

    moving_training_data_container< double > feature_points;

    feature_points.configure( settings );

    feature_points.insert( fv1 );
    feature_points.insert( fv2 );
    feature_points.insert( fv3 );

    TEST( "Test configured buffer size 1", feature_points.size() == 3, true );

    TEST( "Test value 1", feature_points.entry( 0 )[ 0 ], 0.1 );
    TEST( "Test value 2", feature_points.entry( 0 )[ 1 ], 0.2 );

    for( unsigned i = 0; i < 7; i++ )
    {
      feature_points.insert( fv2 );
    }

    TEST( "Test configured buffer size 2", feature_points.size() == 10, true );
    feature_points.insert( fv3 );

    TEST( "Test configured buffer size 3", feature_points.size() == 10, true );
    feature_points.insert( fv3 );

    TEST( "Test configured buffer size 4", feature_points.size() == 10, true );

    TEST( "Test value 3", feature_points.entry( 0 )[ 3 ], 0.0 );
    TEST( "Test value 4", feature_points.entry( 1 )[ 3 ], 0.1 );
    TEST( "Test value 5", feature_points.entry( 2 )[ 3 ], 0.1 );
    TEST( "Test value 6", feature_points.entry( 3 )[ 3 ], 0.0 );
  }
}

} // end anonymous namespace

int test_moving_training_data_container( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "moving_training_data_container" );

  test_moving_training_data_container();

  return testlib_test_summary();
}
