/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/config_block.h>
#include <utilities/config_block_parser.h>
#include <utilities/format_block.h>

#include <string>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cassert>
#include <vul/vul_get_timestamp.h>

#include <logger/logger.h>

VIDTK_LOGGER( "config_block_cxx" );

// Helper routines
namespace {

/// Each node has:
/// - a name, e.g. the last element of the fully-qualified config block name
/// - zero or one value instances
/// - zero or more child nodes

struct config_tree_node
{
  std::string name;
  vidtk::config_block_value* value;
  bool value_is_mandatory;
  std::vector< config_tree_node* > children;
  config_tree_node()
    : name(""), value(0), value_is_mandatory( false )
  {}
  explicit config_tree_node( const std::string& n )
    : name(n), value(0), value_is_mandatory( false )
  {}
  ~config_tree_node();

  config_tree_node& add_to_tree( const std::string path,
                                 const vidtk::config_block_value& v );
  void print( std::ostream& os, size_t indent_level, const std::string& trail ) const;
  void print_item( std::ostream& os, size_t indent_level ) const;
  void print_summary( std::ostream& os, size_t indent_level ) const;
  void print_summary_markdown( std::ostream& os, size_t indent_level ) const;

private:
  config_tree_node& operator=( const config_tree_node& );
  config_tree_node( const config_tree_node& );
};

config_tree_node
::~config_tree_node()
{
  delete this->value;
  for (size_t i=0; i<children.size(); ++i)
  {
    delete children[i];
  }
}

config_tree_node&
config_tree_node
::add_to_tree( const std::string path,
               const vidtk::config_block_value& v )
{
  // path is something like 'foo:bar:baz:num_frames'.
  // Split path into head 'foo' and tail 'bar:baz:num_frames'.
  // Head always gets added as a child.
  // If tail has no further children, then set new child's value
  // and stop recursing.
  // If tail has more children, recurse.

  const size_t bad_index = static_cast<size_t>( -1 );
  size_t p = path.find( ":" );
  std::string head = path.substr(0, p);
  size_t idx = bad_index;
  for (size_t i=0; (idx == bad_index) && (i<this->children.size()); ++i)
  {
    if ( this->children[i]->name == head )
    {
      idx = i;
    }
  }
  if (idx == bad_index)
  {
    this->children.push_back( new config_tree_node( head ));
      idx = this->children.size()-1;
  }
  if ( p == std::string::npos )
  {
    this->children[idx]->value = new vidtk::config_block_value( v );
    this->children[idx]->value_is_mandatory = ! ( v.is_optional() || v.has_default_value() || v.has_user_value() );
    return *(this->children[idx]);
  }
  else
  {
    return this->children[ idx ]->add_to_tree( path.substr( p+1 ), v );
  }
}


// ------------------------------------------------------------------
void
config_tree_node
::print_summary( std::ostream& os, size_t indent_level ) const
{
  std::string indent( indent_level, ' ' );
  // only print begin-block names, i.e. only print name if none of our children have children
  std::vector< const config_tree_node* > nodes_with_children;
  for (size_t i=0; i<children.size(); ++i)
  {
    const config_tree_node* p = children[i];
    if ( ! p->children.empty() )
    {
      nodes_with_children.push_back( p );
    }
  }
  if ( ! nodes_with_children.empty() )
  {
    os << "# " << indent << this->name << "\n";
    for (size_t i=0; i<nodes_with_children.size(); ++i)
    {
      nodes_with_children[i]->print_summary( os, indent_level+2 );
    }
  }
}


// ------------------------------------------------------------------
void
config_tree_node
::print_summary_markdown( std::ostream& os, size_t indent_level ) const
{
  std::string indent( indent_level, ' ' );
  // only print begin-block names, i.e. only print name if none of our children have children
  std::vector< const config_tree_node* > nodes_with_children;

  for ( size_t i = 0; i < children.size(); ++i )
  {
    const config_tree_node* p = children[i];
    if ( ! p->children.empty() )
    {
      nodes_with_children.push_back( p );
    }
  }

  if ( ! nodes_with_children.empty() )
  {
    if ( ! this->name.empty() )
    {
      os << indent << "- " << this->name << "\n";
    }

    for ( size_t i = 0; i < nodes_with_children.size(); ++i )
    {
      nodes_with_children[i]->print_summary_markdown( os, indent_level + 1 );
    }
  }
}


// ------------------------------------------------------------------
void
config_tree_node
::print( std::ostream& os, size_t indent_level, const std::string& trail ) const
{
  // We don't want to print singleton children as a block; first print out
  // all singleton children as values at this level, then any sub-blocks.
  std::string indent( indent_level, ' ' );

  for (size_t i=0; i<children.size(); ++i)
  {
    const config_tree_node* p = children[i];
    if (p->children.empty())
    {
      p->print_item( os, indent_level );
      os << "\n";
    }
  }
  for (size_t i=0; i<children.size(); ++i)
  {
    const config_tree_node* p = children[i];
    if (! p->children.empty() )
    {
      // root is actually nil, don't print leading ':'
      std::string new_trail =
        trail.empty()
        ? p->name
        : trail+":"+p->name;
      os << "\n";
      os << indent << "block " << p->name << "\n";
      os << indent << "# " << new_trail << "\n";
      os << "\n";
      p->print( os, indent_level+2, new_trail );
      os << indent << "endblock  # " << p->name << "\n";
    }
  }
}


// ------------------------------------------------------------------
void
config_tree_node
::print_item( std::ostream& os, size_t indent_level ) const
{
  std::string flags = "";
  std::ostringstream value_oss;
  const vidtk::config_block_value& v = *(this->value);
  bool print_commented = false;
  if (v.has_user_value())
  {
    flags += "user ";
    value_oss << v.user_value();
    print_commented = true;
  }
  if (v.has_default_value())
  {
    flags += "default ";
    value_oss << v.default_value();
  }
  if (v.is_optional())
  {
    flags += "optional ";
  }
  if (flags.empty())
  {
    flags = "MANDATORY ";
    value_oss << " # must be set by user";
    print_commented = true;
  }
  std::string indent( indent_level, ' ');
  std::string formatted_description =
    vidtk::format_block( "[ "+flags+"] "+v.description(),
                         indent+"# ",
                         indent_level+75 );
  os << formatted_description;
  os << indent;
  if ( print_commented )
  {
    os << "# ";
  }
  os << this->name << " = " << value_oss.str() << "\n";
}


// ------------------------------------------------------------------
/// Is the string \a prefix a prefix of the string \a str?
bool
is_prefixed( std::string const& str, std::string const& prefix )
{
  std::string::size_type const n = prefix.length();

  if( str.length() < n )
  {
    return false;
  }

  for( std::string::size_type i = 0; i < n; ++i )
  {
    if( str[i] != prefix[i] )
    {
      return false;
    }
  }

  return true;
}

/// Should this parameter name throw an unregistered parameter exception?
/// Yes if:
/// (a) the check list is empty, OR
/// (b) the check list is not empty, and contains a name which is a
/// prefix of the parameter name

bool
name_should_throw( const std::string& param_name, const std::vector<std::string>& check_list )
{
  if ( check_list.empty() ) return true;

  for (unsigned i=0; i<check_list.size(); ++i)
  {
    std::string prefix = check_list[i] + ":";
    if ( ( (prefix.size() <= param_name.size() ) &&
           (param_name.substr( prefix.size() ) == prefix) ))
    {
      return true;
    }
  }

  // no matches; this parameter name shouldn't throw
  return false;
}



} // end anonymous namespace



