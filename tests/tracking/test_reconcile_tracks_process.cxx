/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <tracking/reconcile_tracks_process.h>
#include <tracking/reconcile_tracks_algo.h>

#include <tracking/kw18_reader.h>
#include <tracking/track.h>
#include <tracking/track_view.h>
#include <tracking/tracking_keys.h>

#include <pipeline/async_pipeline.h>

#include <boost/foreach.hpp>
#include <vcl_algorithm.h>


#define ElementsOf(A) ( sizeof(A) / sizeof((A)[0]) )

#if 1

#define DEBUG_PRINT(v,A) do { if ((v) <= verbose) std::cout << A; } while(0)
#define ERROR_PRINT(v,A) do { if ((v) <= verbose) std::cerr << A; } while(0)

#else

#define DEBUG_PRINT(v,A) do { } while (0)
#define ERROR_PRINT(v,A) do { } while (0)

#endif

namespace { // anon

  const  int verbose(0);


// ----------------------------------------------------------------
/** Process data set.
 *
 * The proc_data_type is used to hold process inputs and outputs.
 */
struct proc_data_t
{
  typedef vcl_vector < proc_data_t > vector_t;


#define DATUM_DEF(T, N, I) T N;
  RECONCILER_DATUM_LIST
#undef DATUM_DEF

  vidtk::track_vector_t terminated_tracks;


  // CTOR
  proc_data_t() { }

  // CTOR
  explicit proc_data_t( const vidtk::reconcile_tracks_process& proc)
  {
    this->set_from_proc_outputs( proc );
  }


  void copy_to_proc_inputs( vidtk::reconcile_tracks_process& proc )
  {
#define DATUM_DEF(T, N, I) proc.input_ ## N (N);
  RECONCILER_DATUM_LIST
#undef DATUM_DEF
  }


  void set_from_proc_outputs( const vidtk::reconcile_tracks_process& proc )
  {
#define DATUM_DEF(T, N, I) N = proc.output_ ## N ();
  RECONCILER_DATUM_LIST
#undef DATUM_DEF
  }
};


bool
operator==( const proc_data_t& lhs, const proc_data_t& rhs )
{
  return
#define DATUM_DEF(T, N, I) (lhs.N == rhs.N) &&
  RECONCILER_DATUM_LIST
#undef DATUM_DEF
    true;
}

bool
operator!=( const proc_data_t& lhs, const proc_data_t& rhs )
{
  return ! operator==(lhs, rhs);
}

// ==================================================
//
// Since we have to test the process in an asynchronous setting,
// the proc_data_source and proc_data_sink classes are used
// to supply inputs and collect outputs from the async pipeline.
//
// ==================================================

// ----------------------------------------------------------------
/** Data source process.
 *
 *
 */
struct proc_data_source_process
  : public vidtk::process
{
  // outside the pipeline framework, expose the data buffer
  // and allow the user to load it.  Call data_list().push_back(foo)
  // to load up the source before the pipeline executes.

  proc_data_t::vector_t d;
  proc_data_t::vector_t::iterator d_index;
  proc_data_t::vector_t & data_list() { return d; }

  // inside the pipeline, read data off the list at each step
  // until we hit the end.

  proc_data_t v;

  typedef proc_data_source_process self_type;

  proc_data_source_process( const vcl_string& name )
    : vidtk::process( name, "proc_data_source")
  {
    d_index = d.end();
  }

  // process interface
  vidtk::config_block params() const { return vidtk::config_block(); }
  bool set_params( const vidtk::config_block& ) { return true; }
  bool initialize() { d_index = d.begin(); return true; }

  bool step()
  {
    if (d_index == d.end())
    {
      return false;
    }
    else
    {
      v = *d_index;
      ++d_index;
      return true;
    }
  }

  // -- input ports --
#define DATUM_DEF(T, N, I)  T output_ ## N() const { return v.N; } VIDTK_OUTPUT_PORT (T, output_ ## N);
  RECONCILER_DATUM_LIST // macro magic
#undef DATUM_DEF
};


// ----------------------------------------------------------------
/** Data sink process.
 *
 *
 */
struct proc_data_sink_process
  : public vidtk::process
{
  // outside the pipeline framework, expose the data buffer
  // and allow the user to access it.  Call data_list() to examine
  // values after the pipeline has executed.

  proc_data_t::vector_t d;
  proc_data_t::vector_t const& data_list() const { return d; }
  proc_data_t::vector_t & data_list() { return d; }

  // inside the pipeline, accept and store outputs at each step.

  proc_data_t v;

  typedef proc_data_sink_process self_type;

  proc_data_sink_process( const vcl_string& name )
    : vidtk::process( name, "proc_data_sink")
  {  }

  vidtk::config_block params() const { return vidtk::config_block(); }
  bool set_params( const vidtk::config_block& ) { return true; }
  bool initialize() { d.clear(); return true; }

  bool step()
  {
    d.push_back( v );
    return true;
  }

// -- output ports --
#define DATUM_DEF(T, N, I)  void input_ ## N (T val) { v.N = val; } VIDTK_INPUT_PORT (input_ ## N, T);
  RECONCILER_DATUM_LIST // macro magic
#undef DATUM_DEF

  void input_terminated_tracks (vidtk::track_vector_t const& val) { v.terminated_tracks = val; }
  VIDTK_INPUT_PORT (input_terminated_tracks, vidtk::track_vector_t const& );
};

// ==================================================
//
// Creates a test pipeline with a source, process node, and sink.
//
// ==================================================

struct test_pipeline
{
  vidtk::async_pipeline p;
  vidtk::process_smart_pointer< proc_data_source_process > src;
  vidtk::process_smart_pointer< vidtk::reconcile_tracks_process > put;
  vidtk::process_smart_pointer< proc_data_sink_process > dst;

