/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <testlib/testlib_test.h>

#include <tracking_data/io/active_tracks_generator.h>

#include <tracking_data/io/track_reader.h>
#include <tracking_data/track.h>

#include <algorithm>


using namespace vidtk;

namespace { // anonymous

bool load_tracks( std::string const& full_path, vidtk::track_vector_t& tracks )
{
  vidtk::track_reader reader( full_path );
  if (! reader.open() )
  {
    std::ostringstream oss;
    oss << "Failed to open '" << full_path << "'";
    TEST( oss.str().c_str(), false, true );
    return false;
  }

  if (reader.read_all( tracks ) == 0)
  {
    std::ostringstream oss;
    oss << "Failed to load '" << full_path << "'";
    TEST( oss.str().c_str(), false, true );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
/** Simple test
 *
 *
 */
void test1 ( vidtk::track_vector_t const& tracks)
{
  vidtk::active_tracks_generator gen;

  gen.add_tracks (tracks);
  gen.create_active_tracks();

  // std::cout << "Frame range: " << gen.first_frame() << " to " << gen.last_frame() << std::endl;

  // dataset specific values
  TEST("First frame as expected", gen.first_frame(), 3);
  TEST("Last frame as expected",  gen.last_frame(), 2608);
}


// ----------------------------------------------------------------
/** Test timestamp map
 *
 *
 */
void test2 ( vidtk::track_vector_t const& tracks)
{
  vidtk::active_tracks_generator gen;

  gen.add_tracks (tracks);
  gen.create_active_tracks();

  // Test timestamp estimation
  unsigned first_frame = gen.first_frame();
  unsigned last_frame = gen.last_frame();

  unsigned full_count(0);
  unsigned pass_count(0);
  unsigned real_count(0);
  unsigned est_count(0);

  double min_dt(1e9), max_dt(0);
  for (unsigned i = first_frame; i < last_frame; i++)
  {
      full_count++;
    vidtk::timestamp ts, ets;
    if ( gen.timestamp_for_frame(i, ts) )
    {
      // we have a real timestamp - test estimated ts
      real_count++;
      gen.timestamp_for_frame(i, ets, true);

      double dt =  ((ets.time() - ts.time()) * 1e-6); // account for uSec scaling
      min_dt = std::min (min_dt, dt);
      max_dt = std::max (max_dt, dt);
      if (dt < 0.1)
      {
        pass_count++;
      }
      else
      {
        // std::cout << "Real ts: " << ts << std::endl << "Estimated ts: " << ets << std::endl;

        // std::cout << "Time diff: " << ((ets.time() - ts.time()) * 1e-6) << std::endl;
        TEST("Estimated timestamp is close enough to real", false, true);
      }
    }
    else
    {
      est_count++; // estimated timestamp
    }


  } // end for

  // std::cout << "Timestamps requestes: " << full_count << std::endl;
  // std::cout << "Real ts: " << real_count << std::endl;
  // std::cout << "Estaimated ts: " << est_count << std::endl;
  TEST("All timestamps account for", full_count, (real_count + est_count) );

  // std::cout << "min_dt: " << min_dt << "    max_dt: " << max_dt << std::endl;
  TEST("Estimated timestamp generation accuracy", real_count, pass_count);

}



} // end namespace


// ----------------------------------------------------------------
/** Main test driver
 *
 *
 */
int test_active_tracks_generator( int argc, char * argv[])
{
  std::string data_dir;

  testlib_test_start( "active tracks generator" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    data_dir = argv[1];
    data_dir += "/";

    vidtk::track_vector_t tracks;

    TEST("Load first set of tracks", load_tracks (data_dir + "reconcile_tracks_1.kw18", tracks), true);

    test1 (tracks);
    test2 (tracks);

  }

    // -- last but not least --
  return testlib_test_summary();

}
