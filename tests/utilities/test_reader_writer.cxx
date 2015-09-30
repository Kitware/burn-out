/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>
#include <vcl_sstream.h>
#include <vil/vil_image_view.h>

#include <utilities/gsd_reader_writer.h>
#include <utilities/timestamp_reader_writer.h>
#include <utilities/homography_reader_writer.h>
#include <utilities/video_metadata_reader_writer.h>
#include <utilities/video_modality_reader_writer.h>
#include <utilities/shot_break_flags_reader_writer.h>
#include <utilities/image_view_reader_writer.h>

#include <utilities/group_data_reader_writer.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


#if 0
//
// Test compile of these templates
//
void junk()
{
  vcl_ifstream in;
  image_to_image_homography_reader_writer i2i(0);
  i2i.read_object(in);

  image_to_plane_homography_reader_writer i2p(0);
  plane_to_image_homography_reader_writer p2i(0);
  image_to_utm_homography_reader_writer i2u(0);
  utm_to_image_homography_reader_writer u2i(0);
  plane_to_utm_homography_reader_writer p2u(0);
  utm_to_plane_homography_reader_writer u2p(0);
}
#endif


// ----------------------------------------------------------------
/** Make an image view.
 *
 * This method fabricates an image view of the specified size and
 * fills the pixels with ramped values.
 *
 * @param[in] ni - image width (number of columns)
 * @param[in] nj - image height (number of rows)
 * @param[in] np - number of planes
 */
static vil_image_view < vxl_byte >
make_image( unsigned int ni, unsigned int nj, unsigned int np )
{
  vil_image_view<vxl_byte> img( ni, nj, np );

  for( unsigned int j = 0; j < nj; ++j )
  {
    for( unsigned int i = 0; i < ni; ++i )
    {
      for( unsigned int p = 0; p < np; ++p )
      {
        img(i,j,p) = p * i * j; // overflow is o.k.
      }
    }
  }

  return img;
}



// Test reader/writer
// write to string stream
// print stream
// rewind and readback
// test for equality

// ----------------------------------------------------------------
/** Test timestamp IO
 *
 *
 */
void test_timestamp()
{
  vcl_stringstream working_stream;
  vidtk::timestamp ts;
  vidtk::timestamp ts_orig;

  timestamp_reader_writer * the_rw = new timestamp_reader_writer( &ts );

  ts_orig.set_frame_number(123);
  ts_orig.set_time(123.4567);
  ts = ts_orig;

  the_rw->write_object (working_stream);

  // display results
  // vcl_cout << working_stream.str() << vcl_endl;

  working_stream.seekg(0);  // rewind stream

  ts.set_frame_number(1);
  TEST ("Reading timestamp", the_rw->read_object (working_stream), 0);
  TEST ("timestamp valid", the_rw->is_valid(), true);

  TEST ("Read timestamp matches written one", ts, ts_orig);

  TEST ("Reading timestamp (eof)", the_rw->read_object (working_stream), 1);

  delete the_rw;
}


// ----------------------------------------------------------------
/** test GSD reader/writer.
 *
 *
 */
void test_gsd()
{
  vcl_stringstream working_stream;
  double gsd;
  double gsd_orig;

  gsd_reader_writer * the_rw = new gsd_reader_writer( &gsd );

  gsd = 123.456;
  gsd_orig = gsd;

  the_rw->write_object (working_stream);

  // display results
  // vcl_cout << working_stream.str() << vcl_endl;

  working_stream.seekg(0);  // rewind stream

  gsd = 0;
  TEST ("Reading GSD", the_rw->read_object (working_stream), 0);
  TEST ("GSD valid", the_rw->is_valid(), true);

  TEST ("Read GSD matches written one", gsd, gsd_orig);

  TEST ("Reading GSD (eof)", the_rw->read_object (working_stream), 1);

  delete the_rw;
}


// ----------------------------------------------------------------
/** Test video metadata reader/writer
 *
 *
 */
