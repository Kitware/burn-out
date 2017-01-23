/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/frame_downsampler.h>

#include <string>
#include <vector>
#include <iostream>

#include <boost/lexical_cast.hpp>

using namespace vidtk;

namespace
{

void run_sim( frame_downsampler& downsampler,
              const frame_downsampler_settings& settings,
              const std::string& test_id )
{
  std::vector< vidtk::timestamp > input_ts;

  input_ts.push_back( vidtk::timestamp( 1000000, 1 ) );
  input_ts.push_back( vidtk::timestamp( 1110000, 2 ) );
  input_ts.push_back( vidtk::timestamp( 1190000, 3 ) );
  input_ts.push_back( vidtk::timestamp( 1303000, 4 ) );
  input_ts.push_back( vidtk::timestamp( 1400900, 5 ) );
  input_ts.push_back( vidtk::timestamp( 1580000, 6 ) );
  input_ts.push_back( vidtk::timestamp( 1610000, 7 ) );
  input_ts.push_back( vidtk::timestamp( 1720000, 8 ) );
  input_ts.push_back( vidtk::timestamp( 1830000, 9 ) );
  input_ts.push_back( vidtk::timestamp( 1890000, 10 ) );
  input_ts.push_back( vidtk::timestamp( 2010000, 11 ) );

  std::string configure_id = test_id + " configure";
  std::string main_test_id = test_id + " drop percent";

  TEST( configure_id.c_str(), downsampler.configure( settings ), true );

  unsigned drop_counter = 0;

  for( unsigned i = 0; i < input_ts.size(); i++ )
  {
    if( downsampler.drop_current_frame( input_ts[i] ) )
    {
      drop_counter++;
    }
  }

  double drop_percent = static_cast<double>( drop_counter ) / input_ts.size();
  TEST( main_test_id.c_str(), ( 0.5 < drop_percent && drop_percent < 0.8 ), true );
}

}

int test_frame_downsampler( int /*argc*/, char */*argv*/[] )
{
  testlib_test_start( "frame_downsampler" );

  frame_downsampler downsampler;
  frame_downsampler_settings settings;

  settings.rate_limiter_enabled = true;
  settings.rate_threshold = 3.33333;
  settings.initial_ignore_count = 1;
  settings.lock_rate_frame_count = 5;

  run_sim( downsampler, settings, "Rate threshold test" );

  settings.known_fps.resize( 3 );

  settings.known_fps[0] = 5.0;
  settings.known_fps[1] = 10.0;
  settings.known_fps[2] = 15.0;

  run_sim( downsampler, settings, "Rate threshold w/ priors test" );

  return testlib_test_summary();
}
