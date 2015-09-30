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

#include <tracking/transform_image_object_process.h>

#include <tracking/tracking_keys.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void tests()
{
  vidtk::transform_image_object_process<vxl_byte> p("test",NULL);
  TEST( "can init fails", p.initialize(), false );
  TEST( "test step fails", p.step(), false);
  p = vidtk::transform_image_object_process<vxl_byte>( "test",
                                                       new vidtk::add_image_clip_to_image_object< vxl_byte >() );
  vidtk::config_block blk = p.params();
  vcl_vector< vidtk::image_object_sptr > objs(1);
  objs[0] = new vidtk::image_object;
  objs[0]->bbox_ = vgl_box_2d<unsigned>(100,110,100,110);
  TEST( "can set with default params", p.set_params(blk), true );
  TEST( "can init with default params", p.initialize(), true );
  TEST( "since it is disabled by default step succeeds", p.step(), true);
  TEST( "No input give empty output", p.objects().size(), 0);
  p.set_objects(objs);
  TEST( "step succeeds", p.step(), true);
  TEST( "Output has one obj", p.objects().size(), 1);
  TEST( "Output is same pointer", p.objects()[0], objs[0]);
  bool r = p.objects()[0]->data_.has( vidtk::tracking_keys::pixel_data);
  TEST( "Output does not have image data", r, false );
  TEST( "since it is disabled by default step succeeds", p.step(), true);
  TEST( "No input give empty output", p.objects().size(), 0);
  blk.set("disabled", "BOB");
  TEST( "returns false because BOB is not a boolean", p.set_params(blk), false );
  blk.set("disabled", "false");
  TEST( "can set with change params", p.set_params(blk), true );
  TEST( "can init", p.initialize(), true );
  TEST( "Step fails with no input", p.step(), false);
  vil_image_view<vxl_byte>  img( 1000, 1000 );
  img.fill( 0 );
  vcl_cout << objs.size() << vcl_endl;
  p.set_objects(objs);
  p.set_image(img);
  TEST( "Step succeeds", p.step(), true);
  TEST( "Output has one obj", p.objects().size(), 1);
  r = p.objects()[0]->data_.has( vidtk::tracking_keys::pixel_data);
  TEST( "Output does have image data", r, true );
  r = p.objects()[0]->data_.has( vidtk::tracking_keys::pixel_data_buffer );
  TEST( "Output does has buffer info", r, true );
  vil_image_view<vxl_byte>  clip;
  p.objects()[0]->data_.get( vidtk::tracking_keys::pixel_data, clip );
  TEST( "Clip image is the correct size", clip.ni() == 12 && clip.nj() == 12, true );
}


} // end anonymous namespace

int test_add_image_clip_to_image_object_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test add image clip to image object process" );

  tests();

  return testlib_test_summary();
}