void test_video_metadata()
{
  vcl_stringstream working_stream;
  video_metadata vmd, vmd_orig;

  // initialize metadata
  // TBD

  video_metadata_reader_writer the_rw( &vmd );

  the_rw.write_object (working_stream);
  working_stream.seekg(0);  // rewind stream

  TEST ("Reading video metadata", the_rw.read_object (working_stream), 0);
  TEST ("metadata valid", the_rw.is_valid(), true);

  TEST ("read metadata matches written one", vmd, vmd_orig);

  TEST ("Reading video metadata (eof)", the_rw.read_object (working_stream), 1);
}


// ----------------------------------------------------------------
/** Video modality
 *
 *
 */
void test_video_modality()
{
  vcl_stringstream working_stream;
  vidtk::video_modality mo = VIDTK_IR_N;
  vidtk::video_modality mo_orig = mo;
  video_modality_reader_writer the_rw(&mo);

  the_rw.write_object (working_stream);

  // vcl_cout << working_stream.str() << vcl_endl;
  working_stream.seekg(0);  // rewind stream

  mo = VIDTK_EO_N;

  TEST ("Reading video modality", the_rw.read_object (working_stream), 0);
  TEST ("Video modality valid", the_rw.is_valid(), true);

  TEST ("Read video modality matches written one", mo, mo_orig);

  TEST ("Reading video modality (eof)", the_rw.read_object (working_stream), 1);
}


// ----------------------------------------------------------------
/** Test shot break flags reader/writer
 *
 *
 */
void test_shot_break_flags()
{
  vcl_stringstream working_stream;
  vidtk::shot_break_flags sbf;
  vidtk::shot_break_flags sbf_orig;

  // initialize metadata
  sbf_orig.set_shot_end(false);
  sbf_orig.set_frame_usable(true);
  sbf_orig.set_frame_not_processed(false);
  sbf = sbf_orig;

  shot_break_flags_reader_writer the_rw( &sbf );

  the_rw.write_object (working_stream);
  the_rw.write_object (working_stream);
  the_rw.write_object (working_stream);

  // display results
  // vcl_cout << working_stream.str() << vcl_endl;

  working_stream.seekg(0);  // rewind stream

  // read 1
  sbf.set_shot_end(true);
  TEST ("Reading shot break flags", the_rw.read_object (working_stream), 0);
  TEST ("Shot break flags valid", the_rw.is_valid(), true);
  TEST ("read flags matches written one", sbf, sbf_orig);

  // read 2
  sbf.set_shot_end(true);
  TEST ("Reading shot break flags", the_rw.read_object (working_stream), 0);
  TEST ("Shot break flags valid", the_rw.is_valid(), true);
  TEST ("read flags matches written one", sbf, sbf_orig);

  // read 3
  sbf.set_shot_end(true);
  TEST ("Reading shot break flags", the_rw.read_object (working_stream), 0);
  TEST ("Shot break flags valid", the_rw.is_valid(), true);
  TEST ("read flags matches written one", sbf, sbf_orig);

  TEST ("Reading shot break flags (EOF)", the_rw.read_object (working_stream), 1);
}


// ----------------------------------------------------------------
/** Test image reader/writer
 *
 *
 */
