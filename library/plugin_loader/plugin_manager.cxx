/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "plugin_manager.h"
#include "plugin_factory.h"

#include <logger/logger.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#ifndef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 3
#else
#if BOOST_FILESYSTEM_VERSION == 2
#error "Only boost::filesystem version 3 is supported."
#endif
#endif

#include <boost/filesystem/path.hpp>

#include <sstream>

#include <vidtksys/SystemTools.hxx>
#include <vidtksys/DynamicLoader.hxx>

#if defined __linux__
  #include <cxxabi.h>
#endif

#include <plugin_loader/module-paths.h>


namespace vidtk {

VIDTK_LOGGER( "plugin_manager" );

namespace {

typedef boost::filesystem::path::string_type path_string_t;
typedef boost::filesystem::path path_t;
typedef std::vector<path_string_t> module_paths_t;
typedef std::string lib_suffix_t;
typedef std::string function_name_t;

/// A path on the filesystem.
typedef boost::filesystem::path path_t;

/// A collection of paths on the filesystem.
typedef std::vector<path_t> paths_t;

} // end anon namespace

static bool is_separator( path_string_t::value_type ch );

static std::string const library_suffix = LIBRARY_SUFFIX;  // Supplied by build system
static path_string_t const default_module_dirs = path_string_t(DEFAULT_MODULE_PATHS); // supplied by build system
static const std::string module_envvar( "VIDTK_MODULE_PATH" );

typedef vidtksys::DynamicLoader DL;


// ================================================================
//
// Static data for singleton
//
plugin_manager* plugin_manager::s_instance = 0;


// ==================================================================
/**
 * @brief Plugin manager private implementation.
 *
 */
class plugin_manager_impl
{
public:
  plugin_manager_impl( plugin_manager* parent)
    : m_parent( parent )
  { }

  ~plugin_manager_impl() { }

  void load_known_modules();
  void look_in_directory(path_t const& directory);
  void load_from_module(path_t const& path);

  void print( std::ostream& str ) const;

  plugin_manager* m_parent;

  // Map from interface type name to vector of class loaders
  plugin_map_t m_plugin_map;

  // Map to keep track of the modules we have opened and loaded.
  typedef std::map< std::string, vidtksys::DynamicLoader::LibraryHandle > library_map_t;
  library_map_t m_library_map;

