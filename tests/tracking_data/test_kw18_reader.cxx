/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <tracking_data/io/track_reader.h>
#include <testlib/testlib_test.h>
#include <vul/vul_temp_filename.h>
#include <fstream>
#include <stdio.h>


namespace {  // anon


  // scoped file
class scoped_file
{
public:
  scoped_file()
    : file_name_( vul_temp_filename() )
  {
    this->open();
  }

  scoped_file( std::string const& fn )
    : file_name_( fn )
  {
    this->open();
  }

  ~scoped_file()
  {
    // close if open
    if ( this->stream_.is_open() )
    {
      this->stream_.close();
    }

    // delete file
    remove( this->file_name_.c_str() );
  }

  std::ostream& operator()() { return this->stream_; }
  bool operator!() const { return ! this->stream_.is_open(); }
  void flush() { this->stream_.flush(); }
  std::string const& filename() { return this->file_name_; }
  void close() { this->stream_.close(); }
  bool is_open() const { return ! this->stream_.is_open(); }

private:

  void open()
  {
    this->stream_.open( this->file_name_.c_str(), std::ios_base::out );
    if ( ! this->stream_ )
    {
    std::ostringstream oss;
    oss << "Failed to open temp file '" << this->file_name_ << "'";
    TEST( oss.str().c_str(), false, true );
    }
  }


