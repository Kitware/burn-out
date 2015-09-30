/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_sstream.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vnl/vnl_double_3.h>

#include <boost/bind.hpp>
#include <tracking/klt_tracker_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


vcl_string g_data_dir;


inline double sqr( double x ) { return x*x; }


void
step( vidtk::klt_tracker_process& tp,
      unsigned frame_number,
      vcl_string const& filename )
{
  vidtk::timestamp ts;
  ts.set_frame_number( frame_number );
  vil_image_view<vxl_byte> img = vil_load( (g_data_dir+filename).c_str() );
  vcl_cout << "Processing " << filename << "\n";
  if( !img )
  {
    TEST( "File load", false, true );
  }
  else
  {
    tp.set_timestamp( ts );
    tp.set_image( img );
    TEST( "Step process", tp.step(), true );
  }
}


template<class TrkContainer, class Comparison>
void
verify_length( TrkContainer const& trks,
               Comparison const& comp,
               vcl_string const& msg )
{
  typedef typename TrkContainer::const_iterator iter_type;

  bool good = true;
  unsigned cnt = 0;
  for( iter_type it = trks.begin(); it != trks.end(); ++it, ++cnt )
  {
    if( ! comp( (*it)->history().size() ) )
    {
      vcl_cout << "Track " << cnt << " (id="
               << (*it)->id() << ") has length "
               << (*it)->history().size() << "\n";
      good = false;
    }
  }
  TEST( msg.c_str(), good, true );
}


template<class TrkContainer>
void
verify_length_at_least( TrkContainer const& trks,
                        unsigned min_len )
{
  std::ostringstream msg;
  msg << "Track length is at least " << min_len;
  verify_length( trks,
                 boost::bind( std::not2( std::less<unsigned>() ), _1, min_len ),
                 msg.str() );
}


template<class TrkContainer>
void
verify_length_at_most( TrkContainer const& trks,
                       unsigned max_len )
{
  std::ostringstream msg;
  msg << "Track length is at most " << max_len;
  verify_length( trks,
                 boost::bind( std::not2( std::greater<unsigned>() ), _1, max_len ),
                 msg.str() );
}


bool
test_translation( vidtk::track_sptr const& trk,
                  double dx, double dy )
{
  bool good = true;
  if( trk->history().size() < 2 )
  {
    vcl_cout << "Track length < 2; no delta available\n";
    good = false;
  }
  else
  {
    vnl_double_3 loc0 = trk->history()[ trk->history().size()-2 ]->loc_;
    vnl_double_3 loc1 = trk->history()[ trk->history().size()-1 ]->loc_;
    vnl_double_3 delta = loc1-loc0;
    if( loc0[2] != 0 || loc1[2] != 0 )
    {
      vcl_cout << "Location has non-zero Z component\n"
               << "  loc0 = " << loc0 << "\n"
               << "  loc1 = " << loc1 << "\n";
      good = false;
    }
    else
    {
      vnl_double_2 err( delta[0]-dx, delta[1]-dy );
      double dist = err.magnitude();
      if( dist > 0.5 )
      {
        vcl_cout << "Delta test:\n"
                 << "  loc0 = " << loc0 << "\n"
                 << "  loc1 = " << loc1 << "\n"
                 << "  delta = " << delta << "\n"
                 << "  expected = " << dx << " " << dy << "\n"
                 << "  error = " << err << " (mag=" << dist << ")" << "\n";
        good = false;
      }
    }
  }

  return good;
}


template<class TrkContainer>
void
test_translation( TrkContainer const& trks,
                  double dx, double dy )
{
  typedef typename TrkContainer::const_iterator iter_type;

  bool good = true;
  unsigned cnt = 0;
  for( iter_type it = trks.begin(); it != trks.end(); ++it, ++cnt )
  {
    if( ! test_translation( *it, dx, dy ) )
    {
      vcl_cout << "(this was for track " << cnt
               << ", id=" << (*it)->id() << ")\n";
      good = false;
    }
  }
  vcl_ostringstream msg;
  msg << "Track last delta is (" << dx << "," << dy << ")";
  TEST( msg.str().c_str(), good, true );
}