namespace vidtk
{


config_block_value
::config_block_value()
  : has_user_value_( false ),
    has_default_( false ),
    optional_( false )
{
}

config_block
::config_block()
  : fail_on_missing_block_( true )
{
}

config_block
::~config_block()
{
}

void
config_block_value
::set_user_value( std::string const& v )
{
  has_user_value_ = true;
  user_value_ = v;
}


void
config_block_value
::set_default( std::string const& v )
{
  has_default_ = true;
  default_ = v;
}


void
config_block_value
::set_optional( bool v )
{
  optional_ = v;
}


void
config_block_value
::set_description( std::string const& v )
{
  description_ = v;
}


bool
config_block_value
::has_value() const
{
  return has_user_value_ || has_default_;
}

bool
config_block_value
::has_user_value() const
{
  return has_user_value_;
}

bool
config_block_value
::has_default_value() const
{
  return has_default_;
}

std::string
config_block_value
::value() const
{
  assert( this->has_value() );
  if( has_user_value_ )
  {
    return user_value_;
  }
  else
  {
    return default_;
  }
}

bool
config_block_value
::is_optional() const
{
  return optional_;
}

std::string
config_block_value
::description() const
{
  return description_;
}

std::string
config_block_value
::default_value() const
{
  return default_;
}

std::string
config_block_value
::user_value() const
{
  return user_value_;
}


void
config_block
::add_optional( std::string const& name,
                std::string const& descr )
{
  // call this to ensure that the name doesn't already exist.
  add_parameter( name, descr );

  vmap_[name].set_optional( true );
}


void
config_block
::add_parameter( std::string const& name,
                 std::string const& description )
{
  check_if_exists( name );

  vmap_[name] = config_block_value();
  vmap_[name].set_description( description );
}


void
config_block
::add_parameter( std::string const& name,
                 std::string const& default_value,
                 std::string const& description )
{
  // call this to ensure that the name doesn't already exist.
  add_parameter( name, description );

  vmap_[name].set_default( default_value );
}


void
config_block
::add( std::string const& name, config_block_value const& val )
{
  // call this to ensure that the name doesn't already exist.
  check_if_exists( name );

  vmap_[name] = val;
}


void
config_block
::merge( config_block const& blk )
{
  typedef value_map_type::const_iterator map_iter_type;

  for( map_iter_type src_it = blk.vmap_.begin();
       src_it != blk.vmap_.end(); ++src_it )
  {
    this->add( src_it->first, src_it->second );
  }
}


void
config_block
::update( config_block const& blk )
{
  typedef value_map_type::const_iterator map_iter_type;

  for( map_iter_type src_it = blk.vmap_.begin();
       src_it != blk.vmap_.end(); ++src_it )
  {
    if( src_it->second.has_value() )
    {
      this->set( src_it->first, src_it->second );
    }
  }
}

void
config_block
::remove( config_block const& blk )
{
  typedef value_map_type::const_iterator map_iter_type;

  for( map_iter_type src_it = blk.vmap_.begin();
       src_it != blk.vmap_.end(); ++src_it )
  {
    size_t erase_size = this->vmap_.erase( src_it->first );
    if( erase_size != 1 )
    {
      std::stringstream reason;
      reason << "parameter name \"" << src_it->first << "\" has " << erase_size << " copies.";
      LOG_ERROR( reason.str() );
      throw std::runtime_error( reason.str() );
    }
  }
}


void
config_block
::add_subblock( config_block const& blk, std::string const& subblock_name )
{
  typedef value_map_type::const_iterator map_iter_type;

  for( map_iter_type src_it = blk.vmap_.begin();
       src_it != blk.vmap_.end(); ++src_it )
  {
    this->add( subblock_name + ":" + src_it->first, src_it->second );
  }
}


config_block
config_block
::subblock( std::string const& subblock_name ) const
{
  typedef value_map_type::const_iterator map_iter_type;

  config_block out_blk;

  std::string prefix = subblock_name + ":";
  std::string::size_type const n = prefix.length();

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    if( is_prefixed( it->first, prefix ) )
    {
      std::string suffix = it->first.substr( n );
      out_blk.add( suffix, it->second );
    }
  }