  // Name of current module file we are processing
  std::string m_current_filename;

}; // end class plugin_manager_impl


// ==================================================================
plugin_manager*
plugin_manager::
instance()
{
  static boost::mutex instance_lock;

  if ( 0 == s_instance )
  {
    boost::lock_guard< boost::mutex > lock( instance_lock );

    if ( 0 == s_instance )
    {
      s_instance = new plugin_manager();
    }
  }

  return s_instance;
}


// ------------------------------------------------------------------
plugin_manager
::plugin_manager()
  : m_impl( new plugin_manager_impl( this ) )
{
  // Load all applicable plugins
  m_impl->load_known_modules();
}


plugin_manager
::~plugin_manager()
{ }


// ------------------------------------------------------------------
plugin_factory_vector_t const&
plugin_manager
::get_factories( std::string const& type_name ) const
{
  static plugin_factory_vector_t empty; // needed for error case

  plugin_map_t::const_iterator it = m_impl->m_plugin_map.find(type_name);
  if ( it == m_impl->m_plugin_map.end() )
  {
    return empty;
  }

  return it->second;
}


// ------------------------------------------------------------------
plugin_factory_handle_t
plugin_manager
::add_factory( plugin_factory* fact )
{
  fact->add_attribute( plugin_factory::PLUGIN_FILE_NAME, m_impl->m_current_filename );

  std::string interface_type;
  fact->get_attribute( plugin_factory::INTERFACE_TYPE, interface_type );

  /// @todo make sure factory is not already in the list
  /// Check the two types as a signature.
  plugin_factory_handle_t fact_handle( fact );
  m_impl->m_plugin_map[interface_type].push_back( fact_handle );

  std::string ift;
  fact->get_attribute( plugin_factory::INTERFACE_TYPE, ift );

  std::string ct;
  fact->get_attribute( plugin_factory::CONCRETE_TYPE, ct );

  LOG_TRACE( "Adding plugin to create interface: " << ift
             << " from derived type: " << ct
             << " from file: " << m_impl->m_current_filename );

  return fact_handle;
}


// ------------------------------------------------------------------
plugin_map_t const&
plugin_manager
::get_plugin_map() const
{
  return m_impl->m_plugin_map;
}


std::string
plugin_manager::
get_default_path() const
{
  return std::string( path_t( default_module_dirs ).string() );
}


std::vector< std::string >
plugin_manager
::get_file_list() const
{
  std::vector< std::string > retval;

  BOOST_FOREACH( plugin_manager_impl::library_map_t::value_type it, m_impl->m_library_map )
  {
    retval.push_back( it.first );
  } // end foreach

  return retval;
}


// ------------------------------------------------------------------
/**
 * @brief Load all known modules.
 *
 */
void
plugin_manager_impl
::load_known_modules()
{
  module_paths_t module_dirs;

  const char * env_ptr = vidtksys::SystemTools::GetEnv( module_envvar );
  if ( env_ptr )
  {
    std::string const extra_module_dirs(env_ptr);
    boost::split( module_dirs, extra_module_dirs, is_separator, boost::token_compress_on );
  }

  module_paths_t module_dirs_tmp;
  boost::split(module_dirs_tmp, default_module_dirs, is_separator, boost::token_compress_on);
  module_dirs.insert(module_dirs.end(), module_dirs_tmp.begin(), module_dirs_tmp.end());

  BOOST_FOREACH( path_string_t const & module_dir, module_dirs )
  {
    look_in_directory( module_dir );
  }
}


// ------------------------------------------------------------------
void
plugin_manager_impl
::look_in_directory( path_t const& directory )
{
  LOG_TRACE( "Looking in directory \"" << directory.string() << "\" for loadable modules" );

  if ( directory.empty() )
  {
    return;
  }

  if ( ! boost::filesystem::exists( directory.native() ) )
  {
    LOG_WARN( "Directory: \"" << directory.string() << "\" doesn't exist." );
    return;
  }

  if ( ! boost::filesystem::is_directory( directory.native() ) )
  {
    LOG_WARN( "Path: \"" << directory.string() << "\" is not a directory" );
    return;
  }

  boost::system::error_code ec;
  boost::filesystem::directory_iterator module_dir_iter( directory, ec );

  while ( module_dir_iter != boost::filesystem::directory_iterator() )
  {
    boost::filesystem::directory_entry const ent = *module_dir_iter;

    ++module_dir_iter;

    if ( ! boost::ends_with( ent.path().native(), library_suffix ) )
    {
      continue;
    }

    if ( ent.status().type() != boost::filesystem::regular_file )
    {
      LOG_WARN( "Found a non-file matching path: \"" << directory.string() << "\"" );
      continue;
    }

    load_from_module( ent.path().native() );
  }
} // plugin_manager_impl::look_in_directory


// ----------------------------------------------------------------
/**
 * \brief Load single module from shared object / DLL
 *
 * @param path Name of module to load.
 */
void
plugin_manager_impl
::load_from_module( path_t const& path )
{
  DL::LibraryHandle lib_handle;

  m_current_filename = path.string();

  LOG_DEBUG( "Loading plugins from: " << path.string() );

  lib_handle = DL::OpenLibrary( path.string().c_str() );
  if ( ! lib_handle )
  {
    std::stringstream str;
    LOG_WARN( "plugin_manager::Unable to load shared library \""  << path.string() << "\" : "
              << DL::LastError() );
    return;
  }

  DL::SymbolPointer fp =
    DL::GetSymbolAddress( lib_handle, "register_factories" );
  if ( 0 == fp )
  {
    std::string str("Unknown error");
    char const* last_error = DL::LastError();
    if ( last_error )
    {
      str = std::string( last_error );
    }

    LOG_WARN( "plugin_manager:: Unable to bind to function \"register_factories()\" : "
              << last_error );

    DL::CloseLibrary( lib_handle );
    return;
  }

  // Save currently opened library in map
  m_library_map[path.string()] = lib_handle;

  typedef void (* reg_fp_t)( plugin_manager* );

  reg_fp_t reg_fp = reinterpret_cast< reg_fp_t > ( fp );

  ( *reg_fp )( m_parent ); // register plugins
}


// ------------------------------------------------------------------
bool
is_separator( path_string_t::value_type ch )
{
  path_string_t::value_type const separator =
#if defined ( _WIN32 ) || defined ( _WIN64 )
    ';';
#else
    ':';
#endif

    return ( ch == separator );
}

} // end namespace