void test_image_reader_writer(int ni, int nj, int np)
{
  vcl_stringstream working_stream;
  vil_image_view < vxl_byte > img, img_orig;
  vcl_string image_file_name;

  // initialize an image.
  img_orig = make_image(ni, nj, np);
  img = img_orig; // shallow copy

  {
    image_view_reader_writer< vxl_byte > the_rw ( &img, "test_image" );

    the_rw.write_object (working_stream);
    the_rw.write_object (working_stream);
    the_rw.write_object (working_stream);

    // display results
    // vcl_cout << working_stream.str() << vcl_endl;

    working_stream.seekg(0);  // rewind stream

    // read 1
    img.clear();
    TEST ("Reading image", the_rw.read_object (working_stream), 0);
    TEST ("Image valid", the_rw.is_valid(), true);
    TEST ("read image matches written one", vil_image_view_deep_equality (img, img_orig), true );
    TEST ("read image view not matching written one", (img == img_orig) , false); // not equal

    // read 2
    img.clear();
    TEST ("Reading image", the_rw.read_object (working_stream), 0);
    TEST ("Image valid", the_rw.is_valid(), true);
    TEST ("read image matches written one", vil_image_view_deep_equality (img, img_orig), true );
    TEST ("read image view not matching written one", (img == img_orig) , false); // not equal

    // read 3
    img.clear();
    TEST ("Reading image", the_rw.read_object (working_stream), 0);
    TEST ("Image valid", the_rw.is_valid(), true);
    TEST ("read image matches written one", vil_image_view_deep_equality (img, img_orig), true );
    TEST ("read image view not matching written one", (img == img_orig) , false); // not equal

    TEST ("Reading image (eof)", the_rw.read_object (working_stream), 1);

    image_file_name = the_rw.get_image_file_name();
  }

  // Delete the image data file
  vul_file::delete_file_glob (image_file_name);
}


// ----------------------------------------------------------------
/** Test group reader/writer
 *
 *
 */
void test_group_reader_writer_exact()
{
  vcl_stringstream working_stream;
  vidtk::group_data_reader_writer the_group;

  double gsd, gsd_orig;

  gsd = 123.456;
  gsd_orig = gsd;

  vidtk::timestamp ts, ts_orig;

  ts_orig.set_frame_number(123);
  ts_orig.set_time(123.4567);
  ts = ts_orig;

  vidtk::shot_break_flags sbf, sbf_orig;

  sbf_orig.set_shot_end(false);
  sbf_orig.set_frame_usable(true);
  sbf_orig.set_frame_not_processed(false);
  sbf = sbf_orig;


  base_reader_writer* gsd_rw  = the_group.add_data_reader_writer (gsd_reader_writer ( &gsd ) );
  base_reader_writer* ts_rw = the_group.add_data_reader_writer (timestamp_reader_writer ( &ts ) );
  base_reader_writer* sbf_rw = the_group.add_data_reader_writer (shot_break_flags_reader_writer ( &sbf ) );
  sbf_rw->add_instance_name ("current_flags");

  the_group.write_header (working_stream);
  the_group.write_object (working_stream);
  the_group.write_object (working_stream);
  the_group.write_object (working_stream);
  //vcl_cout << working_stream.str() << vcl_endl;

  working_stream.seekg(0);  // rewind stream

  // read 1
  gsd = 0;
  ts.set_frame_number (0);
  sbf.set_shot_end (true);
  TEST ("Reading group", the_group.read_object (working_stream), 0);

  TEST ("timestamp reader valid data", ts_rw->is_valid(), true);
  TEST ("GSD reader valid data", gsd_rw->is_valid(), true);
  TEST ("Shot break flags valid", sbf_rw->is_valid(), true);

  TEST ("Read timestamp matches written one", ts, ts_orig);
  TEST ("Read GSD matches written one", gsd, gsd_orig);
  TEST ("read flags matches written one", sbf, sbf_orig);

  // read 2
  gsd = 0;
  ts.set_frame_number (0);
  sbf.set_shot_end (true);
  TEST ("Reading group", the_group.read_object (working_stream), 0);

  TEST ("timestamp reader valid data", ts_rw->is_valid(), true);
  TEST ("GSD reader valid data", gsd_rw->is_valid(), true);
  TEST ("Shot break flags valid", sbf_rw->is_valid(), true);

  TEST ("Read timestamp matches written one", ts, ts_orig);
  TEST ("Read GSD matches written one", gsd, gsd_orig);
  TEST ("read flags matches written one", sbf, sbf_orig);

  // read 3
  gsd = 0;
  ts.set_frame_number (0);
  sbf.set_shot_end (true);
  TEST ("Reading group", the_group.read_object (working_stream), 0);

  TEST ("timestamp reader valid data", ts_rw->is_valid(), true);
  TEST ("GSD reader valid data", gsd_rw->is_valid(), true);
  TEST ("Shot break flags valid", sbf_rw->is_valid(), true);

  TEST ("Read timestamp matches written one", ts, ts_orig);
  TEST ("Read GSD matches written one", gsd, gsd_orig);
  TEST ("read flags matches written one", sbf, sbf_orig);

  // read 4
  TEST ("Reading group (eof)", the_group.read_object (working_stream), -1);

}

