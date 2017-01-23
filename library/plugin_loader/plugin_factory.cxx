/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "plugin_factory.h"


namespace vidtk {

  const std::string plugin_factory::INTERFACE_TYPE( "interface-type" );
  const std::string plugin_factory::CONCRETE_TYPE( "concrete-type" );
  const std::string plugin_factory::PLUGIN_FILE_NAME( "plugin-file-name" );
  const std::string plugin_factory::PLUGIN_NAME( "plugin-name" );
  const std::string plugin_factory::PLUGIN_DESCRIPTION( "plugin-descrip" );
  const std::string plugin_factory::CONFIG_HELP( "config-help" );

typedef std::map< std::string, std::string>::iterator map_it;
typedef std::map< std::string, std::string>::const_iterator const_map_it;

// ------------------------------------------------------------------
plugin_factory::
plugin_factory( std::string const& itype )
{
  m_interface_type = itype; // Optimize and store locally
  add_attribute( INTERFACE_TYPE, itype );
}


plugin_factory::
~plugin_factory()
{ }


// ------------------------------------------------------------------
bool plugin_factory::
get_attribute( std::string const& attr, std::string& val ) const
{
  const_map_it it = m_attribute_map.find( attr );
  if ( it != m_attribute_map.end() )
  {
    val = it->second;
    return true;
  }

  return false;
}


// ------------------------------------------------------------------
plugin_factory&
plugin_factory::
add_attribute( std::string const& attr, std::string const& val )
{
  // Create if not there. Overwrite if already there.
  m_attribute_map[attr] = val;

  return *this;
}

} // end namespace
