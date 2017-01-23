/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/frame_rate_estimator.h>

#include <string>
#include <vector>
#include <iostream>

#include <boost/lexical_cast.hpp>

using namespace vidtk;

namespace
{

void run_sim( frame_rate_estimator& estimator,
              const frame_rate_estimator_settings& settings,
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

  std::string configure_id = test_id + " Configure Test";

  TEST( configure_id.c_str(), estimator.configure( settings ), true );

  for( unsigned i = 0; i < input_ts.size(); i++ )
  {
    frame_rate_estimator::output_t estimate = estimator.step( input_ts[i] );

    if( estimate.source_fps > 0 )
    {
      std::string fps_test_id = "FPS Estimate Frame " + boost::lexical_cast<std::string>(i);
      std::string lag_test_id = "Lag Estimate Frame " + boost::lexical_cast<std::string>(i);

      bool in_range = ( 7.4 < estimate.source_fps && estimate.source_fps < 12.6 );
      bool lag_is_positive = ( estimate.estimated_lag >= 0 );

      TEST( fps_test_id.c_str(), in_range, true );
      TEST( lag_test_id.c_str(), lag_is_positive, true );
    }
  }
}

}

int test_frame_rate_estimator( int /*argc*/, char */*argv*/[] )
{
  testlib_test_start( "frame_rate_estimator" );

  frame_rate_estimator estimator;
  frame_rate_estimator_settings settings;

  settings.enable_source_fps_estimator = true;
  settings.enable_local_fps_estimator = true;
  settings.enable_lag_estimator = true;

  settings.fps_estimator_mode = frame_rate_estimator_settings::MEDIAN;
  run_sim( estimator, settings, "Median Estimation" );

  settings.fps_estimator_mode = frame_rate_estimator_settings::MEAN;
  run_sim( estimator, settings, "Mean Estimation" );

  return testlib_test_summary();
}
