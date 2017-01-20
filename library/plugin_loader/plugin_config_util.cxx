/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "plugin_config_util.h"

#include <plugin_loader/plugin_factory.h>
#include <utilities/config_block.h>

#include <logger/logger.h>

namespace vidtk {

VIDTK_LOGGER( "plugin_config_util" );

// Special config attribute prefix. This is used to indicate that an
// attribute is destined to be added to the config.
const std::string CONFIG_PREFIX( "(config)" );


// ------------------------------------------------------------------
void add_config_attribute( plugin_factory& fact,
                           std::string const& attr,
                           std::string const& description )
{
  fact.add_attribute( CONFIG_PREFIX + attr, description );
}


// ------------------------------------------------------------------
void add_config_attribute( plugin_factory& fact,
                           std::string const& attr,
                           std::string const& def_val,
                           std::string const& description )
{
  std::string val;
  val = def_val + '\004' + description; // insert separator in string
  fact.add_attribute( CONFIG_PREFIX + attr, val );
}


// ------------------------------------------------------------------

struct update_config_functor
{
  update_config_functor( config_block& block )
    : m_block( block )
  { }

  // There is a semantic issue here when we are collecting config
  // entries.  In some polymorphic cases, all leaf nodes may
  // legitimately register the same config entry (such as
  // filename). In this case subsequent appearances of the same config
  // key should overwrite or not be added a second time.
  //
  // This brings up what to do when both (or all) config entries with
  // the same key have different defaults.
  // - Remove default from entry and force configuration from the file.
  // - Log an error/warning and terminate
  // - modify our API and not allow default values int the config attribs.

  void operator()( std::string const& key, std::string const& val )
  {
    // If the key has the super secret config prefix, remove that
    // prefix and add the rest to the config.
    if (key.find( CONFIG_PREFIX ) == 0 )
    {
      const std::string name = key.substr( CONFIG_PREFIX.size() ) ;
      try
      {
        // See if we have the two parameter version or the three?
        // If the \004 marker appears in the string anywhere, we have 3
        size_t idx =  val.find( '\004' );
        if ( std::string::npos != idx )
        {
          // If the marker happens to be the first element in the string,
          // we have an empty string as the default, let that pass through.
          // Otherwise, get the substring that represents the default value.
          std::string def_val = "";
          if ( 0 != idx )
          {
            def_val = val.substr( 0, idx );
          }

          const std::string descrip = val.substr( idx+1 );

          // Add with description value
          if ( m_block.has( name ) )
          {
            m_block.set( name, def_val );
          }
          else
          {
            m_block.add_parameter( name, def_val, descrip );
          }
        }
        else
        {
          // Add without description
          if ( m_block.has( name ) )
          {
            m_block.set( name, val );
          }
          else
          {
            m_block.add_parameter( name, val );
          }
        }
      }
      catch (std::runtime_error const& e)
      {
        LOG_WARN( "Exception caught merging configs: " << e.what() );
      }
    }
  }

  // Member data
  config_block& m_block;
};


// ------------------------------------------------------------------
void collect_plugin_config( plugin_factory& fact, config_block& block )
{
  update_config_functor ucf( block );
  fact.for_each_attr( ucf );
}

} // end namespace