  test_pipeline()
    : src( new proc_data_source_process( "src") ),
      put( new vidtk::reconcile_tracks_process( "reconciler" )),
      dst( new proc_data_sink_process( "dst" ) )
  {
    p.add( src );
    p.add( put );
    p.add( dst );

#define DATUM_DEF(T, N, I)                                              \
    p.connect( src->output_ ## N ##_port(), put->input_ ## N ##_port() ); \
    p.connect( put->output_ ## N ##_port(), dst->input_ ## N ##_port() );
  RECONCILER_DATUM_LIST // macro magic
#undef DATUM_DEF

    // connect terminated tracks port from put -> dst
    p.connect( put->output_terminated_tracks_port(), dst->input_terminated_tracks_port() );
  }


  bool set_params_and_init( const vidtk::config_block& cfg )
  {
    p.set_params( cfg );
    return p.initialize();
  }


  vidtk::config_block params() const
  {
    return p.params();
  }

};


// ================================================================
// ================================================================

struct input_data
{
  vidtk::timestamp time;
  vidtk::track_view::vector_t tracks;
};

// ----------------------------------------------------------------
/** Globald data area.
 *
 *
 */
struct GLOBALS
{
  // data items
  vidtk::track_vector_t tracks;
  vcl_vector < input_data > input_list;
  vcl_string data_dir;

  /** Initialize global data area.
   *
   * Read in tracks file(s) and build input data set.
   */
  void init( vcl_string const& data)
  {
    this->data_dir = data;

  }


// ----------------------------------------------------------------
/** Load tackds from file(s).
 *
 *
 */
  bool load_tracks (vcl_vector< vcl_string > file_list )
  {
    input_list.clear();
    tracks.clear();

    for (unsigned i = 0; i < file_list.size(); ++i)
    {
      vcl_string full_path = data_dir + "/" + file_list[i];
      vidtk::kw18_reader reader(full_path.c_str());
      vidtk::track_vector_t local_tracks;

      if ( ! reader.read(local_tracks))
      {
        vcl_ostringstream oss;
        oss << "Failed to load '" << full_path << "'";
        TEST(oss.str().c_str(), false, true);
        return ( false );
      }

      DEBUG_PRINT (1, "Loaded " << local_tracks.size() << " tracks from " << full_path << "\n");

      // loop over all tracks - set tracker type
      unsigned tracker_type(1);
      unsigned tracker_subtype(i+1);

      BOOST_FOREACH (vidtk::track_sptr trk, local_tracks)
      {
        trk->data().set( vidtk::tracking_keys::tracker_type,    tracker_type );
        trk->data().set( vidtk::tracking_keys::tracker_subtype, tracker_subtype ); // unique number based on file index
      }

      // Add new elements to our list
      tracks.insert(tracks.end(), local_tracks.begin(), local_tracks.end() );

    } // end for

    DEBUG_PRINT (1, "Loaded " << tracks.size() << " total tracks\n");

    // find lowest frame number and highest frame number.
    unsigned lowest_frame(~0);
    unsigned highest_frame(0);

    BOOST_FOREACH (const vidtk::track_sptr ptr, tracks)
    {
      lowest_frame  = vcl_min (lowest_frame, ptr->first_state()->time_.frame_number() );
      highest_frame = vcl_max (highest_frame, ptr->last_state()->time_.frame_number() );
    }

    // build a map of frames to timestamps
    vcl_map < unsigned, vidtk::timestamp > ts_map;
    for (unsigned frame = lowest_frame; frame < highest_frame; frame++)
    {
      // loop over all tracks
      BOOST_FOREACH (vidtk::track_sptr ptr, tracks)
      {
        // We need a real timestamp from the input set, not just a
        // frame number, so if we havent found one before, look
        // through the current track for the current frame and build
        // a timestamp from that.
        vidtk::timestamp ts = find_timestamp( frame, ptr);
        if (ts.is_valid())
        {
          ts_map[frame] = ts;
          break;
        }
      } // end foreach
    } // end for

    for (unsigned frame = lowest_frame; frame < highest_frame; frame++)
    {
      input_data id;
      vidtk::timestamp ts = ts_map[frame];
      if ( ! ts.is_valid())
      {
        // This happens when there are no tracks for current frame.
        DEBUG_PRINT (3, "No track states found at frame " << frame << "\n");
        continue; // can't work without a timestamp.
      }

      // loop over all tracks
      BOOST_FOREACH (vidtk::track_sptr ptr, tracks)
      {
        // If this track contains current timestamp, then add to input list
        if ( (ptr->first_state()->time_.frame_number() <= frame )
             && (frame <= ptr->last_state()->time_.frame_number()) )
        {
          // Add track view to our set
          vidtk::track_view::sptr tvsp= vidtk::track_view::create(ptr);
          tvsp->set_view_bounds (ptr->first_state()->time_, ts);

          DEBUG_PRINT (3, "Track " << ptr->id() << " from " << ptr->first_state()->time_.frame_number()
                       << " to " << ts.frame_number()
                       << " added to frame " << frame << "\n");
          id.tracks.push_back ( tvsp );
        }

      } // end foreach

      DEBUG_PRINT (3, "at frame " << frame << " there are " << id.tracks.size() << " track views\n");
      id.time = ts;

      // Add input to our list
      input_list.push_back (id);
    } // end for

    DEBUG_PRINT (1, "There are " << input_list.size() << " steps of data\n");
    return ( true );
  }


  // ----------------------------------------------------------------
  // Find timestamp in a track for a frame number
  vidtk::timestamp
  find_timestamp (unsigned frame, const vidtk::track_sptr ptr)
  {
    vcl_vector< vidtk::track_state_sptr > const& hist (ptr->history());
    BOOST_FOREACH (const vidtk::track_state_sptr ptr, hist)
    {
      if (ptr->time_.frame_number() == frame)
      {
        return ptr->time_;
      }
    }

    return vidtk::timestamp();
  }


} G;


// ----------------------------------------------------------------
/** Fill input process buffer.
 *
 * This method fills the input buffer of the input process with all
 * data from the global input list.
 */
size_t
fill_pipeline_input(vidtk::process_smart_pointer< proc_data_source_process > proc)
{
  // int limit(200);

  BOOST_FOREACH (input_data id, G.input_list)
  {
    proc_data_t pd;
    pd.timestamp = id.time;

    vidtk::track_vector_t track_vect;

    // convert from vector of track_views to vector of tracks
    BOOST_FOREACH (vidtk::track_view::sptr tvsp, id.tracks)
    {
      vidtk::track_sptr a_trk = *tvsp;
      track_vect.push_back(a_trk);
    }
    pd.tracks = track_vect;

    proc->d.push_back(pd);

    // if (--limit < 0) break;
  } // end foreach

  return proc->d.size();
}


// ----------------------------------------------------------------
/** Test setting parameters.
 *
 *
 */
void
test_param_sets()
{
  vidtk::process_smart_pointer< vidtk::reconcile_tracks_process > proc (new vidtk::reconcile_tracks_process( "p" ) );


  {
    vidtk::config_block blk = proc->params();
    blk.set ("enabled", "1");
    TEST( "Process: setting params does not trap", proc->set_params( blk ), true );
  }

  {
    test_pipeline p;
    vidtk::config_block blk = p.params();
    blk.set( "reconciler:enabled", "1" );
    TEST( "Test pipeline: setting params does not trap", p.set_params_and_init( blk ), true );
  }
}


// ----------------------------------------------------------------
/** Test process being disabled.
 *
 *
 */
void
test_process_disabled()
{
  test_pipeline p;

  size_t count = fill_pipeline_input (p.src);

  vidtk::config_block blk = p.params();
  blk.set("reconciler:enabled", "0");
  blk.set("reconciler:delay_frames", "20");

  blk.set("reconciler:overlap_fraction", "0.80");
  blk.set("reconciler:state_count_fraction", "0.67");
  blk.set("reconciler:min_track_size", "10");

  TEST("Disabled: param set & init succeeds", p.set_params_and_init(blk), true);
  TEST("Disabled: before run, output set is empty", p.dst->d.empty(), true);

  bool run_status = p.p.run();
  TEST("Disable: pipeline runs", run_status, true);
  bool all_outputs_present = ( p.dst->d.size() == count );
  {
    vcl_ostringstream oss;
    oss << "Disable: pipeline output has " << p.dst->d.size() << " (should be " << count << ")";
    TEST(oss.str().c_str(), all_outputs_present, true);
  }
}


// ----------------------------------------------------------------
/** Results from a test.
 *
 *
 */
struct test_result {
  size_t input_count;
  size_t output_count;

  size_t input_track_count;
  size_t output_track_count;
  size_t terminated_track_count;

};

// ----------------------------------------------------------------
/** Test processing tracks
 *
 *
 */
void
test_processing_tracks(test_result & res)
{
  test_pipeline p;

  res.input_count = fill_pipeline_input (p.src);
  res.input_track_count = 0;
  res.output_track_count = 0;
  res.terminated_track_count = 0;

  vidtk::config_block blk = p.params();
  blk.set("reconciler:enabled", "1");
  blk.set("reconciler:verbose", "0");
  blk.set("reconciler:delay_frames", "10");

  blk.set("reconciler:overlap_fraction", "0.50");
  blk.set("reconciler:state_count_fraction", "0.70");
  blk.set("reconciler:min_track_size", "8");

  TEST("Processing: param set & init succeeds", p.set_params_and_init(blk), true);
  TEST("Processing: before run, output set is empty", p.dst->d.empty(), true);

  proc_data_t::vector_t & src_data = p.src->data_list();
  proc_data_t::vector_t & dst_data = p.dst->data_list();

  bool run_status = p.p.run();
  TEST("Processing: pipeline runs", run_status, true);
  bool all_outputs_present = ( p.dst->d.size() == res.input_count );
  {
    vcl_ostringstream oss;
    oss << "Processing: pipeline output has " << p.dst->d.size() << " (should be " << res.input_count << ")";
    TEST(oss.str().c_str(), all_outputs_present, true);
  }

  res.output_count = p.dst->d.size();

  // Test output to see if it is same as input.
  for (size_t i = 0; i < res.input_count; i++)
  {
    if (src_data[i].timestamp != dst_data[i].timestamp)
    {
      vcl_ostringstream oss;
      oss << "Data compare error at element " << i;
      TEST(oss.str().c_str(), false, true);
      break;
    }
  } // end for

    // compute output track count
  for (size_t i = 0; i < res.input_count; i++)
  {
    res.input_track_count += src_data[i].tracks.size();
    res.output_track_count += dst_data[i].tracks.size();
    res.terminated_track_count += dst_data[i].terminated_tracks.size();

    if (verbose >= 3)
    {
      if (src_data[i].timestamp != dst_data[i].timestamp)
      {
        vcl_cout << "Timestamp mismatch at step: " << i
                 << "Src " << src_data[i].timestamp
                 << "Dst " << dst_data[i].timestamp
                 << vcl_endl;
      }

      if (src_data[i].tracks.size() != dst_data[i].tracks.size() )
      {
        vcl_cout << "Step: " << i << " at frame " << src_data[i].timestamp.frame_number()
                 << " input tracks: " << src_data[i].tracks.size()
                 << " output tracks: " << dst_data[i].tracks.size()
                 << vcl_endl;

        vcl_cout << "Input tracks: ";
        BOOST_FOREACH (const vidtk::track_sptr ptr, src_data[i].tracks )
        {
          vcl_cout << ptr->id() << " ";
        }  vcl_cout << vcl_endl;

        vcl_cout << "Output tracks: ";
        BOOST_FOREACH (const vidtk::track_sptr ptr, dst_data[i].tracks )
        {
          vcl_cout << ptr->id() << " ";
        }  vcl_cout << vcl_endl;

        vcl_cout << "Terminated tracks: ";
        BOOST_FOREACH (const vidtk::track_sptr ptr, dst_data[i].terminated_tracks )
        {
          vcl_cout << ptr->id() << " ";
        }  vcl_cout << vcl_endl;
      }
    }
  } // end for

  if (verbose >= 1)
  {
    vcl_cout << "input tracks count: " << res.input_track_count
             << "    output tracks count: " << res.output_track_count
             << "    terminated tracks count: " << res.terminated_track_count
             << vcl_endl;
  }
}


} // end namespace


// ----------------------------------------------------------------
/** Main test driver
 *
 *
 */
int test_reconcile_tracks_process( int argc, char* argv[] )
{
  testlib_test_start( "reconcile tracks process" );
  if ( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    G.init( argv[1] );

    // basic sanity tests
    test_param_sets();

    // Read tracks file(s)
    vcl_vector < vcl_string > file_list;
    file_list.push_back ("reconcile_tracks_1.kw18");

    bool data_ready;

    data_ready = G.load_tracks(file_list);
    TEST( "Data loaded", data_ready, true );
    if (data_ready)
    {
      test_result res;

      // basic sanity tests
      test_process_disabled();
      test_processing_tracks(res);
    }

    file_list.clear();

    file_list.push_back ("reconcile_tracks_1.kw18");
    file_list.push_back ("reconcile_tracks_1.kw18");

    data_ready = G.load_tracks(file_list);
    TEST( "Data loaded", data_ready, true );
    if (data_ready)
    {
      test_result res;

      test_processing_tracks(res);
      //TEST( "Input tracks count", 12644, res.input_track_count);
      //TEST( "Output tracks count", 6317, res.output_track_count);
      //TEST( "Terminated tracks count", 130, res.terminated_track_count);
    }


#if 0
    file_list.clear();
    file_list.push_back ("reconcile_tracks_2.kw18");
    file_list.push_back ("reconcile_tracks_3.kw18");
    data_ready = G.load_tracks(file_list);
    TEST( "Data loaded", data_ready, true );
    if (data_ready)
    {
      test_processing_tracks();

    }
#endif


  }
  return testlib_test_summary();
}
