/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "base_reader_writer.h"

#include <vcl_iostream.h>



namespace vidtk
{


// ================================================================
// methods for class base_reader_writer
// ================================================================

base_reader_writer::
base_reader_writer ( vcl_string const& tag )
  : m_entryTag (tag)
{
  // Add delimeter/ending char
  m_entryTag += ":";
}


vcl_string const& base_reader_writer::
add_instance_name (vcl_string const& name)
{
  m_instanceName = name;
  m_entryTag += name + ":";
  return m_entryTag;
}


vcl_string const& base_reader_writer::
entry_tag_string() const
{
  return m_entryTag;
}


bool base_reader_writer::
verify_tag(vcl_istream& str) const
{
  vcl_streampos pos = str.tellg();
  vcl_string tag;

  str >> tag;                   // read tag
  str.seekg(pos);               // rewind file
  if (str.fail() )
  {
    return (false);
  }

  return (tag == this->entry_tag_string());
}

} // end namespace