bool
test_velocity( vidtk::track_sptr const& trk,
                  double vx, double vy )
{
  bool good = true;
  if( trk->history().size() < 1 )
  {
    vcl_cout << "Track length < 1; no velocity available\n";
    good = false;
  }
  else
  {
    vnl_double_3 vel = trk->last_state()->vel_;
    if( vel[2] != 0 )
    {
      vcl_cout << "Velocity has non-zero Z component\n"
               << "  vel = " << vel << "\n";
      good = false;
    }
    else
    {
      vnl_double_2 err( vel[0]-vx, vel[1]-vy );
      double dist = err.magnitude();
      if( dist > 0.5 )
      {
        vcl_cout << "Delta test:\n"
                 << "  vel = " << vel << "\n"
                 << "  expected = " << vx << " " << vy << "\n"
                 << "  error = " << err << " (mag=" << dist << ")" << "\n";
        good = false;
      }
    }
  }

  return good;
}


template<class TrkContainer>
void
test_velocity( TrkContainer const& trks,
                  double vx, double vy )
{
  typedef typename TrkContainer::const_iterator iter_type;

  bool good = true;
  unsigned cnt = 0;
  for( iter_type it = trks.begin(); it != trks.end(); ++it, ++cnt )
  {
    if( ! test_velocity( *it, vx, vy ) )
    {
      vcl_cout << "(this was for track " << cnt
               << ", id=" << (*it)->id() << ")\n";
      good = false;
    }
  }
  vcl_ostringstream msg;
  msg << "Track last delta is (" << vx << "," << vy << ")";
  TEST( msg.str().c_str(), good, true );
}


void
test_expected_translation()
{
  vcl_cout << "\n\n\nTest image translation\n\n";

  using namespace vidtk;

  klt_tracker_process tp( "klt" );

  config_block blk = tp.params();
  blk.set( "num_features", "10" );
  blk.set( "replace_lost", "false" );
  blk.set( "verbosity_level", "1" );

  TEST( "Set parameters", tp.set_params( blk ), true );
  TEST( "Initialize", tp.initialize(), true );

  {
    step( tp, 0, "square.png" );
    TEST( "4 features found", tp.active_tracks().size(), 4 );
    TEST( "4 tracks created", tp.created_tracks().size(), 4 );
  }

  {
    step( tp, 1, "square_trans+7+4.png" );
    TEST( "Still have 4 features", tp.active_tracks().size(), 4 );
    TEST( "0 tracks created", tp.created_tracks().size(), 0 );
    verify_length_at_least( tp.active_tracks(), 2 );
    test_translation( tp.active_tracks(), 7, 4 );
  }

  {
    step( tp, 2, "square_trans+13-1.png" );
    TEST( "Still have 4 features", tp.active_tracks().size(), 4 );
    verify_length_at_least( tp.active_tracks(), 3 );
    test_translation( tp.active_tracks(), 6, -5 );
  }

  {
    step( tp, 3, "square_trans+13-1.png" );
    TEST( "Still have 4 features", tp.active_tracks().size(), 4 );
    verify_length_at_least( tp.active_tracks(), 4 );
    test_translation( tp.active_tracks(), 0, 0 );
  }

  {
    step( tp, 4, "square_trans+7+4.png" );
    TEST( "Still have 4 features", tp.active_tracks().size(), 4 );
    verify_length_at_least( tp.active_tracks(), 5 );
    test_translation( tp.active_tracks(), -6, 5 );
  }

  {
    step( tp, 5, "square_trans+13-1_3cnr.png" );
    TEST( "Only have 3 features", tp.active_tracks().size(), 3 );
    verify_length_at_least( tp.active_tracks(), 6 );
    test_translation( tp.active_tracks(), 6, -5 );
    TEST( "1 track stopped", tp.terminated_tracks().size(), 1 );
    verify_length_at_most( tp.terminated_tracks(), 5 );
  }
}



