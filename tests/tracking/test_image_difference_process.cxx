/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_cassert.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>

#include <tracking/image_difference_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

// Create an image with a square "object" of length sz at pos_i, pos_j
vil_image_view<vxl_byte>
make_image( unsigned ni, unsigned nj, unsigned pos_i, unsigned pos_j, unsigned sz )
{
  vil_image_view<vxl_byte> img( ni, nj );
  img.fill( 0 );

  assert( pos_i + sz < ni );
  assert( pos_j + sz < nj );

  for( unsigned j = 0; j < sz; ++j )
  {
    for( unsigned i = 0; i < sz; ++i )
    {
      img( pos_i+i, pos_j+j ) = 150;
    }
  }

  return img;
}


void
test_is_blank( vil_image_view<bool> const& img )
{
  bool good = true;
  for( unsigned j = 0; j < img.nj() && good; ++j )
  {
    for( unsigned i = 0; i < img.ni() && good; ++i )
    {
      if( img(i,j) )
      {
        good = false;
      }
    }
  }

  TEST( "FG image is empty", good, true );
}


void test_internal_buffer()
{
  vcl_cout << "\n\nTest internal buffer\n\n";

  using namespace vidtk;

  image_difference_process<vxl_byte> idp( "idp" );
  config_block blk = idp.params();
  blk.set( "shadow_removal_with_second_diff", "false" );
  blk.set( "image_spacing", 1 );
  TEST( "Configure", idp.set_params( blk ), true );
  TEST( "Initialize", idp.initialize(), true );

  idp.set_source_image( make_image( 100, 100, 10, 10, 6 ) );
  TEST( "Step", idp.step(), true );
  test_is_blank( idp.fg_image() );

  idp.set_source_image( make_image( 100, 100, 20, 10, 6 ) );
  TEST( "Step", idp.step(), true );
  // expect signal at (10,10) and at (20,10)
  TEST( "Difference is as expected", true,
        idp.fg_image()(0,0)==false &&
        idp.fg_image()(10,10)==true &&
        idp.fg_image()(20,10)==true &&
        idp.fg_image()(30,10)==false );

  idp.set_source_image( make_image( 100, 100, 30, 10, 6 ) );
  TEST( "Step", idp.step(), true );
  // expect signal at (20,10) and at (30,10)
  TEST( "Difference is as expected", true,
        idp.fg_image()(0,0)==false &&
        idp.fg_image()(10,10)==false &&
        idp.fg_image()(20,10)==true &&
        idp.fg_image()(30,10)==true &&
        idp.fg_image()(40,10)==false );
}


void test_internal_buffer_shadow_removal()
{
  vcl_cout << "\n\nTest internal buffer without shadow removal\n\n";

  using namespace vidtk;

  {
    image_difference_process<vxl_byte> idp( "idp" );
    config_block blk = idp.params();
    blk.set( "shadow_removal_with_second_diff", "false" );
    blk.set( "image_spacing", 2 );
    TEST( "Configure", idp.set_params( blk ), true );
    TEST( "Initialize", idp.initialize(), true );

    idp.set_source_image( make_image( 100, 100, 10, 10, 6 ) );
    TEST( "Step", idp.step(), true );
    test_is_blank( idp.fg_image() );

    idp.set_source_image( make_image( 100, 100, 20, 10, 6 ) );
    TEST( "Step", idp.step(), true );
    // expect no signal, since there aren't 3 frames yet.
    test_is_blank( idp.fg_image() );

    idp.set_source_image( make_image( 100, 100, 30, 10, 6 ) );
    TEST( "Step", idp.step(), true );
    // expect signal at (10,10) and at (30,10)
    //vil_save( idp.fg_image(), "diffb.pbm" );
    TEST( "Difference is as expected", true,
          idp.fg_image()(0,0)==false &&
          idp.fg_image()(10,10)==true &&
          idp.fg_image()(20,10)==false &&
          idp.fg_image()(30,10)==true &&
          idp.fg_image()(40,10)==false );
  }

  vcl_cout << "\n\nTest internal buffer with shadow removal\n\n";

  {
    image_difference_process<vxl_byte> idp( "idp" );
    config_block blk = idp.params();
    blk.set( "shadow_removal_with_second_diff", "true" );
    blk.set( "image_spacing", 2 );
    TEST( "Configure", idp.set_params( blk ), true );
    TEST( "Initialize", idp.initialize(), true );

    idp.set_source_image( make_image( 100, 100, 10, 10, 6 ) );
    TEST( "Step", idp.step(), true );
    test_is_blank( idp.fg_image() );

    idp.set_source_image( make_image( 100, 100, 20, 10, 6 ) );
    TEST( "Step", idp.step(), true );
    // expect no signal, since there aren't 3 frames yet.
    test_is_blank( idp.fg_image() );

    idp.set_source_image( make_image( 100, 100, 30, 10, 6 ) );
    TEST( "Step", idp.step(), true );
    // expect signal only at (30,10); signal at (10,10) should be
    //removed.
    TEST( "Difference is as expected", true,
          idp.fg_image()(0,0)==false &&
          idp.fg_image()(10,10)==false &&
          idp.fg_image()(20,10)==false &&
          idp.fg_image()(30,10)==true &&
          idp.fg_image()(40,10)==false );
  }

}


