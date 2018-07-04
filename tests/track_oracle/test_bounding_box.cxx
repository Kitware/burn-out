/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>
#include <iomanip>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_intersection.h>

#include <testlib/testlib_test.h>

namespace {  /* anonymous */

  template <class Type>
  void LoadBoundingBox(vgl_box_2d<Type>& b, Type ulx, Type uly, Type lrx, Type lry)
  {
    b.set_min_x(ulx);
    b.set_min_y(uly);
    b.set_max_x(lrx);
    b.set_max_y(lry);
  }
  template <class Type>
  void TranslateBoundingBox( vgl_box_2d<Type>& b,
    Type dx,
    Type dy )
  {
    b.set_min_x(b.min_x()+dx);
    b.set_max_x(b.max_x()+dx);
    b.set_min_y(b.min_y()+dy);
    b.set_max_y(b.max_y()+dy);
  }
  template <class currType>
  void RunTestAsType()
  {
    vgl_box_2d<currType> b1, b2, b3;

    b3 = vgl_intersection(b1,b2);
    TEST("Intersect checks uninitialized boxes b1",b1.is_empty() == true, true);
    TEST("Intersect checks uninitialized boxes b2",b2.is_empty() == true, true);
    TEST("Intersect checks uninitialized boxes intersection",b3.is_empty() == true, true);

    LoadBoundingBox<currType>( b1,static_cast<currType>(42.0),static_cast<currType>(42.0),static_cast<currType>(84.0),static_cast<currType>(84.0));
    LoadBoundingBox<currType>( b2,static_cast<currType>(0.0),static_cast<currType>(0.0),static_cast<currType>(10.0),static_cast<currType>(10.0));

    b3 = vgl_intersection(b1,b2);
    TEST( "Disjoint boxes do not intersect", b3.is_empty(), true );

    LoadBoundingBox<currType>( b1,static_cast<currType>(0.0),static_cast<currType>(0.0),static_cast<currType>(0.0),static_cast<currType>(0.0) );
    LoadBoundingBox<currType>( b2,static_cast<currType>(0.0),static_cast<currType>(0.0),static_cast<currType>(0.0),static_cast<currType>(0.0) );

    TEST( "Empty boxes have zero area", b1.volume() ==static_cast<currType>(0.0), true );
    b3 = vgl_intersection(b1,b2);
    //Note if all coords are 0 the bbox is not defined as empty. It is just a point.
    TEST( "Empty boxes do not intersect", b3.volume() ==static_cast<currType>(0.0), true );

    LoadBoundingBox<currType>( b1,static_cast<currType>(10.0),static_cast<currType>(10.0),static_cast<currType>(20.0),static_cast<currType>(20.0) );
    LoadBoundingBox<currType>( b2,static_cast<currType>(10.0),static_cast<currType>(10.0),static_cast<currType>(20.0),static_cast<currType>(20.0) );

    TEST( "Area of a 10x10 == 100", b1.volume() ==static_cast<currType>(100.0), true );
    b3 = vgl_intersection( b1, b2);
    TEST( "Intersect equal boxes gives same area", (b3.volume() ==static_cast<currType>(100.0)), true );
    bool boxesEqual = ((b3.min_x() == b1.min_x()) &&
      (b3.min_y() == b1.min_y()) &&
      (b3.max_x() == b1.max_x()) &&
      (b3.max_y() == b1.max_y()) );
    TEST( "Intersect equal boxes yields the source box", boxesEqual, true );

    // offset b1 by five; same area; intersection should be 25
    TranslateBoundingBox<currType>( b1,static_cast<currType>(5.0),static_cast<currType>(5.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Area of (5,5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );
    TranslateBoundingBox<currType>( b1,static_cast<currType>(-10.0),static_cast<currType>(0.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Area of (-5,5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );
    TranslateBoundingBox<currType>( b1,static_cast<currType>(0.0),static_cast<currType>(-10.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Area of (-5,-5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );
    TranslateBoundingBox<currType>( b1,static_cast<currType>(10.0),static_cast<currType>(0.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Area of (5,-5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );

    LoadBoundingBox<currType>( b1,static_cast<currType>(-5.0),static_cast<currType>(-5.0),static_cast<currType>(5.0),static_cast<currType>(5.0) );
    LoadBoundingBox<currType>( b2,static_cast<currType>(-5.0),static_cast<currType>(-5.0),static_cast<currType>(5.0),static_cast<currType>(5.0) );

    TEST( "Q1/2 Area of a 10x10 == 100", (b1.volume() ==static_cast<currType>(100.0)), true );
    b3 = vgl_intersection( b1, b2);
    TEST( "Q1/2 Intersect equal boxes gives same area", (b3.volume() ==static_cast<currType>(100.0)), true );
    boxesEqual = ((b3.min_x() == b1.min_x()) &&
      (b3.min_y() == b1.min_y()) &&
      (b3.max_x() == b1.max_x()) &&
      (b3.max_y() == b1.max_y()) );
    TEST( "Q1/2 Intersect equal boxes yields the source box", boxesEqual, true );

    // offset b1 by five; same area; intersection should be 25
    TranslateBoundingBox<currType>( b1,static_cast<currType>(5.0),static_cast<currType>(5.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Q1/2 Area of (5,5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );
    TranslateBoundingBox<currType>( b1,static_cast<currType>(-10.0),static_cast<currType>(0.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Q1/2 Area of (-5,5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );
    TranslateBoundingBox<currType>( b1,static_cast<currType>(0.0),static_cast<currType>(-10.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Q1/2 Area of (-5,-5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );
    TranslateBoundingBox<currType>( b1,static_cast<currType>(10.0),static_cast<currType>(0.0) );
    b3 = vgl_intersection( b1, b2);
    TEST( "Q1/2 Area of (5,-5) displacement is 25%", (b3.volume() ==static_cast<currType>(25.0)), true );


    LoadBoundingBox<currType>( b1,static_cast<currType>(50.0),static_cast<currType>(50.0),static_cast<currType>(70.0),static_cast<currType>(60.0));
    LoadBoundingBox<currType>( b2,static_cast<currType>(69.0),static_cast<currType>(59.0),static_cast<currType>(100.0),static_cast<currType>(100.0));
    b3 = vgl_intersection( b1, b2);
    TEST( "Q1 Single-pixel bounding box intersection", (b3.volume()==static_cast<currType>(1.0)), true );
    bool boxExpected = ((b3.min_x() ==static_cast<currType>(69.0)) &&
      (b3.min_y() ==static_cast<currType>(59.0)) &&
      (b3.max_x() ==static_cast<currType>(70.0)) &&
      (b3.max_y() ==static_cast<currType>(60.0)));
    TEST( "Q1 Single-pixel intersection is as expected", boxExpected, true );

    LoadBoundingBox<currType>( b1,static_cast<currType>(-10.0),static_cast<currType>(-10.0),static_cast<currType>(10.0),static_cast<currType>(10.0));
    LoadBoundingBox<currType>( b2,static_cast<currType>(-20.0),static_cast<currType>(-20.0),static_cast<currType>(20.0),static_cast<currType>(20.0));
    b3 = vgl_intersection( b1, b2);
    TEST( "Nested bounding box intersection", (b3.volume() == 400), true );
    boxExpected = ((b3.min_x() == static_cast<currType>(-10.0)) &&
      (b3.min_y() == static_cast<currType>(-10.0)) &&
      (b3.max_x() == static_cast<currType>(10.0)) &&
      (b3.max_y() == static_cast<currType>(10.0)));
    TEST( "Nested intersection is as expected", boxExpected, true );
  }

}

int test_bounding_box( int /*argc*/, char * /*argv*/[] )
{

  testlib_test_start( "test_bounding_box" );

  TEST( "Running tests as type double", true, true );
  RunTestAsType<double>();
  TEST( "Redoing tests as type int", true, true );
  RunTestAsType<int>();
  TEST( "Redoing tests as type float", true, true );
  RunTestAsType<float>();

  return testlib_test_summary();
}

