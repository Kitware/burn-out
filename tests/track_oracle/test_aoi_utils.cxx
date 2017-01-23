/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vector>
#include <sstream>

#include <track_oracle/aoi_utils/aoi_utils.h>

#include <vgl/vgl_point_2d.h>

#include <testlib/testlib_test.h>


#include <logger/logger.h>
VIDTK_LOGGER( "test_aoi_utils_cxx" );

namespace { // anon

using vidtk::aoi_utils::aoi_t;
using vidtk::aoi_utils::aoi_exception;
using std::string;
using std::ostringstream;
using std::vector;

void
test_aoi( const string& s,
          aoi_t::flavor_t expected_flavor,
          double in_x_0, double in_y_0,
          double in_x_1, double in_y_1,
          double out_x_0, double out_y_0,
          double out_x_1, double out_y_1 )
{
  vector<double> x;
  x.push_back( in_x_0 ); x.push_back( in_x_1 ); x.push_back( out_x_0 ); x.push_back( out_x_1 );
  vector<double> y;
  y.push_back( in_y_0 ); y.push_back( in_y_1 ); y.push_back( out_y_0 ); y.push_back( out_y_1 );

  try
  {
    aoi_t a( s );
    {
      ostringstream oss;
      oss << "AOI '" << s << "' constructs; string is '" << a.to_str() << "'";
      TEST( oss.str().c_str(), true, true );
    }
    {
      ostringstream oss;
      oss << "AOI flavor is " << a.flavor() << "; expected " << expected_flavor;
      TEST( oss.str().c_str(), a.flavor(), expected_flavor );
    }
    for (size_t i=0; i<4; ++i)
    {
      bool expected_in = (i < 2);
      bool is_in = a.in_aoi( x[i], y[i] );
      ostringstream oss;
      oss << "AOI: point " << x[i] << ", " << y[i] << " is in? " << is_in << " ; expected to be in? " << expected_in;
      TEST( oss.str().c_str(), expected_in, is_in );
    }
  }
  catch (aoi_exception& e)
  {
    ostringstream oss;
    oss << "AOI '" << s << "' constructs";
    TEST( oss.str().c_str(), true, false );
  }
}

void
test_aoi( const string& s )
{
  try
  {
    aoi_t a( s );
    {
      ostringstream oss;
      oss << "AOI '" << s << "' constructed but shouldn't have; string is '" << a.to_str() << "'";
      TEST( oss.str().c_str(), true, false );
    }
  }
  catch (aoi_exception& e )
  {
    ostringstream oss;
    oss << "AOI '" << s << "' corrected failed to construct: error was '" << e.what() << "'";
    TEST( oss.str().c_str(), true, true );
  }
}


} // anon


int test_aoi_utils( int /* argc */, char* /*argv*/ [] )
{
  testlib_test_start( "test_aoi_utils" );

  test_aoi( "240x191+1500+1200", aoi_t::PIXEL, 1501, 1201, 1600, 1300,   1400, 1100, 0, 0);
  test_aoi( " p -19.5 , 20.0 : -1 , 1", aoi_t::PIXEL, -2,5, -10,10,  0,0, 100,100 );
  test_aoi( "p0,0:0,10:10,20:20,10:20,0", aoi_t::PIXEL, 0,0,  10,10,  -1, 1, 20.5,0 );
  test_aoi( "p0,10:10,20:0,0:20,10:20,0", aoi_t::PIXEL, 0,0,  10,10,  -1, 1, 20.5,0 );
  test_aoi( "p -1.0e6,3.141e3: 0.3E2 , -12", aoi_t::PIXEL, -9.0e5,400,  0,0,   0,-14, -1.0e7,0 );
  test_aoi( "250x250-1000-1000", aoi_t::PIXEL, -750,-750, -1000,-1000, 0,0,  100, 100);

  test_aoi( "-84.139042,39.799339:-84.095042,39.75533", aoi_t::GEO, -84.1,39.78,  -84.099,39.77,  -85, 40,  0,0);
  test_aoi( "85.14,-39.8:85.15,-40:85.16,-39.8", aoi_t::GEO, 85.15,-39.9,  85.149,-39.89, 0,0, 85.15,-40.001 );

  test_aoi( "" );
  test_aoi( "240x191+1500+1200 blah blah" );

  // shouldn't fit in a single MGRS zone
  test_aoi( "0,0:89,89" );

  return testlib_test_summary();
}
