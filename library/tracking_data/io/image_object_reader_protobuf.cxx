/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_reader_protobuf.h"

#include <protobuf/protobuf_container.h>
#include <protobuf/protobuf_message_container.h>
#include <protobuf/protobuf_composite_container.h>
#include <protobuf/protobuf_container_visitor.h>

#include <protobuf/convert_protobuf.h>

#include <vul/vul_file.h>

#include <fstream>
#include <iostream>
#include <stdio.h>

#include <logger/logger.h>

namespace vidtk {
namespace ns_image_object_reader {

VIDTK_LOGGER ("image_object_reader_protobuf");


// ==================================================================
class extract_data
  : public vidtk_type_visitor
{
public:
  extract_data() { }
  virtual ~extract_data() { }


  virtual void visit_timestamp( vidtk::timestamp const& ts )
  {
    m_timestamp = ts;
  }


  virtual void visit_image_object_vector(  std::vector< vidtk::image_object_sptr > const& vect )
  {
    m_obj_list = vect;
  }


  // extracted data is stored here
  timestamp m_timestamp;
  std::vector< image_object_sptr > m_obj_list;
};



// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
image_object_reader_protobuf::
image_object_reader_protobuf()
  : m_next_obj( m_obj_list.end() )
{
}


image_object_reader_protobuf::
~image_object_reader_protobuf()
{
}


// ----------------------------------------------------------------
bool
image_object_reader_protobuf::
open( std::string const& filename)
{
  if ( m_stream.is_open() ) // meaning we are already open
  {
    return true;
  }

  this->filename_ = filename;

  // Currently the only semantic test for our file type is the file
  // extension
  std::string const file_ext( filename_.substr(filename_.rfind(".")+1) );
  if (file_ext != "kwpc")
  {
    return false;
  }

  m_stream.open( this->filename_.c_str() );
  if( ! m_stream )
  {
    LOG_ERROR( "Couldn't open " << this->filename_ << " for reading." );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
bool
image_object_reader_protobuf::
read_next(ns_image_object_reader::datum_t& datum)
{
  // if none buffered, read another chunk
  if ( m_next_obj == m_obj_list.end() )
  {
    if ( ! m_stream )
    {
      return false; // stream in error mode
    }

    if( m_stream.eof() )
    {
      return false;
    }

    extract_data xd;

    // Need to loop over file until we actually get some image
    // objects, since there may be times where the vector is empty.
    do
    {
      protobuf_container_sptr tainer;
      if ( ! protobuf_container::parse_from_stream( m_stream, tainer ) )
      {
        return false; // end of file
      }

      tainer->accept_visitor( xd );
    }
    while ( xd.m_obj_list.empty() );

    // Copy data from the visitor
    this->m_timestamp = xd.m_timestamp;
    this->m_obj_list = xd.m_obj_list;

    // Reset iterator to point to next obj to return
    this->m_next_obj = this->m_obj_list.begin();
  }

  datum.first = this->m_timestamp;
  datum.second = *(this->m_next_obj);
  ++this->m_next_obj;

  return true;
}

} // end namespace
} // end namespace
