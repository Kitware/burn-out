/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_writer_process.h"

#include <tracking_data/io/image_object_writer.h>

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include <logger/logger.h>

#include <tracking_data/image_object.h>
#include <utilities/timestamp.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>

namespace vidtk {

VIDTK_LOGGER ("image_object_writer_process");

// ================================================================
/** Constructor
 *
 *
 */
image_object_writer_process
::image_object_writer_process( std::string const& _name )
  : process( _name, "image_object_writer_process" ),
    disabled_( true )
{
  std::string config_help;

  // Get a list of plugin factories that support our interface type so
  // we can create the allowable format help.
  plugin_factory_vector_t factory_list =
    plugin_manager::instance()->get_factories( typeid(vidtk::image_object_writer).name() );

  BOOST_FOREACH( plugin_factory_handle_t facto, factory_list )
  {
    // Collect format help messages
    std::string msg;
    facto->get_attribute( plugin_factory::CONFIG_HELP, msg );
    config_help += msg + "\n";
  }

  config_.add_parameter( "disabled", "true", "Process state" );
  config_.add_parameter( "format", "0", config_help );
  config_.add_parameter( "filename", "", "Output filename" );
  config_.add_parameter( "overwrite_existing", "false", "Overwrite existing file" );
}


image_object_writer_process
::~image_object_writer_process()
{
  BOOST_FOREACH (image_object_writer* wrt, this->the_writer_)
  {
    delete wrt;
  }
}


// ------------------------------------------------------------------
config_block
image_object_writer_process
::params() const
{
  return config_;
}


// ------------------------------------------------------------------
bool
image_object_writer_process
::set_params( config_block const& blk )
{

  try
  {
    this->disabled_ = blk.get<bool>( "disabled" );
    format_ = blk.get<std::string>( "format" );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


// ----------------------------------------------------------------
bool
image_object_writer_process
::initialize()
{
  if( this->disabled_ )
  {
    return true;
  }

  // Get a list of plugin factories that support our interface type.
  plugin_factory_vector_t factory_list =
    plugin_manager::instance()->get_factories( typeid(vidtk::image_object_writer).name() );

  BOOST_FOREACH( plugin_factory_handle_t facto, factory_list )
  {
    // get name of file type supported
    std::string type_attr;
    if ( ! facto->get_attribute( "file-type", type_attr ) )
    {
      LOG_WARN( "Track reader plugin did not register file-types supported" );
      continue;
    }

    // Split file types into tokens
    std::vector< std::string> file_types;
    boost::split( file_types, type_attr, boost::is_any_of(" ," ) );

    // @todo The structure is in place to support multiple writers, but
    // has not been fully realized. All that has to be done is split the
    // format parameter into words and process each one as a format type.

    // Add this writer for each format it supports
    BOOST_FOREACH( std::string file_type, file_types )
    {
      // Check if this is the requested file type
      if (this->format_ == file_type)
      {
        this->the_writer_.push_back ( facto->create_object< vidtk::image_object_writer >() );
      }
    }
  }

  // If we did not instantiate a writer, then the format must be bad.
  if ( this->the_writer_.empty() )
  {
    LOG_ERROR( name() << ": unrecognized  format " << this->format_ );
    return false;
  }

  // Initialize all writers
  BOOST_FOREACH (image_object_writer * wrt, this->the_writer_)
  {
    if ( wrt->initialize(this->config_) == false)
    {
      return false;
    }
  }
  return true;
}


// ----------------------------------------------------------------
bool
image_object_writer_process
::step()
{
  if( this->disabled_ )
  {
    return false;
  }

  // cycle all writers
  BOOST_FOREACH (image_object_writer * wrt, this->the_writer_)
  {
    wrt->write( ts_, src_objs_ );
  }

  // reset inputs
  src_objs_.clear();
  ts_ = timestamp();

  return true;
}


// ----------------------------------------------------------------
// -- inputs --
void
image_object_writer_process
::set_image_objects( std::vector< image_object_sptr > const& objs )
{
  src_objs_ = objs;
}


void
image_object_writer_process
::set_timestamp( timestamp const& ts )
{
  ts_ = ts;
}


bool
image_object_writer_process
::is_disabled() const
{
  return this->disabled_;
}

} // end namespace vidtk