  return out_blk;
}


checked_bool
config_block
::set( std::string const& name, std::string const& val )
{
  value_map_type::iterator it = vmap_.find( name );

  if( it == vmap_.end() )
  {
    if( ( fail_on_missing_block_ ) && ( name_should_throw( name, check_list_ ) ) )
    {
      return checked_bool( std::string( "unregistered parameter name \"" ) + name + "\"" );
    }
    else
    {
      LOG_INFO( "ignoring the parameter (in file): " << name );
      return true;
    }
  }

  // Set the value, leaving the "default" unchanged.
  it->second.set_user_value( val );

  return true;
}


bool
config_block
::move_subblock( std::string const& old_subblock_name,
                 std::string const& new_subblock_name )
{
  if(!this->has_subblock(old_subblock_name))
  {
    return false;
  }
  if(this->has_subblock(new_subblock_name))
  {
    LOG_WARN( new_subblock_name << "is already used a subblock name" );
  }
  config_block blk = this->subblock( old_subblock_name );
  this->add_subblock( blk, new_subblock_name );
  return this->remove_subblock( old_subblock_name );
}


bool
config_block
::has_subblock( std::string const& subblock_name )
{
  typedef value_map_type::const_iterator map_iter_type;

  std::string prefix = subblock_name + ":";

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    if( is_prefixed( it->first, prefix ) )
    {
      return true;
    }
  }

  return false;
}


bool
config_block
::remove_subblock( std::string const& subblock_name )
{
  config_block to_remove;
  to_remove.add_subblock( this->subblock( subblock_name ), subblock_name );

  try
  {
    this->remove( to_remove );
  }
  catch( const config_block_parse_error& /*e*/ )
  {
    return false;
  }

  return true;
}


