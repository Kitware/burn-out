/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <protobuf/convert_protobuf.h>

#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <vgl/vgl_box_2d.h>
#include <vnl/vnl_vector_fixed.h>
#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_polygon.h>
#include <vil/vil_image_view.h>
#include <tracking_data/image_object.h>

#include <protobuf/ImageObject.pb.h>
#include <protobuf/Image.pb.h>
#include <protobuf/PixelBoundingBox.pb.h>
#include <protobuf/PixelPosition.pb.h>
#include <protobuf/Polygon.pb.h>
#include <protobuf/Timestamp.pb.h>
#include <protobuf/VidtkTypeUnion.pb.h>
#include <protobuf/WorldPoint.pb.h>

#include <cstring>


#define DEBUG 0

using namespace vidtk;

namespace {

#if DEBUG
#define test_field( F ) TEST("Compare field " #F, !(lhs.F == rhs.F), false );
#else
#define test_field( F ) if ( !(lhs.F == rhs.F) ) { return false; }
#endif

// ------------------------------------------------------------------
// application specific test for equality
bool operator== ( vgl_polygon< double > const& lhs, vgl_polygon< double > const& rhs )
{
  if (lhs.num_sheets() != rhs.num_sheets() ) { return false; }

  for (unsigned int i = 0; i < lhs.num_sheets(); ++i)
  {
    if ( lhs[i] != rhs[i] ) { return false; }
  }

  return true;
}


// ------------------------------------------------------------------
template < typename T >
bool operator==( vil_image_view< T > const& lhs, vil_image_view< T > const& rhs )
{
  if ( !lhs == true ) { return false; }
  if ( !rhs == true ) { return false; }

  test_field( ni() );
  test_field( nj() );
  test_field( nplanes() );
  test_field( size_bytes() );

  int res = memcmp( lhs.memory_chunk()->data(),
                    rhs.memory_chunk()->data(),
                    lhs.size_bytes() );
#if DEBUG
  TEST( "Same memory contents", res, 0 );
#else
  if ( 0 !=  res) { return false; }
#endif

  return true;
}



// ------------------------------------------------------------------
bool operator==( image_object const& lhs, image_object const& rhs )
{
  test_field( get_boundary() );
  test_field( get_bbox() );
  test_field( get_image_loc() );
  test_field( get_world_loc() );
  test_field( get_area() );
  test_field( get_image_area() );
  // This is enough to prove the point

  return true;
}


bool operator==( std::vector< image_object_sptr > const& lhs, std::vector< image_object_sptr > const& rhs )
{
  test_field( size() );

  for (unsigned int i = 0; i < lhs.size(); ++i)
  {
    if ( ! (*lhs[i] == *rhs[i]) ) { return false; }
  }

  return true;
}

} // end namespace


