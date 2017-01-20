/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/io/track_reader.h>
#include <tracking_data/io/track_writer_db.h>
#include <tracking_data/io/track_writer_vsl.h>
#include <tracking_data/io/track_writer_kw18_col.h>
#include <tracking_data/io/track_writer_mit.h>
#include <tracking_data/io/track_writer_shape.h>
#include <tracking_data/io/track_writer_4676.h>
#include <tracking_data/io/track_writer_oracle.h>
#include <tracking_data/io/track_writer.h>
#include <tracking_data/io/track_writer_process.h>
#include <tracking_data/tracking_keys.h>
#include <utilities/geo_lat_lon.h>
#include <testlib/testlib_test.h>

#include <vul/vul_file.h>
#include <vul/vul_file_iterator.h>

#include <track_oracle/track_kwxml/track_kwxml.h>
#include <track_oracle/track_kwxml/file_format_kwxml.h>
#include <track_oracle/track_kwiver/file_format_kwiver.h>
#include <track_oracle/track_csv/file_format_csv.h>

#include <stdio.h>

typedef unsigned long long ts_type;

using namespace vidtk;

namespace
{

  // ------------------------------------------------------------------
void check_oracle_tracks(const std::string&               fname,
                         const file_format_base&          reader,
                         const std::vector< track_sptr >& vidtk_tracks )
{
  track_handle_list_type tracks;
  track_kwxml_type kwxml;
  reader.read( fname, tracks );

  {
    std::ostringstream oss;
    oss << ": Read " << tracks.size() << " tracks; expected " << vidtk_tracks.size();
    TEST( oss.str().c_str(), tracks.size(), vidtk_tracks.size() );
    if ( tracks.size() != vidtk_tracks.size() ) return;
  }

  for ( size_t i = 0; i < vidtk_tracks.size(); ++i )
  {
    unsigned id = vidtk_tracks[i]->id();
    oracle_entry_handle_type h = kwxml.external_id.lookup( id, DOMAIN_ALL );
    bool found_track = ( h != INVALID_ROW_HANDLE );
    {
      std::ostringstream oss;
      oss << ": Track " << i << ": file contained track " << id;
      TEST( oss.str().c_str(), found_track, true );
    }
    if ( ! found_track ) continue;

    track_handle_type trk( h );
    frame_handle_list_type frames = track_oracle::get_frames( trk );
    const std::vector< track_state_sptr >& hist = vidtk_tracks[i]->history();
    {
      std::ostringstream oss;
      oss << ": Track " << i << ": found " << frames.size() << " frames; expected " << hist.size();
      TEST( oss.str().c_str(), frames.size(), hist.size() );
    }
    if ( frames.size() != hist.size() ) continue;

    // we can't assume the frames are in the same order
    for ( size_t j = 0; j < hist.size(); ++j )
    {
      const track_state_sptr& state = hist[ j ];
      const timestamp& ts = state->time_;
      // assert that we have frame numbers; only test if false to avoid clutter
      if ( ! ts.has_frame_number() )
      {
        TEST( "Source track has frame numbers", ts.has_frame_number(), true );
        return;
      }

      // lookup via frame number

      unsigned fn = ts.frame_number();
      oracle_entry_handle_type fh = kwxml.frame_number.lookup( fn, trk );
      bool found_frame = ( fh != INVALID_ROW_HANDLE );
      {
        std::ostringstream oss;
        oss << "Track " << i << ": frame " << fn << " found";
        TEST( oss.str().c_str(), found_frame, true );
      }

      if ( ! found_frame ) continue;

      frame_handle_type f( fh );
      // check timestamps (assumes the frequency kw18 option wasn't used)
      ts_type vidtk_ts = static_cast< ts_type > ( ts.time_in_secs() * 1000 * 1000 );
      {
        std::ostringstream oss;
        oss << ": Track " << i << ": frame " << fn << " : timestamp was "
            << kwxml[ f ].timestamp_usecs()
            << "; expected " << vidtk_ts;
        TEST( oss.str().c_str(), vidtk_ts, kwxml[ f ].timestamp_usecs() );
      }
    }
  }
} // check_kwxml_tracks


#define TEST_TOLERANCE(X,Y) if(std::abs((X) - (Y)) > tol) return false

// ----------------------------------------------------------------
bool test_track_size_and_id( std::vector< track_sptr > const& read_tracks,
                             std::vector< track_sptr > const& gt_tracks,
                             double                           tol,
                             bool                             utm,
                             bool                             lat_lon )
{
  if ( read_tracks.size() != gt_tracks.size() )
  {
    std::cerr << "Different number of tracks: " << read_tracks.size() << " " << gt_tracks.size() << std::endl;
    return false;
  }

  for ( unsigned int i = 0; i < read_tracks.size(); ++i )
  {
    track_sptr rt = read_tracks[i];
    track_sptr gt = gt_tracks[i];

    if ( ( gt->id() != rt->id() ) || ( gt->history().size() != rt->history().size() ) )
    {
      std::cerr << "IDs or history size do not match: "
                << gt->id() << " " << rt->id() << "\t"
                << gt->history().size() << " " << rt->history().size() << std::endl;
      return false;
    }

    for ( unsigned int j = 0; j < gt->history().size(); ++j )
    {
      track_state_sptr rts = rt->history()[j];
      track_state_sptr gts = gt->history()[j];

      if ( utm )
      {
        geo_coord::geo_UTM geo;
        gts->get_smoothed_loc_utm( geo );
        vnl_vector_fixed< double, 2 > vel;
        bool r = gts->get_utm_vel( vel );
        if ( r && geo.is_valid() )
        {
          TEST_TOLERANCE( rts->loc_[0], geo.get_easting() );
          TEST_TOLERANCE( rts->loc_[1], geo.get_northing() );
          TEST_TOLERANCE( rts->vel_[0], vel[0] );
          TEST_TOLERANCE( rts->vel_[1], vel[1] );
        }
        else
        {
          return false;
        }
      }
      else
      {
        TEST_TOLERANCE( rts->loc_[0], gts->loc_[0] );
        TEST_TOLERANCE( rts->loc_[1], gts->loc_[1] );
        TEST_TOLERANCE( rts->vel_[0], gts->vel_[0] );
        TEST_TOLERANCE( rts->vel_[1], gts->vel_[1] );
      }
      if ( lat_lon )
      {
        vidtk::geo_coord::geo_lat_lon gt_ll, rt_ll;
        gts->latitude_longitude( gt_ll );
        rts->latitude_longitude( rt_ll );
        TEST_TOLERANCE( gt_ll.get_longitude(), rt_ll.get_longitude() );
        TEST_TOLERANCE( gt_ll.get_latitude(), rt_ll.get_latitude() );
      }
      else
      {
        double gtp[2], rtp[2];
        if ( ! gts->get_image_loc( gtp[0], gtp[1] ) || ! rts->get_image_loc( rtp[0], rtp[1] ) )
        {
          return false;
        }
        TEST_TOLERANCE( gtp[0], rtp[0] );
        TEST_TOLERANCE( gtp[1], rtp[1] );
      }
    }
  }
  return true;
} // test_track_size_and_id


// ----------------------------------------------------------------
bool test_track_size_and_id( std::vector< track_sptr > const& read_tracks,
                             std::vector< track_sptr > const& gt_tracks,
                             double                           tol )
{
  if ( read_tracks.size() != gt_tracks.size() )
  {
    return false;
  }
  for ( unsigned int i = 0; i < read_tracks.size(); ++i )
  {
    track_sptr rt = read_tracks[i];
    track_sptr gt = gt_tracks[i];
    if ( ( gt->id() != rt->id() ) || ( gt->history().size() != rt->history().size() ) )
    {
      return false;
    }
    for ( unsigned int j = 0; j < gt->history().size(); ++j )
    {
      track_state_sptr rts = rt->history()[j];
      track_state_sptr gts = gt->history()[j];
      geo_coord::geo_UTM geo_gt, geo_rt;
      gts->get_smoothed_loc_utm( geo_gt );
      rts->get_smoothed_loc_utm( geo_rt );
      vnl_vector_fixed< double, 2 > vel_gt, vel_rt;
      bool r = gts->get_utm_vel( vel_gt ) && rts->get_utm_vel( vel_rt );
      if ( r && geo_gt.is_valid() && geo_rt.is_valid() )
      {
        TEST_TOLERANCE( geo_gt.get_easting(), geo_rt.get_easting() );
        TEST_TOLERANCE( geo_gt.get_northing(), geo_rt.get_northing() );
        TEST_TOLERANCE( vel_gt[0], vel_rt[0] );
        TEST_TOLERANCE( vel_gt[1], vel_rt[1] );
      }
      else
      {
        return false;
      }
      TEST_TOLERANCE( rts->loc_[0], gts->loc_[0] );
      TEST_TOLERANCE( rts->loc_[1], gts->loc_[1] );
      TEST_TOLERANCE( rts->vel_[0], gts->vel_[0] );
      TEST_TOLERANCE( rts->vel_[1], gts->vel_[1] );

      vidtk::geo_coord::geo_lat_lon gt_ll, rt_ll;
      gts->latitude_longitude( gt_ll );
      rts->latitude_longitude( rt_ll );
      TEST_TOLERANCE( gt_ll.get_longitude(), rt_ll.get_longitude() );
      TEST_TOLERANCE( gt_ll.get_latitude(), rt_ll.get_latitude() );

      double gtp[2], rtp[2];
      if ( ! gts->get_image_loc( gtp[0], gtp[1] ) || ! rts->get_image_loc( rtp[0], rtp[1] ) )
      {
        return false;
      }
      TEST_TOLERANCE( gtp[0], rtp[0] );
      TEST_TOLERANCE( gtp[1], rtp[1] );
    }
  }
  return true;
} // test_track_size_and_id


// ----------------------------------------------------------------
void test_kw18_writing( std::vector< track_sptr > & tracks )
{
  //Test default classical version
  {
    vidtk::track_writer_kw18_col writer(vidtk::track_writer_kw18_col::KW20);
    std::string fname = "test.defaults.kw18";
    TEST("can open: ", writer.open(fname), true);
    TEST("is open: ", writer.is_open(), true);
    TEST("is good: ", writer.is_good(), true);
    TEST("can write: ", writer.write(tracks), true);
    writer.close();

    vidtk::track_reader reader("test.defaults.kw18");
    TEST("Opening kw18 file", reader.open(), true);

    std::vector< track_sptr > trks;
    TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
    TEST("Correct number and size of tracks: ", test_track_size_and_id(trks, tracks, 0.00001, false, false), true );
  }

  //Test world location version
  {
    vidtk::track_writer_kw18_col writer(vidtk::track_writer_kw18_col::KW20);

    track_writer_options two;
    two.suppress_header_ = false;
    two.x_offset_ = 0;
    two.y_offset_ = 0;
    two.write_lat_lon_for_world_ = true;
    two.write_utm_ = true;
    two.frequency_ = 0;
    writer.set_options( two );

    std::string fname = "test.kw18";
    TEST("can open: ", writer.open(fname), true);
    TEST("is open: ", writer.is_open(), true);
    TEST("is good: ", writer.is_good(), true);
    TEST("can write: ", writer.write(tracks), true);
    image_object_sptr io = new image_object();
    timestamp ts;
    TEST("Writing image objects is a no-op: ", writer.write(io,ts), true); //reading should result in failure if this happens
    writer.close();
    vidtk::track_reader reader("test.kw18");

    vidtk::ns_track_reader::track_reader_options opt;
    opt.set_read_lat_lon_for_world(true);
    reader.update_options( opt );
    TEST("Opening kw18 file", reader.open(), true);

    std::vector< track_sptr > trks;
    TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
    TEST("Correct number and size of tracks: ", test_track_size_and_id(trks, tracks, 0.00001, true, true), true );
  }

  //Test bad arguments
  {
    vidtk::track_writer_kw18_col writer(vidtk::track_writer_kw18_col::KW20);
    std::string fname = "BAD_DIRECTORY_DOES_NOT_EXIST/test.kw18";
    TEST("cannot open: ", writer.open(fname), false);
    TEST("is open: ", writer.is_open(), false);
    TEST("is good: ", writer.is_good(), false);
    TEST("write: ", writer.write(tracks[0]), false);
    fname = "test_1.kw18";
    writer.open(fname);
    fname = "test_2.kw18";
    TEST("squentia open test: ", writer.open(fname), true);
    TEST("Null write fails: ", writer.write(NULL), false);
  }
}


// ----------------------------------------------------------------
void test_col_writing( std::vector< track_sptr > & tracks )
{
  {
    vidtk::track_writer_kw18_col writer(vidtk::track_writer_kw18_col::COL);
    std::string fname = "test.obj_col";
    TEST("can open: ", writer.open(fname), true);
    TEST("is open: ", writer.is_open(), true);
    TEST("is good: ", writer.is_good(), true);
    TEST("can write: ", writer.write(tracks), true);
    track_state_sptr tss = tracks[0]->history()[0];
    image_object_sptr image_object;
    TEST("Able to get image object", tss->image_object( image_object ), true);
    TEST("Able to write image object", writer.write(image_object,tss->time_), true);
    writer.close();

    vidtk::track_reader reader("test.obj_col");
    TEST("Opening kw18 file", reader.open(), true);

    std::vector< track_sptr > trks;
    TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
    TEST("should have one more track than truth:", trks.size(), tracks.size()+1);
    trks.pop_back();
    TEST("Correct number and size of tracks: ", test_track_size_and_id(trks, tracks, 0.00001, false, false), true );
  }

  //test bad
  {
    vidtk::track_writer_kw18_col writer(vidtk::track_writer_kw18_col::COL);
    std::string fname = "BADD_DIRECTORY_DOES_NOT_EXIST/test.kw18";
    TEST("cannot open: ", writer.open(fname), false);
    TEST("is open: ", writer.is_open(), false);
    TEST("is good: ", writer.is_good(), false);
    TEST("write: ", writer.write(tracks[0]), false);
    track_state_sptr tss = tracks[0]->history()[0];
    image_object_sptr image_object;
    TEST("Able to get image object", tss->image_object( image_object ), true);
    TEST("Not able to write image object", writer.write(image_object,tss->time_), false);
    fname = "test_2.kw18";
    writer.open(fname);
    TEST("Null write fails: ", writer.write(NULL), false);
    image_object = NULL;
    TEST("Able to get image object", writer.write( image_object, tss->time_ ), false);
  }
}


// ----------------------------------------------------------------
void test_kwxml_writing( std::vector< track_sptr > const& tracks )
{
  // all defaults
  {
    vidtk::track_writer_oracle writer( "kwxml");

    track_writer_options two;
    two.x_offset_ = 0;
    two.y_offset_ = 0;
    two.frequency_ = 0.0;
    two.oracle_format_ = "kwxml";

    writer.set_options( two );

    std::string fname = "test.kwxml";
    TEST( "kwxml: Before open: is open: ", writer.is_open(), false );
    TEST( "kwxml: Before open: is good: ", writer.is_good(), false );
    TEST( "kwxml: can open: ", writer.open( fname ), true );
    TEST( "kwxml: After open: is open: ", writer.is_open(), true );
    TEST( "kwxml: After open: is good: ", writer.is_good(), true );
    TEST( "kwxml: Can write: ", writer.write( tracks ), true );
    writer.close();
    TEST( "kwxml: After close: is open: ", writer.is_open(), false );
    TEST( "kwxml: After close: is good: ", writer.is_good(), false );
    vidtk::file_format_kwxml reader;
    check_oracle_tracks( fname, reader, tracks );
  }
}

void test_kwiver_writing( const std::vector< track_sptr >& tracks )
{
  // all defaults
  {
    vidtk::track_writer_oracle writer( "kwiver", 0, 0, 0.0) ;
    std::string fname = "test.kwiver";
    TEST( "kwiver: Before open: is open: ", writer.is_open(), false );
    TEST( "kwiver: Before open: is good: ", writer.is_good(), false );
    TEST( "kwiver: can open: ", writer.open( fname ), true );
    TEST( "kwiver: After open: is open: ", writer.is_open(), true );
    TEST( "kwiver: After open: is good: ", writer.is_good(), true );
    TEST( "kwiver: Can write: ", writer.write( tracks ), true );
    writer.close();
    TEST( "kwiver: After close: is open: ", writer.is_open(), false );
    TEST( "kwiver: After close: is good: ", writer.is_good(), false );
    vidtk::file_format_kwiver reader;
    check_oracle_tracks( fname, reader, tracks );
  }
}

void test_kwcsv_writing( const std::vector< track_sptr >& tracks )
{
  // all defaults
  {
    vidtk::track_writer_oracle writer( "csv", 0, 0, 0.0) ;
    std::string fname = "test.kwcsv";
    TEST( "kwcsv: Before open: is open: ", writer.is_open(), false );
    TEST( "kwcsv: Before open: is good: ", writer.is_good(), false );
    TEST( "kwcsv: can open: ", writer.open( fname ), true );
    TEST( "kwcsv: After open: is open: ", writer.is_open(), true );
    TEST( "kwcsv: After open: is good: ", writer.is_good(), true );
    TEST( "kwcsv: Can write: ", writer.write( tracks ), true );
    writer.close();
    TEST( "kwcsv: After close: is open: ", writer.is_open(), false );
    TEST( "kwcsv: After close: is good: ", writer.is_good(), false );
    vidtk::file_format_csv reader;
    check_oracle_tracks( fname, reader, tracks );
  }
}


// ----------------------------------------------------------------
void test_vsl_writing( std::vector< track_sptr > const& tracks )
{
  //Test default classical version
  {
    timestamp ts;
    vidtk::track_writer_vsl writer;
    std::string fname = "test.vsl";
    TEST("can open: ", writer.open(fname), true);
    TEST("is open: ", writer.is_open(), true);
    TEST("is good: ", writer.is_good(), true);
    TEST("can write: ", writer.write(tracks, NULL, ts), true);
    writer.close();

    vidtk::track_reader reader("test.vsl");
    TEST("Opening vsl file", reader.open(), true);

    std::vector< track_sptr > trks;
    TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
    TEST("Correct number and size of tracks: ", test_track_size_and_id(trks, tracks, 0.00001), true );
  }

  //Test write image object vector location version
  {
    vidtk::track_writer_vsl writer;
    std::string fname = "test.vsl";
    TEST("can open: ", writer.open(fname), true);
    TEST("is open: ", writer.is_open(), true);
    TEST("is good: ", writer.is_good(), true);
    image_object_sptr io = new image_object();
    timestamp ts;
    track_state_sptr tss = tracks[0]->history()[0];
    image_object_sptr image_object;
    TEST("Able to get image object", tss->image_object( image_object ), true);
    std::vector<image_object_sptr> ios(1,image_object);
    TEST("Writing image objects is a no-op: ", writer.write(tracks,&ios,ts), true);
    writer.close();

    vidtk::track_reader reader("test.vsl");
    TEST("Opening vsl file", reader.open(), true);

    std::vector< track_sptr > trks;
    TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
    TEST("Correct number and size of tracks: ", test_track_size_and_id(trks, tracks, 0.00001), true );
  }

  //Test bad arguments
  {
    timestamp ts;
    vidtk::track_writer_vsl writer;
    std::string fname = "BADD_DIRECTORY_DOES_NOT_EXIST/test.vsl";
    TEST("cannot open: ", writer.open(fname), false);
    TEST("is open: ", writer.is_open(), false);
    TEST("is good: ", writer.is_good(), false);
    TEST("write: ", writer.write(tracks, NULL, ts), false);
    TEST("write: ", writer.write(tracks), false);
    track_state_sptr tss = tracks[0]->history()[0];
    image_object_sptr image_object;
    TEST("Able to get image object", tss->image_object( image_object ), true);
    std::vector< image_object_sptr> temp(1,image_object);
    TEST("write: ", writer.write(temp,ts), false);
    fname = "test_1.vsl";
    writer.open(fname);
    fname = "test_2.vsl";
    TEST("squentia open test: ", writer.open(fname), true);
  }
}


// ----------------------------------------------------------------
void test_mit_writing( std::string dir, std::vector< track_sptr > & tracks )
{
  //Test default classical version
  {
    timestamp ts;
    vidtk::track_writer_mit writer(dir);
    std::string fname = "test.mit";
    TEST("can open: ", writer.open(fname), true);
    TEST("is open: ", writer.is_open(), true);
    TEST("is good: ", writer.is_good(), true);
    TEST("can write: ", writer.write(tracks), true);
    writer.close();

    vidtk::track_reader reader("test.mit");
    TEST("Opening mit file", reader.open(), true);

    std::vector< track_sptr > trks;
    TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
    TEST("Correct number of tracks: ", trks.size(), tracks.size() );
  }

  //Test bad arguments
  {
    timestamp ts;
    vidtk::track_writer_vsl writer;
    std::string fname = "BADD_DIRECTORY_DOES_NOT_EXIST/test.mit";
    TEST("cannot open: ", writer.open(fname), false);
    TEST("is open: ", writer.is_open(), false);
    TEST("is good: ", writer.is_good(), false);
    TEST("write: ", writer.write(tracks), false);
    track_state_sptr tss = tracks[0]->history()[0];
  }
}


// ----------------------------------------------------------------
void test_shape_writing( std::vector< track_sptr > & tracks )
{
#ifdef SHAPELIB_ENABLED
  //Test default classical version
  std::string glob_str = "testing_shape_*.shp";
  //clean up old xml files if they exist
  for(vul_file_iterator iter = glob_str ; iter; ++iter)
  {
    remove(iter.filename());
  }
  {
    timestamp ts;
    std::string dir = vul_file::get_cwd();
    vidtk::track_writer_shape writer(true, dir, "testing_shape_");
    TEST("can write: ", writer.write(tracks), true);
    unsigned int count = 0;
    for(vul_file_iterator iter = glob_str ; iter; ++iter)
    {
      count++;
    }
    std::cout << count << std::endl;
    TEST_EQUAL("Correct number of shape files ", count, tracks.size());
  }
#else
  (void)tracks;
#endif
}

// ----------------------------------------------------------------
void test_4676_NITS(vidtk::track::vector_t const& tracks, bool has_lat_lon)
{
  std::string glob_str = "*writing_test_1*.xml";
  //clean up old xml files if they exist
  for(vul_file_iterator iter = glob_str ; iter; ++iter)
  {
    remove(iter.filename());
  }
  track_writer_4676 writer( track_writer_4676::NITS);

  track_writer_options two;
  two.name_ = "writing_test_1";
  two.output_dir_ = vul_file::get_cwd();

  writer.set_options( two );

  std::string donotcare;
  TEST("can open: ", writer.open(donotcare), true);
  TEST("is open: ", writer.is_open(), true);
  TEST("is good: ", writer.is_good(), true);
  track_sptr track0 = tracks[0];
  track_sptr track1 = tracks[3];
  unsigned int wrote = 0;
  std::vector< track_state_sptr >::const_iterator other = track1->history().begin();
  for(std::vector< track_state_sptr >::const_iterator iter = track0->history().begin(); iter != track0->history().end(); ++iter)
  {
    wrote++;
    std::vector<track_sptr> temp_v;
    temp_v.push_back(track0->get_subtrack( track0->first_state()->time_, (*iter)->time_ ));
    temp_v.push_back(track1->get_subtrack( track1->first_state()->time_, (*iter)->time_ ));
    TEST("Test Write", writer.write(temp_v, NULL, (*iter)->time_), has_lat_lon);

    while( (*other)->time_ <= (*iter)->time_ ) other++;
  }
  for( ;other != track1->history().end() && has_lat_lon; ++other)
  {
    wrote++;
    std::vector<track_sptr> temp_v;
    temp_v.push_back(track1->get_subtrack( track1->first_state()->time_, (*other)->time_ ));
    TEST("Test Write", writer.write(temp_v, NULL, (*other)->time_), has_lat_lon);
  }


  unsigned int count = 0;
  for(vul_file_iterator iter = glob_str ; iter; ++iter)
  {
    count++;
  }
  if(has_lat_lon)
  {
    TEST_EQUAL("Correct number of 4676 files ", count, wrote);
  }
}


// ----------------------------------------------------------------
void test_4676_spade(vidtk::track::vector_t const& tracks, bool has_lat_lon)
{
  std::string glob_str = "*writing_test_2*.xml";
  //clean up old xml files if they exist
  for(vul_file_iterator iter = glob_str ; iter; ++iter)
  {
    remove(iter.filename());
  }
  track_writer_4676 writer( track_writer_4676::SPADE );

  track_writer_options two;
  two.name_ = "writing_test_2";
  two.output_dir_ = vul_file::get_cwd();

  writer.set_options( two );
  std::string donotcare;
  TEST("can open: ", writer.open(donotcare), true);
  TEST("is open: ", writer.is_open(), true);
  TEST("is good: ", writer.is_good(), true);
  track_sptr track0 = tracks[0];
  track_sptr track1 = tracks[3];
  unsigned int wrote = 0;
  std::vector< track_state_sptr >::const_iterator other = track1->history().begin();
  for(std::vector< track_state_sptr >::const_iterator iter = track0->history().begin(); iter != track0->history().end(); ++iter)
  {
    wrote++;
    std::vector<track_sptr> temp_v;
    temp_v.push_back(track0->get_subtrack( track0->first_state()->time_, (*iter)->time_ ));
    temp_v.push_back(track1->get_subtrack( track1->first_state()->time_, (*iter)->time_ ));
    TEST("Test Write", writer.write(temp_v, NULL, (*iter)->time_), has_lat_lon);

    while( (*other)->time_ <= (*iter)->time_ ) other++;
  }
  for( ;other != track1->history().end(); ++other)
  {
    wrote++;
    std::vector<track_sptr> temp_v;
    temp_v.push_back(track1->get_subtrack( track1->first_state()->time_, (*other)->time_ ));
    TEST("Test Write", writer.write(temp_v, NULL, (*other)->time_), has_lat_lon);
  }

  unsigned int count = 0;
  for(vul_file_iterator iter = glob_str ; iter; ++iter)
  {
    count++;
  }
  if(has_lat_lon)
  {
    TEST_EQUAL("Correct number of 4676 files ", count, wrote);
  }
}


// ----------------------------------------------------------------
void test_track_writer(vidtk::track::vector_t & tracks)
{
  track_writer writer;
  TEST("cannot open: ", writer.open(""), false);
  TEST("is open: ", writer.is_open(), false);
  TEST("is good: ", writer.is_good(), false);
  TEST("write: ", writer.write(tracks), false);

  TEST("BAD format: ", writer.set_format("BAD"), false);
  TEST("BAD Extension: ", writer.open("tracks.BAD"), false);
  track_writer_options options;
  TEST("Good format: ", writer.set_format("xml"), true);
  writer.set_options(options);
  TEST("Good format: ", writer.set_format("vsl"), true);
  writer.set_options(options);
  TEST("Good format: ", writer.set_format("4676"), true);
  writer.set_options(options);

#ifdef SHAPELIB_ENABLED
  TEST("Good format: ", writer.set_format("shape"), true);
#else
  TEST("Good format: ", writer.set_format("shape"), false);
#endif

  writer.set_options(options);
  TEST("Good format: ", writer.set_format("kw18"), true);
  writer.set_options(options);
  TEST("can open: ", writer.open("tracks1.kw18"), true);
  TEST("is open: ", writer.is_open(), true);
  TEST("is good: ", writer.is_good(), true);
  TEST("can write: ", writer.write(tracks), true);
  writer.close();

  vidtk::track_reader reader("tracks1.kw18");
  TEST("Opening kw18 file", reader.open(), true);

  std::vector< track_sptr > trks;
  TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
  TEST("Correct number and size of tracks: ", test_track_size_and_id(trks, tracks, 0.00001, false, false), true );
}

// ----------------------------------------------------------------
void test_db_writing(std::vector< track_sptr > & /*tracks*/)
{
#ifdef HAS_CPPDB
  // Test state before anything
  {
    vidtk::track_writer_db writer;
    TEST("Blank state | is_open: ", writer.is_open(), false);
    TEST("Blank state | is_good: ", writer.is_good(), false);
  }

  //Test bad database type
  {
    vidtk::track_writer_db writer;

    track_writer_options options;
    options.db_type_ = "wrong_db";
    options.db_user_ = "wrong_user";
    options.db_user_ = "wrong_pw";
    options.db_host_ = "wrong_host";
    options.db_port_ = "wrong_port";
    writer.set_options(options);
    TEST("Wrong parameters | open: ", writer.open("wrong_name"), false);
    writer.close();
  }

  //Test bad database type
  {
    vidtk::track_writer_db writer;

    track_writer_options options;
    options.db_type_ = "wrong_db";
    options.db_user_ = "wrong_user";
    options.db_user_ = "wrong_pw";
    options.db_host_ = "wrong_host";
    options.db_port_ = "wrong_port";
    writer.set_options(options);
    TEST("Wrong parameters | open: ", writer.open("wrong_name"), false);
    writer.close();
  }

  // This is only basic testing. For more testing see test_writer_db
#endif
}

// ----------------------------------------------------------------
void test_track_writer_process(vidtk::track::vector_t & tracks)
{
  remove("writer_process_test.kw18");
  //Test bad configuration
  {
    track_writer_process writer("writer");
    config_block blk = writer.params();
    blk.set("disabled", "BOB");
    TEST("Test bad disable: ", writer.set_params(blk), false);
    blk.set("disabled", "false");
    TEST("Test good disable: ", writer.set_params(blk), true);
    blk.set("format", "BAD_FORMAT");
    TEST("Test bad format: ", writer.set_params(blk), false);
    blk.set("format", "kw18");
    TEST("Test good format: ", writer.set_params(blk), true);
    blk.set("filename", "BAD_FILE_NAME_DIR/BAD_FILE.kw18");
    TEST("Test bad file name config: ", writer.set_params(blk), true);
    TEST("Test bad file name init: ", writer.initialize(), false);
  }
  //Test disabled
  {
    track_writer_process writer("writer");
    config_block blk = writer.params();
    blk.set("disabled", "true");
    TEST("Test set parms disabled: ", writer.set_params(blk), true);
    TEST("Test init disabled: ", writer.initialize(), true);
    TEST("Test step disabled: ", writer.step2(), process::FAILURE);
    TEST("Test is disabled: ", writer.is_disabled(), true);
  }
  //Test writing
  {
    track_writer_process writer("writer");
    config_block blk = writer.params();
    blk.set("disabled", "false");
    blk.set("format", "kw18");
    blk.set("filename", "writer_process_test.kw18");
    TEST("Test writing config: ", writer.set_params(blk), true);
    TEST("Test writing init: ", writer.initialize(), true);
    std::vector<image_object_sptr> objs;
    timestamp ts;
    writer.set_tracks(tracks);
    writer.set_image_objects(objs);
    writer.set_timestamp(ts);
    TEST("Test step: ", writer.step2(), process::SUCCESS);

    vidtk::track_reader reader("writer_process_test.kw18");
    TEST("Opening kw18 file", reader.open(), true);

    std::vector< track_sptr > trks;
    TEST("can read written tracks: ", reader.read_all(trks) > 0, true);
    TEST("Correct number and size of tracks: ", test_track_size_and_id(trks, tracks, 0.00001, false, false), true );
  }
  //Test overwriting
  {
    track_writer_process writer("writer");
    config_block blk = writer.params();
    blk.set("disabled", "false");
    blk.set("format", "kw18");
    blk.set("filename", "writer_process_test.kw18");
    blk.set("overwrite_existing", "false");
    TEST("Test no overwrite config: ", writer.set_params(blk), true);
    TEST("Test no overwrite init: ", writer.initialize(), false);
    blk.set("overwrite_existing", "true");
    TEST("Test overwrite config: ", writer.set_params(blk), true);
    TEST("Test overwrite init: ", writer.initialize(), true);
  }
  //Test NULL tracks
  {
    track_writer_process writer("writer");
    config_block blk = writer.params();
    blk.set("disabled", "false");
    blk.set("format", "kw18");
    blk.set("filename", "writer_process_test.kw18");
    blk.set("overwrite_existing", "true");
    TEST("Test writing config: ", writer.set_params(blk), true);
    TEST("Test writing init: ", writer.initialize(), true);
    TEST("Test step disabled: ", writer.step2(), process::FAILURE);
  }
}

}