  std::string file_name_;
  std::ofstream stream_;
};


// ----------------------------------------------------------------
// Read file with no header, embedded blanks and comments
void test_1( char* path )
{
  std::string xmlPath( path );
  xmlPath += "/kw18-test.trk";
  vidtk::track_reader reader( xmlPath );
  if ( ! reader.open() )
  {
    std::ostringstream oss;
    oss << "test_1: Failed to open '" << xmlPath << "'";
    TEST( oss.str().c_str(), false, true );
    return;
  }

  vidtk::track::vector_t tracks;

  TEST( "Read kw18 file", reader.read_all( tracks ), 301 );

  TEST( "There are 301 Tracks", tracks.size(), 301 );
  TEST( "Track 3 ID is 800006", tracks[3]->id(), 800006 );
  TEST( "Track 8 has 33 frames", tracks[8]->history().size(), 33 );
  TEST( "Track 250 Frame 2 Loc X", tracks[250]->history()[2]->loc_[0], 197.768 );
  TEST( "Track 250 Frame 2 Loc Y", tracks[250]->history()[2]->loc_[1], 171.715 );
  TEST( "Track 250 Frame 2 Loc Z", tracks[250]->history()[2]->loc_[2], 0.0 );

  TEST_NEAR( "Track 250 Frame 2 Vel X", tracks[250]->history()[2]->vel_[0], 0.31962200000000002, 1e-6 );
  TEST_NEAR( "Track 250 Frame 2 Vel Y", tracks[250]->history()[2]->vel_[1], 0.10968700000000001, 1e-6 );
  TEST_NEAR( "Track 250 Frame 2 Vel Z", tracks[250]->history()[2]->vel_[2], 0.0, 1e-6 );

  TEST( "Track 250 Frame 2 min_x", tracks[250]->history()[2]->amhi_bbox_.min_x(), 507 );
  TEST( "Track 250 Frame 2 min_y", tracks[250]->history()[2]->amhi_bbox_.min_y(), 42 );
  TEST( "Track 250 Frame 2 max_x", tracks[250]->history()[2]->amhi_bbox_.max_x(), 519 );
  TEST( "Track 250 Frame 2 max_y", tracks[250]->history()[2]->amhi_bbox_.max_y(), 73 );
}


// ----------------------------------------------------------------
// Test insufficient images for frames
void test_2( char* path )
{
  std::string xmlPath( path );
  xmlPath += "/kw18-test.trk";
  vidtk::track_reader reader( xmlPath );

  vidtk::ns_track_reader::track_reader_options opt;
  std::string img_path( path );
  opt.set_path_to_images( img_path + "/aph_imgs0000" );
  reader.update_options( opt );

  std::ostringstream oss;
  oss << "test_2: Failed to open insufficient image files '" << xmlPath << "'";
  TEST( oss.str().c_str(), reader.open(), false );
}


// ----------------------------------------------------------------
// Valid filename, missing image file.
void test_2a( char* path )
{
  std::string xmlPath( path );
  xmlPath += "/kw18-test.trk";
  vidtk::track_reader reader( xmlPath );

  vidtk::ns_track_reader::track_reader_options opt;
  std::string img_path( path );
  opt.set_path_to_images( img_path + "/bad_image_file" ); // not an image file
  reader.update_options( opt );

  std::ostringstream oss;
  oss << "test_2a: Failed to open missing image file '" << xmlPath << "'";
  TEST( oss.str().c_str(), reader.open(), false );
}


// ----------------------------------------------------------------
// Valid filename, not an image file.
void test_2b( char* path )
{
  scoped_file image_file( "bad_image_file.png" );
  if ( ! image_file ) { return; }

  image_file() << "this is not an image file\n";
  image_file.close();

  std::string xmlPath( path );
  xmlPath += "/kw18-test.trk";
  vidtk::track_reader reader( xmlPath );

  vidtk::ns_track_reader::track_reader_options opt;
  opt.set_path_to_images( image_file.filename() );
  reader.update_options( opt );

  std::ostringstream oss;
  oss << "test_2b: Failed to open bad image file'" << xmlPath << "'";
  TEST( oss.str().c_str(), reader.open(), false );
}


// ----------------------------------------------------------------
// Test reading a file with one track, one state, no header.
void test_3( char* /* path */ )
{
  scoped_file track_file;
  if ( ! track_file ) { return; }

  track_file() <<     "1 3 1 165 255 0 0 165 255 165 255 175 265 100 0 0 0 .5 -1 -1\n";
  track_file.close();

  vidtk::track_reader reader( track_file.filename() );

  if ( ! reader.open() )
  {
    std::ostringstream oss;
    oss << "Failed to open one line file '" << track_file.filename() << "'";
    TEST( oss.str().c_str(), false, true );
    return;
  }

  vidtk::track::vector_t tracks;
  TEST( "Read kw18 file", reader.read_all( tracks ), 1 );
}


// ----------------------------------------------------------------
// Test reading missing file
void test_3a( char* path )
{
  std::string xmlPath( path );
  xmlPath += "/missing-file.kw18"; // correct extension
  vidtk::track_reader reader( xmlPath );

  std::ostringstream oss;
  oss << "Failed to open missing kw18 file '" << xmlPath << "'";
  TEST( oss.str().c_str(), reader.open(), false );
}


// ----------------------------------------------------------------
// Test reading a file with one track, bad record
// Fails during open
void test_4( char* /*path*/ )
{
  // create bad tracks file
  scoped_file bad_tracks;
  if ( ! bad_tracks ) { return; }

  bad_tracks() <<
    "# 1:Track-id  2:Track-length  3:Frame-number  4-5:Tracking-plane-loc(x,y)  "
    "6-7:velocity(x,y)  8-9:Image-loc(x,y)  10-13:Img-bbox(TL_x,TL_y,BR_x,BR_y)  "
    "14:Area  15-17:World-loc(longitude,latitude,0-when-available,444-otherwise)  "
    "18:timesetamp  19:object-type-id  20:activity-type-id\n"
    "1 3 1 165 255 0 0 165 255 165 255 175 265 100 0 0 0 .5 -1 -1\n"
    "1 3 2 165 255 0 0 1 5 255 165 255 175 265 100 0 0 0 .5 -1 -1\n"
    "1 3 3 165 255 0 0 165 255 165 255 175 265 100 0 0 0 .5 -1 -1\n";
  bad_tracks.close();

  vidtk::track_reader reader( bad_tracks.filename() );

  std::ostringstream oss;
  oss << "Failed to open file - bad record '" << bad_tracks.filename() << "'";
  TEST( oss.str().c_str(), reader.open(), false );
}


// ----------------------------------------------------------------
// Test reading a file with one track, first record bad
// Fails during open, file type check
void test_4a( char* /*path*/ )
{
  // create bad tracks file
  scoped_file bad_tracks;
  if (! bad_tracks ) { return; }

  bad_tracks() <<
    "# 1:Track-id  2:Track-length  3:Frame-number  4-5:Tracking-plane-loc(x,y)  "
    "6-7:velocity(x,y)  8-9:Image-loc(x,y)  10-13:Img-bbox(TL_x,TL_y,BR_x,BR_y)  "
    "14:Area  15-17:World-loc(longitude,latitude,0-when-available,444-otherwise)  "
    "18:timesetamp  19:object-type-id  20:activity-type-id\n"
    "1 3 1 165 255 0 0 xxx 255 165 255 175 265 100 0 0 0 .5 -1 -1\n"
    "1 3 2 165 255 0 0 165 255 165 255 175 265 100 0 0 0 .5 -1 -1\n"
    "1 3 3 165 255 0 0 165 255 165 255 175 265 100 0 0 0 .5 -1 -1\n";
  bad_tracks.close();

  vidtk::track_reader reader( bad_tracks.filename() );

  std::ostringstream oss;
  oss << "Failed to open file - bad first record '" << bad_tracks.filename() << "'";
  TEST( oss.str().c_str(), reader.open(), false );
}


// ----------------------------------------------------------------
// Test reading a file with one track, first record bad
// Fails during open, file type check, no datarecords
void test_4b( char* /*path*/ )
{
  // create bad tracks file
  scoped_file bad_tracks;
  if (! bad_tracks ) { return; }

  bad_tracks() <<
    "# 1:Track-id  2:Track-length  3:Frame-number  4-5:Tracking-plane-loc(x,y)  "
    "6-7:velocity(x,y)  8-9:Image-loc(x,y)  10-13:Img-bbox(TL_x,TL_y,BR_x,BR_y)  "
    "14:Area  15-17:World-loc(longitude,latitude,0-when-available,444-otherwise)  "
    "18:timesetamp  19:object-type-id  20:activity-type-id\n";

  bad_tracks.close();

  vidtk::track_reader reader( bad_tracks.filename() );

  std::ostringstream oss;
  oss << "Failed to open, no file records '" << bad_tracks.filename() << "'";
  TEST( oss.str().c_str(), reader.open(), false );
}


// ----------------------------------------------------------------
void test_clif( char* path )
{
  // clif track file has trailing whitespace
  std::string clif_path = std::string( path ) + "/clif.trk";
  vidtk::track::vector_t tracks;
  vidtk::track_reader reader( clif_path );

  if ( ! reader.open() )
  {
    std::ostringstream oss;
    oss << "Failed to open '" << clif_path << "'";
    TEST( oss.str().c_str(), false, true );
    return;
  }

  if ( reader.read_all( tracks ) == 43 )
  {
    std::ostringstream oss;
    oss << "Expected 43 tracks; found " << tracks.size();
    TEST( oss.str().c_str(), tracks.size(), 43 );
  }
}


// ----------------------------------------------------------------
// test for read next terminated.
void test_next_terminated( char * path )
{
  std::string xmlPath( path );
  xmlPath += "/kw18-test.trk";
  vidtk::track_reader reader( xmlPath );
  if ( ! reader.open() )
  {
    std::ostringstream oss;
    oss << "test_next_terminated: Failed to open '" << xmlPath << "'";
    TEST( oss.str().c_str(), false, true );
    return;
  }

  vidtk::track::vector_t tracks;
  unsigned frame(0);
  int count(0);

  reader.read_next_terminated( tracks, frame );
  TEST( "Frame 41", (41 == frame && tracks.size() == 1), true );

  reader.read_next_terminated( tracks, frame );
  TEST( "Frame 44", (44 == frame && tracks.size() == 1), true );

  reader.read_next_terminated( tracks, frame );
  TEST( "Frame 64", (64 == frame && tracks.size() == 1), true );

  while ( reader.read_next_terminated( tracks, frame ) )
  {
    count++;
    // std::cout << "There are " << tracks.size() << " tracks on frame " << frame << std::endl;

    // spot check some frames
#define TF(F,T)     if (F == frame) TEST( "Frame " # F, tracks.size(), T)
    TF(788, 2);
    TF(833, 1);
    TF(1171, 2);
    TF(1432, 1);
    TF(1433, 2);
    TF(2878, 7);
    TF(3008, 1);
    TF(3018, 7);
  } // end while

  TEST("Last frame", frame, 3018);
  TEST("Frame count", count, 263 );
}

} // end anon ns


// ----------------------------------------------------------------
int test_kw18_reader( int argc, char *argv[] )
{
  testlib_test_start( "test kw18 reader" );
  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  test_1( argv[1] );

  test_2( argv[1] );
  test_2a( argv[1] );
  test_2b( argv[1] );

  test_3( argv[1] );
  test_3a( argv[1] );

  test_4( argv[1] );
  test_4a( argv[1] );
  test_4b( argv[1] );

  test_clif( argv[1] );
  test_next_terminated( argv[1] );

  return testlib_test_summary();
}
