/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "raw_descriptor_writer.h"


#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>
#include <plugin_loader/plugin_config_util.h>
#include <utilities/config_block.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>

#include <logger/logger.h>

namespace vidtk {

VIDTK_LOGGER ("raw_descriptor_writer");


raw_descriptor_writer::
raw_descriptor_writer()
{
  // Get a list of plugin factories that support our interface type.
  plugin_factory_vector_t factory_list =
    plugin_manager::instance()->get_factories( typeid(vidtk::ns_raw_descriptor_writer::raw_descriptor_writer_interface).name() );

  BOOST_FOREACH( plugin_factory_handle_t facto, factory_list )
  {
    std::string name;
    facto->get_attribute(vidtk::plugin_factory::PLUGIN_NAME, name );

    // Get file types supported
    std::string type_attr;
    if ( ! facto->get_attribute( "file-type", type_attr ) )
    {
      LOG_WARN( "Track reader plugin \"" << name
                <<  "\" did not register file-types supported attribute" );
      continue;
    }

    // Collect format help messages
    std::string msg;
    facto->get_attribute( plugin_factory::CONFIG_HELP, msg );
    m_writer_formats += msg + "\n";

    // Collect any config items
    collect_plugin_config( *facto, m_plugin_config );

    std::vector< std::string> file_types;
    boost::split( file_types, type_attr, boost::is_any_of(" ," ) );

    ns_raw_descriptor_writer::raw_descriptor_writer_interface* tmp =
      facto->create_object< ns_raw_descriptor_writer::raw_descriptor_writer_interface >();

    // Add this writer for each format it supports
    BOOST_FOREACH( std::string file_type, file_types )
    {
      LOG_DEBUG( "track_writer: Registering writer \"" << name << "\" for type: " << file_type );
      this->add_writer( file_type, handle_t(tmp) );
    }
  }

}


raw_descriptor_writer::
~raw_descriptor_writer()
{
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer::
set_format( std::string const& format )
{
  this->m_raw_writer = get_writer( format );
  return bool( this->m_raw_writer );
}


// ----------------------------------------------------------------
void
raw_descriptor_writer::
add_writer( std::string const& tag, handle_t w )
{
  this->m_implementation_list[tag] = w;
}


// ----------------------------------------------------------------
raw_descriptor_writer::handle_t
raw_descriptor_writer::
get_writer( std::string const& tag )
{
  if( m_implementation_list.count( tag ) == 0 )
  {
    LOG_ERROR( "No raw descriptor writer registered for format \""
               << tag << "\"" );
    return handle_t();
  }

  return m_implementation_list[tag];
}


// ----------------------------------------------------------------
std::string const&
raw_descriptor_writer::
get_writer_formats() const
{
  return this->m_writer_formats;
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer::
initialize( std::ostream* str )
{
  if( this->m_raw_writer )
  {
    return this->m_raw_writer->initialize( str );
  }

  LOG_ERROR( "No raw descriptor writer format selected" );
  return false;
}


// ----------------------------------------------------------------
void
raw_descriptor_writer::
finalize()
{
  if( this->m_raw_writer )
  {
    this->m_raw_writer->finalize();
  }
  else
  {
    LOG_ERROR( "No raw descriptor writer format selected" );
  }
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer::
write( raw_descriptor::vector_t const& desc )
{
  if( this->m_raw_writer )
  {
    return this->m_raw_writer->write( desc );
  }

  LOG_ERROR( "No raw descriptor writer format selected" );
  return false;
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer::
write( descriptor_sptr_t desc )
{
  if( this->m_raw_writer )
  {
    return this->m_raw_writer->write( desc );
  }

  LOG_ERROR( "No raw descriptor writer format selected" );
  return false;
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer::
is_good() const
{
  if( this->m_raw_writer )
  {
    return this->m_raw_writer->is_good();
  }

  LOG_ERROR( "No raw descriptor writer format selected" );
  return false;
}

} // end namespace vidtk
