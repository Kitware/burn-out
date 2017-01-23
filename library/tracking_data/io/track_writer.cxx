/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_writer.h"
#include "track_writer_interface.h"

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>
#include <plugin_loader/plugin_config_util.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("track_writer");

track_writer
::track_writer( )
:writer_(NULL)
{
  // Get a list of plugin factories that support our interface type.
  plugin_factory_vector_t factory_list =
    plugin_manager::instance()->get_factories( typeid(vidtk::track_writer_interface).name() );

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
    writer_formats_ += msg + "\n";

    // Collect any config items
    collect_plugin_config( *facto, m_plugin_config );

    std::vector< std::string> file_types;
    boost::split( file_types, type_attr, boost::is_any_of(" ," ) );

    /// @todo - do the following:
    // This may be a little wasteful in that we are creating a writer
    // for all available formats even though we will only use
    // one. This could be optimized to keep a list of the factories
    // only and create the desired writer later.
    track_writer_interface_sptr tmp = facto->create_object< vidtk::track_writer_interface >();

    // Add this writer for each format it supports
    BOOST_FOREACH( std::string file_type, file_types )
    {
      LOG_DEBUG( "track_writer: Registering writer \"" << name << "\" for type: " << file_type );
      this->add_writer( file_type, tmp );
    }
  }
}


track_writer
::~track_writer()
{
  if(this->is_open())
  {
    this->close();
  }
}


// ------------------------------------------------------------------
bool track_writer
::write( std::vector<vidtk::track_sptr> const& tracks )
{
  return writer_ && writer_->write(tracks);
}


// ------------------------------------------------------------------
bool
track_writer
::write( std::vector<vidtk::track_sptr> const& tracks,
            std::vector<vidtk::image_object_sptr> const* ios,
            timestamp const& ts )
{
  return writer_ && writer_->write(tracks, ios, ts);
}


// ------------------------------------------------------------------
bool
track_writer
::set_format(std::string const& format)
{
  writer_ = get_writer(format);
  return writer_ != NULL;
}


// ------------------------------------------------------------------
bool
track_writer
::open( std::string const& fname )
{
  if(writer_ == NULL && !fname.empty())
  {
    std::string file_ext = fname.substr(fname.rfind(".")+1);
    writer_ = get_writer(file_ext);
  }
  return writer_ && writer_->open(fname);
}


// ------------------------------------------------------------------
bool
track_writer
::is_open() const
{
  return writer_ && writer_->is_open();
}


// ------------------------------------------------------------------
void
track_writer
::close()
{
  if(writer_)
  {
    writer_->close();
  }
}


// ------------------------------------------------------------------
bool
track_writer
::is_good() const
{
  return writer_ && writer_->is_open();
}


// ------------------------------------------------------------------
void
track_writer
::set_options(track_writer_options const& options)
{
  if(writer_)
  {
    writer_->set_options(options);
  }
}


// ------------------------------------------------------------------
void
track_writer
::add_writer(std::string const& tag, track_writer_interface_sptr w)
{
  writer_options_[tag] = w;
}


// ------------------------------------------------------------------
track_writer_interface_sptr
track_writer
::get_writer(std::string const& tag)
{
  std::map<std::string, track_writer_interface_sptr>::iterator iter = writer_options_.find(tag);
  if(iter == writer_options_.end())
  {
    LOG_ERROR("Writer format not found: " << tag);
    return NULL;
  }

  return iter->second;
}


// ------------------------------------------------------------------
std::string const&
track_writer
::get_writer_formats() const
{
  return this->writer_formats_;
}


// ------------------------------------------------------------------
config_block const&
track_writer
::get_writer_config() const
{
  return m_plugin_config;
}


}