//------------------------------------------------------------------
int test_convert_protobuf( int /* argc */, char* /* argv */ [] )
{
  testlib_test_start( "test protobuf converters" );

#define CONVERSION_TEST( vType, pType, init )                           \
  do {                                                                  \
    const vType in init;                                                \
    proto::pType proto;                                                 \
    vType out;                                                          \
    TEST( # vType " to " # pType, convert_protobuf( in, proto ), true ); \
    TEST( # pType " to " # vType, convert_protobuf( proto, out ), true ); \
    TEST( # vType " conversion fidelity", in, out );                    \
  } while ( 0 )


#define HOMOGRAPHY_CONVERSION_TEST( vType, pType, init )                \
  do {                                                                  \
    const vType in init;                                                \
    proto::pType proto;                                                 \
    vType out;                                                          \
    TEST( # vType " to " # pType, convert_protobuf( in, proto ), true ); \
    TEST( # pType " to " # vType, convert_protobuf( proto, out ), true ); \
    TEST( # vType " conversion fidelity", in, out );                    \
    if (in != out)                                                      \
    {                                                                   \
      std::cout << "in: " << in << std::endl                            \
                << "out: " << out << std::endl;                         \
    }                                                                   \
  } while ( 0 )


  // Special code to test images. The native equality operator for
  // vil_image_view's does not work well for us.
#define IMAGE_CONVERSION_TEST( vType, pType, init )                     \
  do {                                                                  \
    vType in init;                                                      \
    proto::pType proto;                                                 \
    vType out;                                                          \
    in.fill(0);                                                         \
    TEST( # vType " to " # pType, convert_protobuf( in, proto ), true ); \
    TEST( # pType " to " # vType, convert_protobuf( proto, out ), true ); \
    bool equal =                                                        \
      in.ni()        == out.ni() &&                                     \
      in.nj()        == out.nj() &&                                     \
      in.nplanes()   == out.nplanes() &&                                \
      in.istep()     == out.istep() &&                                  \
      in.jstep()     == out.jstep() &&                                  \
      in.planestep() == out.planestep() ;                               \
    TEST( # vType " conversion fidelity - image size", equal, true );   \
    if ( equal )                                                        \
    {                                                                   \
      size_t size = in.size();                                          \
      for (size_t i = 0; i < size; ++i)                                 \
      { /* do pixel by pixel compare */                                 \
        if ( in.top_left_ptr()[i] != out.top_left_ptr()[i] )            \
        {                                                               \
          std::cout << "Mismatch at pixel index: " << i                 \
                    << "  - in pixel: " << in.top_left_ptr()[i]         \
                    << "  out pixel: " << out.top_left_ptr()[i]         \
                    << std::endl;                                       \
          equal = false;                                                \
        }                                                               \
      }                                                                 \
    }                                                                   \
    TEST( # vType " conversion fidelity - image content", equal, true ); \
  } while ( 0 )


  CONVERSION_TEST( timestamp, Timestamp, ( 123.456e6, 321 ) );
  CONVERSION_TEST( vgl_point_2d< double >, PixelPosition, ( 123.456, 234.567 ) );
  CONVERSION_TEST( vgl_point_2d< unsigned >, PixelPosition, ( 123, 234 ) );

  typedef  vnl_vector_fixed< double, 2 > d2d_vector_t;
  CONVERSION_TEST( d2d_vector_t, PixelPosition, ( 123.456, 234.567 ) );

  {
    vgl_box_2d< unsigned > init_box( vgl_point_2d< unsigned > ( 123, 234 ), vgl_point_2d< unsigned > ( 567, 678 ) );
    CONVERSION_TEST( vgl_box_2d< unsigned >, PixelBoundingBox, ( init_box ) );
  }

  typedef vnl_vector_fixed< double, 3 > vfd3_t;
  CONVERSION_TEST( vfd3_t, WorldPoint, ( 123.456, 234.567, 345.678 ) );

  {
    double x[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    double y[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    vgl_polygon< double > init_poly( x, y, 9 );
    CONVERSION_TEST( vgl_polygon< double >, Polygon, ( init_poly ) );
  }

  {
    vil_image_view< vxl_byte > init( 720, 480 );
    for (size_t i = 0; i < init.size(); ++i )
    {
      init.top_left_ptr()[i] = i & 0xff;
    }
    IMAGE_CONVERSION_TEST( vil_image_view< vxl_byte >, Image, (init) );
  }

  {
    vil_image_view< bool > init( 200, 280 );
    for (size_t i = 0; i < init.size(); ++i )
    {
      init.top_left_ptr()[i] = i & 0x1;
    }
    IMAGE_CONVERSION_TEST( vil_image_view< bool >, Image, (init) );
  }

  {
    vil_image_view< float > init( 100, 280 );
    for (size_t i = 0; i < init.size(); ++i )
    {
      init.top_left_ptr()[i] = i/10;
    }
    IMAGE_CONVERSION_TEST( vil_image_view< float >, Image, (init) );
  }

  {
    std::vector< image_object_sptr > init_vector;
    for ( int i = 0; i < 4; ++i )
    {
      image_object_sptr ptr = new image_object;
      ptr->set_bbox( vgl_box_2d< unsigned > ( i, i, i, i ) );

      init_vector.push_back( ptr );
    } // end for

    //+ this may need work - check objects not pointers
    CONVERSION_TEST( std::vector< image_object_sptr >, ImageObjectVector, ( init_vector ) );
  }

  {
    image_object init_im;
    init_im.set_bbox( vgl_box_2d< unsigned > ( 10, 20, 30, 14 ) );
    CONVERSION_TEST( image_object, ImageObject, ( init_im ) );
  }

  // Test homographies
#define TEST_HOMOG(H_TYPE)                              \
  {                                                     \
    H_TYPE homog;                                       \
    HOMOGRAPHY_CONVERSION_TEST( H_TYPE, Homography, );  \
  }

  // These homogs need a valid time stamp
  {
    image_to_image_homography init;
    vidtk::timestamp ts( 123, 456 );
    init
      .set_source_reference( ts)
      .set_dest_reference( ts);
    HOMOGRAPHY_CONVERSION_TEST( image_to_image_homography, Homography, =init );
  }

  {
    image_to_plane_homography init;
    vidtk::timestamp ts( 123, 456 );
    init.set_source_reference( ts);

    HOMOGRAPHY_CONVERSION_TEST( image_to_plane_homography, Homography, =init );
  }

  {
    plane_to_image_homography init;
    vidtk::timestamp ts( 123, 456 );
    init.set_dest_reference( ts);
    HOMOGRAPHY_CONVERSION_TEST( plane_to_image_homography, Homography, =init );
  }

  {
    image_to_utm_homography init;
    vidtk::timestamp ts( 123, 456 );
    init.set_source_reference( ts);

    HOMOGRAPHY_CONVERSION_TEST( image_to_utm_homography, Homography, =init );
  }

  {
    utm_to_image_homography init;
    vidtk::timestamp ts( 123, 456 );
    init.set_dest_reference( ts);
    HOMOGRAPHY_CONVERSION_TEST( utm_to_image_homography, Homography, =init );
  }


  TEST_HOMOG( plane_to_plane_homography );
  TEST_HOMOG( plane_to_utm_homography );
  TEST_HOMOG( utm_to_plane_homography );

  return testlib_test_summary();
} // test_convert_protobuf
