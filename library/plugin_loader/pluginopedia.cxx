/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <vul/vul_arg.h>

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>
#include <vidtksys/SystemTools.hxx>
#include <vidtksys/RegularExpression.hxx>

#include <map>
#include <vector>
#include <string>
#include <stdlib.h>

#include <boost/foreach.hpp>

#if defined __linux__
  #include <cxxabi.h>
#endif


// ------------------------------------------------------------------
/*
 * Functor to print an attribute
 */
struct print_functor
{
  print_functor( std::ostream& str)
    : m_str( str )
  { }

  void operator() ( std::string const& key, std::string const& val ) const
  {
    // Skip canonical names and other attributes that are displayed elsewhere
    if ( ( vidtk::plugin_factory::PLUGIN_NAME != key)
         && ( vidtk::plugin_factory::CONCRETE_TYPE != key )
         && ( vidtk::plugin_factory::INTERFACE_TYPE != key )
         && ( vidtk::plugin_factory::PLUGIN_DESCRIPTION != key )
         && ( vidtk::plugin_factory::PLUGIN_FILE_NAME != key ) )
    {
      std::string value(val);

      size_t pos = value.find( '\004' );
      if ( std::string::npos != pos)
      {
        value.replace( pos, 1, "\"  ::  \"" );
      }

      m_str << "    * " << key << ": \"" << value << "\"" << std::endl;
    }
  }

  std::ostream& m_str;
};


// ------------------------------------------------------------------
std::string
demangle_symbol( std::string const& sym )
{
#if defined __linux__
  std::string tname;
  int status;
  char* demangled_name = abi::__cxa_demangle(sym.c_str(), NULL, NULL, &status);
  if( 0 == status )
  {
    tname = demangled_name;
    std::free( demangled_name );
  }
  return tname;
#else
  return sym;
#endif
}

// -- local static storage --
static bool opt_detail(false);
static bool opt_brief(false);
static bool opt_fact(false);
static  vidtksys::RegularExpression fact_regex;

// ==================================================================
int main(int argc, char *argv[])
{

  vul_arg<bool> arg_print_path(
    "--print-path",
    "Print the module search path", false );

  vul_arg<bool> arg_details(
    "--details",
    "Display all attributes for plugins", false );

  vul_arg<bool> arg_files(
    "--files",
    "Display files successfully opened to load plugins" );

  vul_arg<bool> arg_brief(
    "--brief",
    "Display factory names only" );

  vul_arg<std::string> arg_fact(
    "--fact",
    "Display factories and implementations that match the regexp", ".*" );

  vul_arg_parse( argc, argv );

  opt_detail = arg_details();
  opt_brief = arg_brief();

  if ( arg_fact.set() )
  {
    opt_fact = true;
    if ( ! fact_regex.compile( arg_fact() ) )
    {
      std::cerr << "Invalid regular expression \"" << arg_fact() << "\"" << std::endl;
      return 1;
    }
  }

  vidtk::plugin_manager* pm = vidtk::plugin_manager::instance();
  vidtk::plugin_map_t const& plugin_map = pm->get_plugin_map();

  if ( arg_print_path() )
  {
    std::cout << "\n\nModule discovery path:" << std::endl;

    const char * env_ptr = vidtksys::SystemTools::GetEnv( "VIDTK_MODULE_PATH" );
    if ( env_ptr )
    {
      std::cout << "    From environment: " << env_ptr << std::endl;
    }
    else
    {
      std::cout << "    Environment path VIDTK_MODULE_PATH not set " << std::endl;
    }

    std::cout << "    Default path: " << pm->get_default_path()
              << std::endl << std::endl;
  }

  std::cout << "-------- Plugins discovered --------";
  BOOST_FOREACH( vidtk::plugin_map_t::value_type it, plugin_map )
  {
    std::string ds = demangle_symbol( it.first );

    // If regexp matching is enabled, and this does not match, skip it
    if ( opt_fact && ( ! fact_regex.find( ds ) ) )
    {
      continue;
    }

    std::cout << "\nFactories that create type \"" << ds << "\"" << std::endl;

    if ( opt_brief )
    {
      continue;
    }

    // Get vector of factories
    vidtk::plugin_factory_vector_t const & facts = it.second;

    BOOST_FOREACH( vidtk::plugin_factory_handle_t const fact, facts )
    {
      // Print the required fields first
      std::string buf;
      buf = "-- Not Set --";
      fact->get_attribute( vidtk::plugin_factory::PLUGIN_NAME, buf );
      std::cout << "  Plugin name: " << buf << std::endl;

      if ( opt_detail)
      {
        buf = "-- Not Set --";
        if ( fact->get_attribute( vidtk::plugin_factory::CONCRETE_TYPE, buf ) )
        {
          buf = demangle_symbol( buf );
        }
        std::cout << "      Creates concrete type: " << buf << std::endl;
      }

      buf = "-- Not Set --";
      fact->get_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, buf );
      std::cout << "      Description: " << buf << std::endl;

      if ( opt_detail )
      {
        buf = "-- Not Set --";
        fact->get_attribute( vidtk::plugin_factory::PLUGIN_FILE_NAME, buf );
        std::cout << "      Plugin loaded from file: " << buf << std::endl;

        // print all the rest of the attributes
        print_functor pf( std::cout );
        fact->for_each_attr( pf );
      }

    } // end foreach factory
  } // end foreach interface type

  if ( arg_files() )
  {
    std::vector< std::string > file_list = pm->get_file_list();

    std::cout << "\n-------- Files Successfully Opened --------" << std::endl;
    BOOST_FOREACH( std::string const& name, file_list )
    {
      std::cout << name << std::endl;
    } // end foreach
  }

  return 0;
}