// ----------------------------------------------------------------
/** Test group reader/writer
 *
 *
 */
void test_group_reader_writer_less()
{
  vcl_stringstream working_stream;

  double gsd;

  gsd = 123.456;

  vidtk::timestamp ts, ts_orig;

  ts_orig.set_frame_number(123);
  ts_orig.set_time(123.4567);
  ts = ts_orig;


  vidtk::shot_break_flags sbf, sbf_orig;

  sbf_orig.set_shot_end(false);
  sbf_orig.set_frame_usable(true);
  sbf_orig.set_frame_not_processed(false);
  sbf = sbf_orig;

  { // temporary scope
    vidtk::group_data_reader_writer the_group;

    the_group.add_data_reader_writer (gsd_reader_writer ( &gsd ) );
    the_group.add_data_reader_writer (timestamp_reader_writer ( &ts ) );
    the_group.add_data_reader_writer (shot_break_flags_reader_writer ( &sbf ) );

    the_group.write_header (working_stream);
    the_group.write_object (working_stream); // 1
    the_group.write_object (working_stream); // 2
    the_group.write_object (working_stream); // 3 copies

    // vcl_cout << working_stream.str() << vcl_endl;
  }

  working_stream.seekg(0);  // rewind stream

  class local_policy : public group_data_reader_writer::mismatch_policy
  {
  public:
    local_policy() : unrec_count(0), missing_count(0) { }
    void reset() { missing_count = 0; unrec_count = 0; unrec_line.clear(); }
    virtual bool unrecognized_input (const char * line)
    {
      unrec_line = line;
      unrec_count++;
      // vcl_cout << "\npolicy- unrec: " << line << "\n";
      return true; // allow
    }

    virtual int missing_input (group_data_reader_writer::rw_vector_t const& objs)
    {
      missing_count = objs.size();
      // vcl_cout << "\npolicy- missing: " << missing_count << "\n";
      return 0;
    }

    vcl_string unrec_line;
    int unrec_count;
    int missing_count;
  };


  local_policy lp;
  vidtk::group_data_reader_writer the_group (& lp);
  base_reader_writer* ts_rw = the_group.add_data_reader_writer (timestamp_reader_writer ( &ts ) );
  base_reader_writer* gsd_rw  = the_group.add_data_reader_writer (gsd_reader_writer ( &gsd ) );

  // read 1
  lp.reset();
  TEST ("Reading group", the_group.read_object (working_stream), 0);
  TEST ("timestamp reader valid data", ts_rw->is_valid(), true);
  TEST ("GSD reader valid data", gsd_rw->is_valid(), true);
  TEST ("Detected unhandled line (1)", lp.unrec_line != "", true);

  // read 2
  lp.reset();
  TEST ("Reading group", the_group.read_object (working_stream), 0);
  TEST ("timestamp reader valid data", ts_rw->is_valid(), true);
  TEST ("GSD reader valid data", gsd_rw->is_valid(), true);
  TEST ("Detected unhandled line (2)", lp.unrec_line != "", true);

  base_reader_writer* ps_rw = the_group.add_data_reader_writer (shot_break_flags_reader_writer ( &sbf ) );

  // read 3
  lp.reset();
  TEST ("Reading group", the_group.read_object (working_stream), 0);
  TEST ("timestamp reader valid data", ts_rw->is_valid(), true);
  TEST ("GSD reader valid data", gsd_rw->is_valid(), true);
  TEST ("Pipeline status valid", ps_rw->is_valid(), true);
  TEST ("Detected unhandled line (3)", lp.unrec_line.empty(), true);

  TEST ("Reading group", the_group.read_object (working_stream), -1); // end of file

}