checked_bool
config_block
::set( std::string const& name, config_block_value const& val )
{
  value_map_type::iterator it = vmap_.find( name );

  if( it == vmap_.end() )
  {
    if( ( fail_on_missing_block_ ) && ( name_should_throw( name, check_list_ ) ) )
    {
      return checked_bool( std::string( "unregistered parameter name " ) + name );
    }
    else
    {
      LOG_INFO( "ignoring the parameter (in file): " << name );
      return true;
    }
  }

  // Set the config_block_value, preserving all members in source
  it->second = val;

  return true;
}


bool
config_block
::has( std::string const& name ) const
{
  std::string tmp;
  return this->get_raw_value( name, tmp );
}


template<>
unsigned char
config_block
::get<unsigned char>( std::string const& name ) const
{
  return this->get<unsigned>( name );
}


template<>
signed char
config_block
::get<signed char>( std::string const& name ) const
{
  return this->get<int>( name );
}


template<>
bool
config_block
::get<bool>( std::string const& name ) const
{
  std::string valstr = this->get<std::string>( name );

  // Convert from string to bool
  if( valstr == "true" ||
      valstr == "yes" ||
      valstr == "on" ||
      valstr == "1" )
  {
    return true;
  }
  else if( valstr == "false" ||
           valstr == "no" ||
           valstr == "off" ||
           valstr == "0" )
  {
    return false;
  }
  else
  {
    throw config_block_parse_error( "failed to convert to bool from \'" + valstr + "\' for " + name );
  }
}


template<>
std::string
config_block
::get<std::string>( std::string const& name ) const
{
  std::string val;
  checked_bool res = this->get_raw_value( name, val );
  if ( !res )
  {
    throw config_block_parse_error( res.message() );
  }
  return val;
}

// ----------------------------------------------------------------
/** Get for enumerated strings.
 *
 *
 */
int
config_block
::get_enum_int( config_enum_option const& table, std::string const& name ) const
{
  int ret_val;
  std::string result = this->get< std::string >( name );

  if (! table.find( result, ret_val ) )
  {
    std::stringstream msg;
    msg << "\"" << result << "\" is not a valid option for \"" << name <<
      "\" parameter. Options are " << table.list();
    throw config_block_parse_error( msg.str() );
  }

  return ret_val;
}


checked_bool
config_block
::parse( std::string const& filename )
{
  config_block_parser parser;

  checked_bool res = parser.set_filename( filename );

  if( !res )
  {
    return res;
  }

  return parser.parse( *this );
}



checked_bool
config_block
::parse_arguments( int argc, char** argv )
{
  std::stringstream sstr;
  for( int i = 1; i < argc; ++i )
  {
    sstr << argv[i] << "\n";
  }

  config_block_parser parser;

  parser.set_input_stream( sstr );

  return parser.parse( *this );
}


// ------------------------------------------------------------------
void
config_block
::print_as_tree( std::ostream& os, const std::string& argv0, const std::string& githash ) const
{
  config_tree_node root("");

  std::vector< std::string > user_values, mandatory_values;

  for ( value_map_type::const_iterator i = vmap_.begin(); i != vmap_.end(); ++i)
  {
    const config_tree_node& n = root.add_to_tree( i->first, i->second );
    if (n.value_is_mandatory)
    {
      mandatory_values.push_back( i->first );
    }
    if (i->second.has_user_value())
    {
      user_values.push_back( i->first );
    }
  }

  os << "#\n";
  os << "# This is the config file for " << argv0 << " compiled from git hash\n";
  os << "# " << githash << " generated on " << vul_get_time_as_string() << "\n";
  os << "#\n";
  os << "# This config file has " << mandatory_values.size() << " value(s) which have no default value\n";
  os << "# and are not marked optional, and " << user_values.size()
     << " values() which the user has already set.\n";
  os << "# Note that 'mandatory' values might not need to be set if their enclosing blocks are not\n";
  os << "# enabled; the config parser generating this message cannot determine if this is true or not.\n";
  os << "#\n";
  os << "# This file has the following sections:\n";
  os << "# - Mandatory values\n";
  os << "# - User values\n";
  os << "# - Full config summary\n";
  os << "# - Full config tree\n";
  os << "#\n";
  os << "# The full config tree also contains the user and mandatory values, but commented out.\n";
  os << "#\n";
  os << "\n";
  os << "\n";
  os << "# Mandatory values:\n";

  for (size_t i=0; i<mandatory_values.size(); ++i)
  {
    value_map_type::const_iterator p = vmap_.find( mandatory_values[i] );
    os << format_block( p->second.description(), "# ", 75 );
    os << mandatory_values[i] << " =\n";
    os << "\n";
  }

  os << "\n";
  os << "\n";
  os << "# User-defined values:\n";

  for (size_t i=0; i<user_values.size(); ++i)
  {
    value_map_type::const_iterator p = vmap_.find( user_values[i] );
    os << format_block( p->second.description(), "# ", 75 );
    os << user_values[i] << " = " << p->second.user_value() << "\n";
    os << "\n";
  }

  os << "\n";
  os << "# Summary config tree:\n";
  os << "\n";

  root.print_summary( os, 0 );

  os << "\n";
  os << "# Full config tree:\n";
  os << "\n";

  root.print( os, 0, "");
}


