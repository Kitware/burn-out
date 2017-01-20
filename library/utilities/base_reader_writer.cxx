/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "base_reader_writer.h"

#include <iostream>



namespace vidtk
{


// ================================================================
// methods for class base_reader_writer
// ================================================================

base_reader_writer::
base_reader_writer ( std::string const& tag )
  : m_entryTag (tag),
    m_validEntry( false )
{
  // Add delimeter/ending char
  m_entryTag += ":";
}


std::string const& base_reader_writer::
add_instance_name (std::string const& name)
{
  m_instanceName = name;
  m_entryTag += name + ":";
  return m_entryTag;
}


std::string const& base_reader_writer::
entry_tag_string() const
{
  return m_entryTag;
}


bool base_reader_writer::
verify_tag(std::istream& str) const
{
  std::streampos pos = str.tellg();
  std::string tag;

  str >> tag;                   // read tag
  str.seekg(pos);               // rewind file
  if (str.fail() )
  {
    return (false);
  }

  return (tag == this->entry_tag_string());
}

} // end namespace
