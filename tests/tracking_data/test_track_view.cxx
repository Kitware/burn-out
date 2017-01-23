/*ckwg +5
* Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
* KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
* Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
*/


#include <tracking_data/io/track_reader.h>
#include <tracking_data/track.h>
#include <tracking_data/track_view.h>
#include <testlib/testlib_test.h>

#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace
{

void test_track_size(std::string const& name, vidtk::track_view::sptr const& view,
                     std::vector<vidtk::track_state_sptr> const& history)
{
  size_t expected_size = history.size();

  vidtk::track_view::const_iterator_t begin = view->begin();
  vidtk::track_view::const_iterator_t end = view->end();
  TEST_EQUAL((name + " track length").c_str(), std::distance(begin, end), static_cast<int>(expected_size));

  vidtk::track_view::const_reverse_iterator_t rbegin = view->rbegin();
  vidtk::track_view::const_reverse_iterator_t rend = view->rend();
  TEST_EQUAL((name + " track length reversed").c_str(), std::distance(rbegin, rend), static_cast<int>(expected_size));

  TEST_EQUAL((name + " history length").c_str(), view->history().size(), expected_size);

  for (size_t i = 0; i < expected_size; ++i)
  {
    TEST((name + " matching states in histories at " + boost::lexical_cast<std::string>(i)).c_str(),
         view->history()[i], history[i]);
  }
  TEST((name + " history states equal").c_str(), std::equal(history.begin(), history.end(), view->history().begin()), true);

  TEST((name + " first state").c_str(), view->first_state(), history.front());
  TEST((name + " last state").c_str(), view->last_state(), history.back());

  TEST((name + " first state via iterator").c_str(), *begin, history.front());
  TEST((name + " last state via iterator").c_str(), *(end - 1), history.back());

  TEST((name + " first state via reverse_iterator").c_str(), *(rend - 1), history.front());
  TEST((name + " last state via reverse_iterator").c_str(), *rbegin, history.back());
}

}

int test_track_view(int argc, char* argv[])
{
  testlib_test_start("test vidtk track data structure");
  if ( argc < 2 )
  {
    TEST("DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  //Load in kw18 track with pixel information
  std::string dataPath(argv[1]);
  dataPath += "/track_reader_data.kw18";
  std::string imagePath(argv[1]);
  imagePath += "/score_tracks_data/test_frames";
  std::vector< vidtk::track_sptr > tracks;

  boost::scoped_ptr<vidtk::track_reader> reader( new vidtk::track_reader( dataPath.c_str() ) );
  if ( ! reader->open() )
  {
    std::ostringstream oss;
    oss << "Failed to open '" << dataPath << "'";
    TEST( oss.str().c_str(), false, true );
    return EXIT_FAILURE;
  }

  //Test the deep copy of the track
  TEST("Read kw18 file", reader->read_all( tracks ), 3);
  vidtk::track_sptr track_orig = tracks[0];

  // Iterator tests
  // Full track
  {
    vidtk::track_view::sptr full_view = vidtk::track_view::create(track_orig);

    test_track_size("Full", full_view, track_orig->history());
  }

  // Partial track
  {
    vidtk::track_view::sptr partial_view = vidtk::track_view::create(track_orig);
    std::vector<vidtk::track_state_sptr> const& history = track_orig->history();

    std::vector<vidtk::track_state_sptr>::const_iterator begin = history.begin() + 1;
    std::vector<vidtk::track_state_sptr>::const_iterator end = begin + (history.size() - 3);

    vidtk::timestamp begin_ts = (*begin)->time_;
    vidtk::timestamp end_ts = (*end)->time_;

    std::vector<vidtk::track_state_sptr> view_history(begin, end + 1);

    partial_view->set_view_bounds(begin_ts, end_ts);

    test_track_size("Partial", partial_view, view_history);
  }

  // Single frame
  {
    vidtk::track_view::sptr single_frame_view = vidtk::track_view::create(track_orig);
    vidtk::timestamp first_ts = track_orig->first_state()->time_;
    single_frame_view->set_view_bounds(first_ts, first_ts);

    std::vector<vidtk::track_state_sptr> view_history;

    view_history.push_back(track_orig->history().front());

    test_track_size("Single frame", single_frame_view, view_history);
  }

  // Track API
  {
    vidtk::track_view::sptr view = vidtk::track_view::create(track_orig);
    TEST("Track view ID", view->id(), track_orig->id());
    TEST("Track view first state", view->first_state(), track_orig->first_state());
    TEST("Track view last state", view->last_state(), track_orig->last_state());
  }

#define expect_exception(exc, code, desc) \
  do                                      \
  {                                       \
    bool got_exception = false;           \
                                          \
    try                                   \
    {                                     \
      code;                               \
    }                                     \
    catch (exc const& e)                  \
    {                                     \
      e.what();                           \
      got_exception = true;               \
    }                                     \
    catch (...)                           \
    {                                     \
      TEST("Did not receive expected "    \
           "exception: " desc,            \
           false, true);                  \
      got_exception = true;               \
    }                                     \
                                          \
    TEST("Received exception: " desc,     \
        got_exception, true);             \
  } while (false)

  // Invalid bounds
  {
    vidtk::track_view::sptr view = vidtk::track_view::create(track_orig);
    expect_exception(std::runtime_error,
                     view->set_view_start(vidtk::timestamp()),
                     "Setting start to an invalid timestamp");
    expect_exception(std::runtime_error,
                     view->set_view_end(vidtk::timestamp()),
                     "Seting end to an invalid timestamp");
    expect_exception(std::runtime_error,
                     view->set_view_bounds(vidtk::timestamp(), vidtk::timestamp()),
                     "Seting bounds to invalid timestamps");
    expect_exception(std::runtime_error,
                     view->set_view_bounds(track_orig->last_state()->time_,
                                           track_orig->first_state()->time_),
                     "Setting end < start bound timestamps");
  }

  return testlib_test_summary();
} /* test_track_view */
