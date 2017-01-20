/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <protobuf/protobuf_container.h>
#include <protobuf/protobuf_message_container.h>
#include <protobuf/protobuf_composite_container.h>
#include <protobuf/protobuf_container_visitor.h>

#include <testlib/testlib_test.h>

#include <protobuf/convert_protobuf.h>
#include <utilities/timestamp.h>

#include <protobuf/ImageObject.pb.h>
#include <protobuf/Image.pb.h>
#include <protobuf/PixelBoundingBox.pb.h>
#include <protobuf/PixelPosition.pb.h>
#include <protobuf/Polygon.pb.h>
#include <protobuf/Timestamp.pb.h>
#include <protobuf/VidtkTypeUnion.pb.h>
#include <protobuf/WorldPoint.pb.h>

#include <sstream>
#include <iostream>

// ------------------------------------------------------------------
using namespace vidtk;

namespace {

class test_visitor
  : public protobuf_container_visitor
{
public:
  void visit_message_container( protobuf_message_container* item)
  {
    proto::VidtkTypeUnion& msg = item->get_message();

    switch ( msg.type() )
    {
    case proto::VidtkTypeUnion_Type_Timestamp:
    {
      TEST("Converting timestamp", convert_protobuf( msg.timestamp(), m_timestamp ), true );

    }
    break;

    case proto::VidtkTypeUnion_Type_ImageObjectVector:
    {
      std::vector< image_object_sptr > list;
      TEST("Converting object vector", convert_protobuf( msg.image_object_vector(), m_obj_list ), true );
    }
    break;

    default:
      break;
    }
  }


  // Data areas
  timestamp m_timestamp;
  std::vector< image_object_sptr > m_obj_list;
}; // end class


// ------------------------------------------------------------------
class show_structure
  : public protobuf_container_visitor
{
public:
  show_structure()
    : m_indent(0) { }

  void visit_message_container( protobuf_message_container* item)
  {
    const std::string ind(m_indent * 2, ' ');
    std::cout << ind << "Protobuf message: ";
    proto::VidtkTypeUnion& msg = item->get_message();

    switch ( msg.type() )
    {
    case proto::VidtkTypeUnion_Type_Timestamp:
    {
      std::cout << ind << "Timestamp\n";
    }
    break;

    case proto::VidtkTypeUnion_Type_ImageObjectVector:
    {
      std::cout << ind << "Image object vector\n";
    }
    break;

    default:
      break;
    }

  }


  void visit_composite_container( protobuf_composite_container* item, edge_t edge )
  {
    if (edge == protobuf_container_visitor::START)
    {
      const std::string ind(m_indent * 2, ' ');
      std::cout << ind << "Composite container, size - " <<  item-> size() << std::endl;
      ++m_indent;
    }
    else
    {
      --m_indent;
    }
  }

  // Data areas
    int m_indent;
  timestamp m_timestamp;
  std::vector< image_object_sptr > m_obj_list;
}; // end class


} // end namespace


// ------------------------------------------------------------------
int test_protobuf_container( int /* argc */, char* /* argv */ [] )
{
  testlib_test_start( "test protobuf container" );

  // factory method
  composite_message_sptr slice = protobuf_container::create_composite_container();

  proto::VidtkTypeUnion* tu = new proto::VidtkTypeUnion();
  timestamp in_ts( 100, 100);
  convert_protobuf( in_ts, *tu);

  // add protobuf to container directly
  message_container_sptr ts_container = protobuf_container::create_message_container( tu );

  slice->add_container(ts_container );
  slice->add_container(ts_container );

  // Add more to slice
  composite_message_sptr bucket = protobuf_container::create_composite_container();
  bucket->add_container(ts_container );
  bucket->add_container(ts_container );
  bucket->add_container(ts_container );

  slice->add_container( bucket );


  std::stringstream str;
  // Write container to stream
  slice->serialize_to_stream( str );


// ============ the other way ================

  protobuf_container_sptr tainer;
  protobuf_container::parse_from_stream( str, tainer );

  // tainer points to composite container.

  // Use visitor to get stuff out of container
  show_structure show;
  tainer->accept_visitor( show );

  test_visitor vis;
  tainer->accept_visitor( vis );
  // check vis for data
  TEST("Timetamp Fidelity", in_ts, vis.m_timestamp );

  return testlib_test_summary();
}