// ------------------------------------------------------------------
void
config_block
::print( std::ostream& ostr ) const
{
  typedef value_map_type::const_iterator map_iter_type;

  ostr << "# Generated at " << vul_get_time_as_string() << "\n\n"
       << "# The following values were set by the user/program.\n"
       << "# A more detailed description is available below.\n\n";

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    if( it->second.has_user_value() )
    {
      ostr << it->first << " = " << it->second.user_value() << "\n";
    }
  }

  ostr << "\n\n# The following values have no defaults and must be set.\n"
       << "# A more detailed description is available below.\n\n";

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    if( ! it->second.has_user_value() &&
        ! it->second.has_default_value() &&
        ! it->second.is_optional() )
    {
      ostr << it->first << " = \n";
    }
  }


  ostr << "\n\n# The following is the full set of parameters that are available\n";

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    ostr << "\n\n";
    if( it->second.has_user_value() )
    {
      ostr << "# " << it->first << " = " << it->second.user_value()
           << "\n# Set by user\n";
      if( it->second.has_default_value() )
      {
        ostr << "# Default = " << it->second.default_value() << "\n";
      }
      if( it->second.is_optional() )
      {
        ostr << "# Is optional\n";
      }
      ostr << "#\n"
           << format_block( it->second.description(), "# ", 75 );
    }
    else if( it->second.has_default_value() )
    {
      ostr << "# " << it->first << " = " << it->second.default_value()
           << "\n# Set to default\n#\n"
           << format_block( it->second.description(), "# ", 75 );
    }
    else if( it->second.is_optional() )
    {
      ostr << "# " << it->first
           << "=\n# Not set, no default, but optional\n#\n"
           << format_block( it->second.description(), "# ", 75 );
    }
    else
    {
      ostr << "# " << it->first
           << "=\n# Not set, no default\n#\n"
           << format_block( it->second.description(), "# ", 75 );
    }
  }
}