// Test reading more than written
// Test writing more than read
// Test mismatch policy objects


// ================================================================
#if 0
#define TEST_HOMOG(T)                                                   \
void test_homog_ ## T()  {                                              \
  vcl_stringstream working_stream;                                      \
  T ## _homography homog;                                               \
  T ## _homography homog_orig;                                          \
  T ## _homography_reader_writer homog_rw (&homog);                     \
  homog_orig = homog;                                                   \
  homog_rw.write_object (working_stream);                               \
  vcl_cout << working_stream.str() << vcl_endl;                         \
  working_stream.seekg(0);                                              \
  TEST ("Reading " # T " homography", homog_rw.read_object (working_stream), 0); \
  TEST ("Valid " #T " read", homog_rw.is_valid(), true);                       \
  TEST ("Read " #T " homography matches written", homog, homog_orig);          \
}

TEST_HOMOG (image_to_image)
TEST_HOMOG (image_to_plane)
TEST_HOMOG (plane_to_image)
TEST_HOMOG (image_to_utm)
TEST_HOMOG (utm_to_image)
TEST_HOMOG (plane_to_utm)
TEST_HOMOG (utm_to_plane)

#undef TEST_HOMOG

#endif


// ================================================================
void test_homog_image_to_image()
{
  vcl_stringstream working_stream;
  image_to_image_homography homog;
  image_to_image_homography homog_orig;

  vidtk::timestamp ts;
  ts.set_frame_number(321);
  ts.set_time(1234567);
  homog.set_source_reference (ts);

  ts.set_frame_number(456);
  ts.set_time(3214567);
  homog.set_dest_reference (ts);

  image_to_image_homography_reader_writer homog_rw(& homog);
  homog_orig = homog;
  homog_rw.write_object(working_stream);

  // vcl_cout << working_stream.str() << vcl_endl;
  // working_stream.seekg(0);

  TEST("Reading image_to_image homography", homog_rw.read_object(working_stream), 0);
  TEST("Valid image_to_image read", homog_rw.is_valid(), true);
  TEST("Read image_to_image homography matches written", homog, homog_orig);
}


// ================================================================
void test_homog_image_to_plane()
{
  vcl_stringstream working_stream;
  image_to_plane_homography homog;
  image_to_plane_homography homog_orig;

  vidtk::timestamp ts;
  ts.set_frame_number(321);
  ts.set_time(1234567);
  homog.set_source_reference (ts);

  image_to_plane_homography_reader_writer homog_rw(& homog);
  homog_orig = homog;
  homog_rw.write_object(working_stream);

  // vcl_cout << working_stream.str() << vcl_endl;
  // working_stream.seekg(0);

  TEST("Reading image_to_plane homography", homog_rw.read_object(working_stream), 0);
  TEST("Valid image_to_plane read", homog_rw.is_valid(), true);
  TEST("Read image_to_plane homography matches written", homog, homog_orig);
}


// ================================================================
void test_homog_plane_to_image()
{
  vcl_stringstream working_stream;
  plane_to_image_homography homog;
  plane_to_image_homography homog_orig;

  vidtk::timestamp ts;
  ts.set_frame_number(321);
  ts.set_time(1234567);
  homog.set_dest_reference (ts);

  plane_to_image_homography_reader_writer homog_rw(& homog);
  homog_orig = homog;
  homog_rw.write_object(working_stream);

  // vcl_cout << working_stream.str() << vcl_endl;
  // working_stream.seekg(0);

  TEST("Reading plane_to_image homography", homog_rw.read_object(working_stream), 0);
  TEST("Valid plane_to_image read", homog_rw.is_valid(), true);
  TEST("Read plane_to_image homography matches written", homog, homog_orig);
}


// ================================================================
void test_homog_image_to_utm()
{
  vcl_stringstream working_stream;
  image_to_utm_homography homog;
  image_to_utm_homography homog_orig;

  vidtk::timestamp ts;
  ts.set_frame_number(321);
  ts.set_time(1234567);
  homog.set_source_reference (ts);

  vidtk::utm_zone_t utm (23, false);
  homog.set_dest_reference(utm);

  image_to_utm_homography_reader_writer homog_rw(& homog);
  homog_orig = homog;
  homog_rw.write_object(working_stream);

  // vcl_cout << working_stream.str() << vcl_endl;
  // working_stream.seekg(0);

  TEST("Reading image_to_utm homography", homog_rw.read_object(working_stream), 0);
  TEST("Valid image_to_utm read", homog_rw.is_valid(), true);
  TEST("Read image_to_utm homography matches written", homog, homog_orig);
}


// ================================================================
void test_homog_utm_to_image()
{
  vcl_stringstream working_stream;
  utm_to_image_homography homog;
  utm_to_image_homography homog_orig;

  vidtk::utm_zone_t utm (23, false);
  homog.set_source_reference(utm);

  vidtk::timestamp ts;
  ts.set_frame_number(321);
  ts.set_time(1234567);
  homog.set_dest_reference (ts);

  utm_to_image_homography_reader_writer homog_rw(& homog);
  homog_orig = homog;
  homog_rw.write_object(working_stream);
  // working_stream.seekg(0);

  TEST("Reading utm_to_image homography", homog_rw.read_object(working_stream), 0);
  TEST("Valid utm_to_image read", homog_rw.is_valid(), true);
  TEST("Read utm_to_image homography matches written", homog, homog_orig);

  //vcl_string temp; temp = working_stream.str();
  //vcl_cout << temp << "\n";
  // vcl_cout << "orig: " << homog_orig << "\n" << "read: " << homog << "\n";
}


// ================================================================
void test_homog_plane_to_utm()
{
  vcl_stringstream working_stream;
  plane_to_utm_homography homog;
  plane_to_utm_homography homog_orig;

  plane_to_utm_homography_reader_writer homog_rw(& homog);
  homog_orig = homog;
  homog_rw.write_object(working_stream);
  // vcl_cout << working_stream.str() << vcl_endl;
  // working_stream.seekg(0);

  TEST("Reading plane_to_utm homography", homog_rw.read_object(working_stream), 0);
  TEST("Valid plane_to_utm read", homog_rw.is_valid(), true);
  TEST("Read plane_to_utm homography matches written", homog, homog_orig);
}


// ================================================================
void test_homog_utm_to_plane()
{
  vcl_stringstream working_stream;
  utm_to_plane_homography homog;
  utm_to_plane_homography homog_orig;

  utm_to_plane_homography_reader_writer homog_rw(& homog);
  homog_orig = homog;
  homog_rw.write_object(working_stream);

  // vcl_cout << working_stream.str() << vcl_endl;
  // working_stream.seekg(0);

  TEST("Reading utm_to_plane homography", homog_rw.read_object(working_stream), 0);
  TEST("Valid utm_to_plane read", homog_rw.is_valid(), true);
  TEST("Read utm_to_plane homography matches written", homog, homog_orig);
}





/*

test group reader/writer

test reader process
test writer process
 */



} // end namespace


// ----------------------------------------------------------------
/** Main test routine
 *
 *
 */
int test_reader_writer (int /*argc*/, char * /*argv*/[])
{
  testlib_test_start( "readers and writers" );

  // Run tests
  test_timestamp();
  test_gsd();
  test_video_metadata();
  test_video_modality();
  test_shot_break_flags();

  test_image_reader_writer(100, 500, 1);
  test_image_reader_writer(500, 500, 2);
  test_image_reader_writer(100, 275, 3);

  test_group_reader_writer_exact();
  test_group_reader_writer_less();

  test_homog_image_to_image();
  test_homog_image_to_plane();
  test_homog_plane_to_image();
  test_homog_image_to_utm();
  test_homog_utm_to_image();
  test_homog_plane_to_utm();
  test_homog_utm_to_plane();


  return testlib_test_summary();
}
