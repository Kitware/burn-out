/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <tracking/image_object.h>
#include <tracking/filter_image_objects_process.h>
#include <vxl_config.h>


int test_filter_image_objects_process( int /*argc*/, char* /*argv*/[] )
{
  using namespace vidtk;

  testlib_test_start( "filter image objects process" );

  vcl_vector< image_object_sptr > no_objs;

  vcl_vector< image_object_sptr > src_objs;
  {
    image_object_sptr o = new image_object;
    o->area_ = 10;
    src_objs.push_back( o );
  }
  {
    image_object_sptr o = new image_object;
    o->area_ = 1;
    src_objs.push_back( o );
  }
  {
    image_object_sptr o = new image_object;
    o->area_ = 15;
    src_objs.push_back( o );
  }
  {
    image_object_sptr o = new image_object;
    o->area_ = 3;
    src_objs.push_back( o );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "min_area", "5" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( src_objs );
    TEST( "Step 1", f1.step(), true );
    bool good = true;
    for( unsigned i = 0; i < f1.objects().size(); ++i )
    {
      vcl_cout << "Remaining object " << i
               << ": area = " << f1.objects()[i]->area_ << "\n";
      if( f1.objects()[i]->area_ < 5 )
      {
        good = false;
      }
    }
    TEST( "Remaining areas are valid", good, true );
    TEST( "Two objects remained", f1.objects().size(), 2 );

    f1.set_source_objects( no_objs );
    TEST( "Step 2", f1.step(), true );
    TEST( "No object after second time", f1.objects().empty(), true );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "min_area", "25" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( no_objs );
    TEST( "Step 1", f1.step(), true );
    TEST( "No objects left", f1.objects().empty(), true );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "max_area", "5" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( src_objs );
    TEST( "Step 1", f1.step(), true );
    bool good = true;
    for( unsigned i = 0; i < f1.objects().size(); ++i )
    {
      vcl_cout << "Remaining object " << i
               << ": area = " << f1.objects()[i]->area_ << "\n";
      if( f1.objects()[i]->area_ > 5 )
      {
        good = false;
      }
    }
    TEST( "Remaining areas are valid", good, true );
    TEST( "Two objects remained", f1.objects().size(), 2 );

    f1.set_source_objects( no_objs );
    TEST( "Step 2", f1.step(), true );
    TEST( "No object after second time", f1.objects().empty(), true );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "max_area", "0" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( no_objs );
    TEST( "Step 1", f1.step(), true );
    TEST( "No objects left", f1.objects().empty(), true );
  }


  return testlib_test_summary();
}
