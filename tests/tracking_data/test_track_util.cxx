/*ckwg +5
* Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
* KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
* Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
*/


#include <tracking_data/io/track_reader.h>
#include <tracking_data/track_util.h>
#include <testlib/testlib_test.h>

#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <iterator>

namespace {

// ----------------------------------------------------------------
void test_has_timestamp( vidtk::track::vector_t const& tracks )
{
  vidtk::track_sptr const trk = tracks[1];

  // Test existing state
  vidtk::timestamp ts = trk->history()[4]->get_timestamp();

  TEST( "Has existing timestamp", has_timestamp( trk, ts ), true );

  // test missing state
  ts = tracks[0]->history()[0]->get_timestamp();
  TEST( "Has non-existing timestamp", has_timestamp( trk, ts ), false );

  // test state before start
  ts.set_frame_number( 1000 );
  ts.set_time( trk->last_state()->get_timestamp().time() + 10000 );
  TEST( "Timestamp after last", has_timestamp( trk, ts ), false );

  // test state after end
  ts.set_frame_number( trk->first_state()->get_timestamp().frame_number() - 1 );
  ts.set_time( trk->first_state()->get_timestamp().time() - 10000 );
  TEST( "Timestamp before first", has_timestamp( trk, ts ), false );
}


// ----------------------------------------------------------------
void test_spatial_bounds( vidtk::track::vector_t const& tracks )
{
  vidtk::track_sptr const trk = tracks[0];

  vgl_box_2d< double > result = get_spatial_bounds( trk );
  TEST_NEAR( "Track spatial bounds width", result.width() , 16.763, 0.001 );
  TEST_NEAR( "Track spatial bounds height", result.height() , 3.174, 0.001 );
}


// ----------------------------------------------------------------
void test_truncate_history(vidtk::track::vector_t const& tracks )
{
  // Test trimming history
  vidtk::track_sptr trk = tracks[0]->clone();
  vidtk::timestamp ts = trk->history()[7]->get_timestamp();

  TEST( "Full history", trk->history().size(), 11 );
  TEST( "Truncate operation", truncate_history( trk, ts ), true );
  TEST( "Truncated history", trk->history().size(), 8 );

}

} // end namespace

// ----------------------------------------------------------------
int test_track_util( int argc, char* argv[] )
{
  testlib_test_start( "test vidtk track utilities" );

  if ( argc < 2 )
  {
    TEST( "DATA directory not specified", false, true );
    return EXIT_FAILURE;
  }

  //Load in kw18 track with pixel information
  std::string dataPath( argv[1] );
  dataPath += "/track_reader_data.kw18";

  vidtk::track::vector_t tracks;
  vidtk::track_reader* reader = new vidtk::track_reader( dataPath );

  TEST( "Opening track file", reader->open(), true );
  TEST( "Read kw18 file", reader->read_all( tracks ), 3 );
  TEST( "There are 3 tracks", tracks.size(), 3 );

  /*
  std::cout << *tracks[0] << std::endl;
  std::cout << *tracks[1] << std::endl;
  std::cout << *tracks[2] << std::endl;
  */

  test_has_timestamp( tracks );
  test_spatial_bounds( tracks );
  test_truncate_history( tracks );

  return testlib_test_summary();
}
