/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <video_transforms/threaded_image_transform.h>

#include <string>
#include <vector>
#include <iostream>

#include <boost/lexical_cast.hpp>

using namespace vidtk;

namespace
{

void fill_func( vil_image_view< bool >& image )
{
  image.fill( true );
}

}

int test_threaded_image_transform( int /*argc*/, char */*argv*/[] )
{
  testlib_test_start( "threaded_image_transform" );

  threaded_image_transform< vil_image_view< bool > > threader( 5, 5 );
  threader.set_function( boost::bind( &fill_func, _1 ) );

  vil_image_view< bool > test_image( 100, 100, 1 );
  test_image.fill( false );

  threader.apply_function( test_image );

  bool all_valid = true;

  for( unsigned i = 0; i < test_image.ni(); ++i )
  {
    for( unsigned j = 0; j < test_image.nj(); ++j )
    {
      if( test_image( i, j ) == false )
      {
        all_valid = false;
      }
    }
  }

  TEST( "Output Image Size 1", test_image.ni(), 100 );
  TEST( "Output Image Size 2", test_image.nj(), 100 );

  TEST( "Output Image Content", all_valid, true );

  return testlib_test_summary();
}