// ------------------------------------------------------------------
void
config_block
::print_as_markdown( std::ostream& os, const std::string& argv0 ) const
{
  typedef value_map_type::const_iterator map_iter_type;

  config_tree_node root( "" );

  std::vector< std::string > user_values, mandatory_values;

  for ( value_map_type::const_iterator i = vmap_.begin(); i != vmap_.end(); ++i )
  {
    const config_tree_node& n = root.add_to_tree( i->first, i->second );
    if ( n.value_is_mandatory )
    {
      mandatory_values.push_back( i->first );
    }
    if ( i->second.has_user_value() )
    {
      user_values.push_back( i->first );
    }
  }

  os  << "# Config file for " << argv0
      << "\n\n"
      << "This config file has " << mandatory_values.size() << " value(s) which have no default value "
      << "and are not marked optional, and " << user_values.size()
      << " value(s) which the user has already set. "
      << "Note that *mandatory* values might not need to be set if their enclosing blocks are not "
      << "enabled. The config parser generating this message cannot determine if this is true or not.\n"
      << "\n"
      << "The config description has the following sections:\n"
      << "- **Mandatory values.** These configuration items have no default values and are not marked as optional.\n"
      << "- **User values.** These configuration items have user specified values set from a configuration file "
      << " or are set internally by the program.\n"
      << "- **Full config summary tree.** Structural representation of the configuration.\n"
      << "- **Full config listing.** Detailed listing of all configuration parameters, "
      << "including user and mandatory  values.\n"
      << "\n*****\n"
      << "## Mandatory Values\n";

  for ( size_t i = 0; i < mandatory_values.size(); ++i )
  {
    value_map_type::const_iterator p = vmap_.find( mandatory_values[i] );
    os << p->second.description() << "\n";
    os << "**" << mandatory_values[i] << "** =\n";
    os << "\n";
  }

  os << "*****\n";
  os << "## User-defined values\n";

  for ( size_t i = 0; i < user_values.size(); ++i )
  {
    value_map_type::const_iterator p = vmap_.find( user_values[i] );
    os << p->second.description() << "\n";
    os << "**" << user_values[i] << "** = " << p->second.user_value() << "\n";
    os << "\n";
  }

  os << "*****\n"
     << "## Summary Config Tree\n"
     << "Indented listing of configuration structure.\n";

  root.print_summary_markdown( os, 0 );

  os << "*****\n"
     << "## Full Config Tree\n"
     << "### The following is the full set of parameters that are available.\n";

  for ( map_iter_type it = vmap_.begin(); it != vmap_.end(); ++it )
  {
    os << "\n\n";
    if ( it->second.has_user_value() )
    {
      os  << "**" << it->first << "** = " << it->second.user_value()
          << "\n(Set by user.)\n";

      if ( it->second.has_default_value() )
      {
        os << "Default = " << it->second.default_value() << "\n";
      }

      if ( it->second.is_optional() )
      {
        os << "(Is optional.)\n";
      }

      os  << "\n" << it->second.description();
    }
    else if ( it->second.has_default_value() )
    {
      os  << "**" << it->first << "** = " << it->second.default_value()
          << "\n(Set to default.)\n"
          << it->second.description();
    }
    else if ( it->second.is_optional() )
    {
      os  << "**" << it->first
          << "** =\n(Not set, no default, but optional.)\n"
          << it->second.description();
    }
    else
    {
      os  << "**" << it->first
          << "** =\n(Not set, no default.)\n"
          << it->second.description();
    }
  }
} // config_block::print_as_markdown


// ------------------------------------------------------------------
void
config_block
::set_fail_on_missing_block( bool flag )
{
  fail_on_missing_block_ = flag;
}


// ------------------------------------------------------------------
std::map< std::string, config_block_value >
config_block
::enumerate_values() const
{
  return vmap_;
}


// ------------------------------------------------------------------
void
config_block
::add_component_to_check_list( const std::string& name )
{
  check_list_.push_back( name );
}


// ------------------------------------------------------------------
checked_bool
config_block
::get_has_user_value( std::string const& parameter_name, bool & has_user_value ) const
{
  checked_bool found = this->has( parameter_name );
  if ( found )
  {
    std::map< std::string, config_block_value >::const_iterator it;
    it = vmap_.find( parameter_name );
    has_user_value = it->second.has_user_value();
    return true;
  }
  else
  {
    has_user_value = false;
    return found;
  }
}


// ------------------------------------------------------------------
checked_bool
config_block
::get_raw_value( std::string const& name, std::string& val ) const
{
  value_map_type::const_iterator it = vmap_.find( name );

  if( it == vmap_.end() )
  {
    return checked_bool( std::string( "parameter name not found: " ) + name );
  }
  if( ! it->second.has_value() )
  {
    return checked_bool( std::string("value not set, and no default available, for ") + name );
  }

  val = it->second.value();
  return true;
}


// ------------------------------------------------------------------
void
config_block
::check_if_exists( std::string const& name )
{
  if (vmap_.count( name ))
  {
    std::string const reason = "parameter name \"" + name + "\" already exists";
    LOG_ERROR( reason );
    throw std::runtime_error( reason );
  }
}


// ================================================================
// config_enum_option methods

config_enum_option&
config_enum_option
::add( std::string const& name, int val )
{
  this->list_[name] = val;
  return *this;
}


// ------------------------------------------------------------------
bool
config_enum_option
::find( std::string const& name, int& ret_val ) const
{
  std::map< std::string, int >::const_iterator ix;
  ix = list_.find( name );
  if (ix == list_.end() )
  {
    return false; // not found
  }

  ret_val = ix->second;
  return true;
}


// ------------------------------------------------------------------
std::string
config_enum_option
::list() const
{
  std::string ret_val;

  std::map< std::string, int >::const_iterator ix = list_.begin();
  for (; ix != list_.end(); ++ix )
  {
    if ( ! ret_val.empty() ) { ret_val += ", "; }
    ret_val += "\"" + ix->first + "\"" ;
  }

  return ret_val;
}

} // end namespace vidtk