void test_explicit_frames_shadow_removal()
{
  using namespace vidtk;

  vil_image_view<vxl_byte> img1 = make_image( 100, 100, 10, 10, 6 );
  vil_image_view<vxl_byte> img2 = make_image( 100, 100, 20, 10, 6 );
  vil_image_view<vxl_byte> img3 = make_image( 100, 100, 30, 10, 6 );

  vcl_cout << "\n\nTest explicit frames without shadow removal\n\n";

  {
    image_difference_process<vxl_byte> idp( "idp" );
    config_block blk = idp.params();
    blk.set( "shadow_removal_with_second_diff", "false" );
    // for explicit frame setting should ignore this spacing
    blk.set( "image_spacing", 200 );
    TEST( "Configure", idp.set_params( blk ), true );
    TEST( "Initialize", idp.initialize(), true );

    vcl_cout << "Only two images (1)\n";
    idp.set_latest_image( img1 );
    idp.set_second_image( img2 );
    TEST( "Step", idp.step(), true );
    // expect signal at (10,10) and at (20,10)
    TEST( "Difference is as expected", true,
          idp.fg_image()(0,0)==false &&
          idp.fg_image()(10,10)==true &&
          idp.fg_image()(20,10)==true &&
          idp.fg_image()(30,10)==false &&
          idp.fg_image()(40,10)==false );

    vcl_cout << "Only two images (2)\n";
    idp.set_latest_image( img1 );
    idp.set_second_image( img3 );
    TEST( "Step", idp.step(), true );
    // expect signal at (10,10) and at (30,10)
    TEST( "Difference is as expected", true,
          idp.fg_image()(0,0)==false &&
          idp.fg_image()(10,10)==true &&
          idp.fg_image()(20,10)==false &&
          idp.fg_image()(30,10)==true &&
          idp.fg_image()(40,10)==false );

    vcl_cout << "Third image should be ignored\n";
    idp.set_latest_image( img1 );
    idp.set_second_image( img3 );
    idp.set_third_image( img2 );
    TEST( "Step", idp.step(), true );
    // expect signal at (10,10) and at (30,10)
    TEST( "Difference is as expected", true,
          idp.fg_image()(0,0)==false &&
          idp.fg_image()(10,10)==true &&
          idp.fg_image()(20,10)==false &&
          idp.fg_image()(30,10)==true &&
          idp.fg_image()(40,10)==false );
  }

  vcl_cout << "\n\nTest explicit frames with shadow removal\n\n";


  {
    image_difference_process<vxl_byte> idp( "idp" );
    config_block blk = idp.params();
    blk.set( "shadow_removal_with_second_diff", "true" );
    // for explicit frame setting should ignore this spacing
    blk.set( "image_spacing", 200 );
    TEST( "Configure", idp.set_params( blk ), true );
    TEST( "Initialize", idp.initialize(), true );

    idp.set_latest_image( img1 );
    idp.set_second_image( img3 );
    idp.set_third_image( img2 );
    TEST( "Step", idp.step(), true );
    // expect signal only at (30,10)
    vil_save( idp.fg_image(), "diffb.pbm" );
    TEST( "Difference is as expected", true,
          idp.fg_image()(0,0)==false &&
          idp.fg_image()(10,10)==true &&
          idp.fg_image()(20,10)==false &&
          idp.fg_image()(30,10)==false &&
          idp.fg_image()(40,10)==false );
  }


}



} // end anonymous namespace

int test_image_difference_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "image difference process" );

  test_internal_buffer();
  test_internal_buffer_shadow_removal();
  test_explicit_frames_shadow_removal();

  return testlib_test_summary();
}
