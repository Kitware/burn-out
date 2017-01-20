/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include <track_oracle/track_oracle.h>
#include <track_oracle/file_format_manager.h>

#include <vgl/vgl_box_2d.h>

#include <boost/thread/thread.hpp>

#include <testlib/testlib_test.h>

#include <logger/logger.h>

#include <boost/thread/mutex.hpp>


using std::ostringstream;
using std::string;

using namespace vidtk;

VIDTK_LOGGER("test_track_oracle_thread_safety");

namespace // anon
{

boost::mutex test_lock;

struct track_stats
{
  size_t n_tracks;
  size_t n_frames;
  double sum_frame_area;
  track_stats(): n_tracks(0), n_frames(0), sum_frame_area(0.0) {}
  explicit track_stats( const track_handle_list_type& tracks );
  void set( const track_handle_list_type& tracks );
  void compare( const track_stats& other, const string& tag ) const;
};

track_stats
::track_stats( const track_handle_list_type& tracks )
{
  this->set( tracks );
}

void
track_stats
::set( const track_handle_list_type& tracks )
{
  this->n_tracks = tracks.size();
  n_frames = 0;
  sum_frame_area = 0.0;
  track_field< vgl_box_2d<double> > bounding_box( "bounding_box" );
  for (size_t i=0; i<this->n_tracks; ++i)
  {
    frame_handle_list_type frames = track_oracle::get_frames( tracks[i] );
    this->n_frames += frames.size();
    for (size_t j=0; j<frames.size(); ++j)
    {
      if (bounding_box.exists( frames[j].row ))
      {
        this->sum_frame_area += bounding_box( frames[j].row ).area();
      }
    }
  }
}

void
track_stats
::compare( const track_stats& other, const string& tag ) const
{
  {
    ostringstream oss;
    oss << tag << " : n_tracks " << this->n_tracks << " matches " << other.n_tracks;
    TEST( oss.str().c_str(), this->n_tracks, other.n_tracks );
  }
  {
    ostringstream oss;
    oss << tag << " : n_frames " << this->n_frames << " matches " << other.n_frames;
    TEST( oss.str().c_str(), this->n_frames, other.n_frames );
  }
  {
    ostringstream oss;
    oss << tag << " : sum_frame_area " << this->sum_frame_area << " matches " << other.sum_frame_area;
    TEST( oss.str().c_str(), this->sum_frame_area, other.sum_frame_area );
  }
}

void
load_test_tracks( const string tag, const string& fn, const track_stats& reference )
{
  track_handle_list_type tracks;
  bool rc = file_format_manager::read( fn, tracks );
  track_stats s( tracks );

  {
    // begin critical section for all threaded console output
    boost::unique_lock< boost::mutex > lock( test_lock );
    TEST( tag.c_str(), rc, true );
    reference.compare( s, tag );
    // end critical section
  }

}

} // anon namespace

int test_track_oracle_thread_safety( int argc, char *argv[])
{
  if(argc < 2)
    {
      LOG_ERROR("Need the data directory as argument\n");
      return EXIT_FAILURE;
    }

  testlib_test_start( "test_track_oracle_thread_safety");

  string dir = argv[1];
  string track_file(dir + "/tracks_filtered.kw18" );

  track_stats reference;
  {
    track_handle_list_type tracks;
    bool rc = file_format_manager::read( track_file, tracks );
    TEST( "Reading single-threaded", rc, true );
    reference.set( tracks );
  }

  const size_t max_threads = 6;  // arbitrary
  for (size_t n_threads = 2; n_threads < max_threads; ++n_threads)
  {
    boost::thread_group threads;
    for (size_t i=1; i<=n_threads; ++i)
    {
      ostringstream oss;
      oss << "Thread " << i << " / " << n_threads;
      threads.add_thread( new boost::thread( load_test_tracks, oss.str(), track_file, reference ));
    }
    threads.join_all();
    ostringstream oss;
    oss << "Sucessfully ran with " << n_threads << " threads";
    TEST( oss.str().c_str(), true, true );
  }

  return testlib_test_summary();
}