// ----------------------------------------------------------------
int test_writing_tracks( int argc, char *argv[] )
{
  testlib_test_start( "test writers" );
  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }
  //read_tracks
  vidtk::track::vector_t tracks_all;
  {
    std::string vslPath(argv[1]);
    vslPath += "/lair_test_tracks_with_ll.vsl";
    vidtk::track::vector_t tracks;

    vidtk::track_reader reader(vslPath.c_str());
    TEST("Opening vsl file", reader.open(), true);

    unsigned int i = 0;
    unsigned int frame;
    for(; reader.read_next_terminated(tracks, frame); ++i )
    {
      tracks_all.insert(tracks_all.end(), tracks.begin(), tracks.end());
    }
  }
  std::cout << tracks_all.size() << std::endl;

  test_kw18_writing(tracks_all);
  test_col_writing(tracks_all);
  test_kwxml_writing( tracks_all );
  test_kwiver_writing( tracks_all );
  test_kwcsv_writing( tracks_all );
  test_vsl_writing(tracks_all);
  test_mit_writing(argv[1], tracks_all);
  test_shape_writing( tracks_all );
  test_4676_NITS(tracks_all, true);
  test_4676_spade(tracks_all, true);
  test_db_writing(tracks_all);
  test_track_writer(tracks_all);
  test_track_writer_process(tracks_all);

  tracks_all.clear();
  //test tracks with out lat lon
  {
    std::string vslPath(argv[1]);
    vslPath += "/lair_test_tracks.vsl";
    vidtk::track::vector_t tracks;

    vidtk::track_reader reader(vslPath.c_str());
    TEST("Opening vsl file", reader.open(), true);

    unsigned int i = 0;
    unsigned int frame;
    for(; reader.read_next_terminated(tracks, frame); ++i )
    {
      tracks_all.insert(tracks_all.end(), tracks.begin(), tracks.end());
    }
  }

  test_kw18_writing(tracks_all);
  test_col_writing(tracks_all);
  test_vsl_writing(tracks_all);
  test_mit_writing(argv[1], tracks_all);
  test_shape_writing( tracks_all );
  test_4676_NITS(tracks_all, false);
  test_4676_spade(tracks_all, false);
  test_db_writing(tracks_all);
  test_track_writer(tracks_all);
  test_track_writer_process(tracks_all);
  return testlib_test_summary();
}