void
test_feature_replacement()
{
  vcl_cout << "\n\n\nTest feature replacement\n\n";

  using namespace vidtk;

  klt_tracker_process tp( "klt" );

  config_block blk = tp.params();
  blk.set( "num_features", "10" );

  TEST( "Set parameters", tp.set_params( blk ), true );
  TEST( "Initialize", tp.initialize(), true );

  {
    step( tp, 0, "square.png" );
    TEST( "4 features found", tp.active_tracks().size(), 4 );
    TEST( "4 tracks created", tp.created_tracks().size(), 4 );
  }

  {
    step( tp, 1, "square_trans+7+4.png" );
    TEST( "Still have 4 features", tp.active_tracks().size(), 4 );
    verify_length_at_least( tp.active_tracks(), 2 );
    test_translation( tp.active_tracks(), 7, 4 );
    TEST( "No new tracks created", tp.created_tracks().size(), 0 );
  }

  {
    step( tp, 2, "square_trans+13-1_3cnr.png" );
    TEST( "Have 5 features", tp.active_tracks().size(), 5 );
    verify_length_at_least( tp.active_tracks(), 1 );
    TEST( "1 track stopped", tp.terminated_tracks().size(), 1 );
    verify_length_at_most( tp.terminated_tracks(), 2 );
    TEST( "2 new track created", tp.created_tracks().size(), 2 );
  }

  {
    step( tp, 5, "square_trans+13-1_3cnr.png" );
    TEST( "Still have 5 features", tp.active_tracks().size(), 5 );
    verify_length_at_least( tp.active_tracks(), 2 );
    TEST( "0 tracks stopped", tp.terminated_tracks().size(), 0 );
    TEST( "0 new tracks created", tp.created_tracks().size(), 0 );
  }
}


void
test_double_delete()
{
  vcl_cout << "\n\n\nTest double delete\n\n";

  using namespace vidtk;

  klt_tracker_process tp( "klt" );

  config_block blk = tp.params();
  blk.set( "num_features", "10" );
  blk.set( "replace_lost", "false" );

  TEST( "Set parameters", tp.set_params( blk ), true );
  TEST( "Initialize", tp.initialize(), true );

  {
    step( tp, 0, "square.png" );
    TEST( "4 features found", tp.active_tracks().size(), 4 );
    TEST( "4 tracks created", tp.created_tracks().size(), 4 );
  }

  {
    step( tp, 1, "square.png" );
    TEST( "Still have 4 features", tp.active_tracks().size(), 4 );
    TEST( "No new tracks created", tp.created_tracks().size(), 0 );
    TEST( "No tracks terminated", tp.terminated_tracks().size(), 0 );
  }

  {
    step( tp, 2, "black.png" );
    TEST( "Have 0 features", tp.active_tracks().size(), 0 );
    TEST( "No new tracks created", tp.created_tracks().size(), 0 );
    TEST( "4 tracks terminated", tp.terminated_tracks().size(), 4 );
  }

  {
    step( tp, 3, "square.png" );
    TEST( "Have 0 features", tp.active_tracks().size(), 0 );
    TEST( "No new tracks created", tp.created_tracks().size(), 0 );
    TEST( "No tracks terminated", tp.terminated_tracks().size(), 0 );
  }
}



void
test_velocity()
{
  vcl_cout << "\n\n\nTest velocity\n\n";

  using namespace vidtk;

  klt_tracker_process tp( "klt" );

  config_block blk = tp.params();
  blk.set( "num_features", "5" );

  TEST( "Set parameters", tp.set_params( blk ), true );
  TEST( "Initialize", tp.initialize(), true );

  {
    step( tp, 0, "corner.png" );
    TEST( "1 feature found", tp.active_tracks().size(), 1 );
  }

  {
    step( tp, 1, "corner_trans+5+8.png" );
    TEST( "Still have 1 features", tp.active_tracks().size(), 1 );
    test_velocity( tp.active_tracks(), 5, 8 );
  }

  {
    step( tp, 3, "corner.png" ); // two time units
    TEST( "Still have 1 features", tp.active_tracks().size(), 1 );
    test_velocity( tp.active_tracks(), -2.5, -4.0 );
  }
}




} // end anonymous namespace

int test_klt_tracker_process( int argc, char* argv[] )
{
  testlib_test_start( "KLT process" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    g_data_dir = argv[1];
    g_data_dir += "/";

    test_expected_translation();
    test_feature_replacement();
    test_velocity();
    test_double_delete();
  }

  return testlib_test_summary();
}
