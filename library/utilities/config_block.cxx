/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/config_block.h>
#include <utilities/config_block_parser.h>
#include <utilities/log.h>
#include <utilities/format_block.h>

#include <vcl_string.h>
#include <vcl_exception.h>
#include <vcl_iostream.h>
#include <vcl_cassert.h>
#include <vul/vul_get_timestamp.h>


// Helper routines
namespace {


/// Is the string \a prefix a prefix of the string \a str?
bool
is_prefixed( vcl_string const& str, vcl_string const& prefix )
{
  vcl_string::size_type const n = prefix.length();

  if( str.length() < n )
  {
    return false;
  }

  for( vcl_string::size_type i = 0; i < n; ++i )
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
name_should_throw( const vcl_string& param_name, const vcl_vector<vcl_string>& check_list )
{
  if ( check_list.empty() ) return true;

  for (unsigned i=0; i<check_list.size(); ++i)
  {
    vcl_string prefix = check_list[i] + ":";
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

void
config_block_value
::set_user_value( vcl_string const& v )
{
  has_user_value_ = true;
  user_value_ = v;
}


void
config_block_value
::set_default( vcl_string const& v )
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
::set_description( vcl_string const& v )
{
  description_ = v;
}


bool
config_block_value
::has_value() const
{
  return has_user_value_ || has_default_;
}


vcl_string
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


void
config_block::
add( vcl_string const& name )
{
  value_map_type::iterator it = vmap_.find( name );

  if( it != vmap_.end() )
  {
    // Define a better exception to throw.
    log_error( "parameter name \"" << name << "\" already exists" << vcl_endl );
    throw vcl_exception();
  }

  vmap_[name] = config_block_value();
}


void
config_block
::add( vcl_string const& name, vcl_string const& default_value )
{
  // call this to ensure that the name doesn't already exist.
  add( name );

  vmap_[name].set_default( default_value );
}


void
config_block
::add_optional( vcl_string const& name,
                vcl_string const& descr )
{
  // call this to ensure that the name doesn't already exist.
  add_parameter( name, descr );

  vmap_[name].set_optional( true );
}


void
config_block
::add_parameter( vcl_string const& name,
                 vcl_string const& description )
{
  value_map_type::iterator it = vmap_.find( name );

  if( it != vmap_.end() )
  {
    // Define a better exception to throw.
    log_error( "parameter name \"" << name << "\" already exists" << vcl_endl );
    throw vcl_exception();
  }

  vmap_[name] = config_block_value();
  vmap_[name].set_description( description );
}


void
config_block
::add_parameter( vcl_string const& name,
                 vcl_string const& default_value,
                 vcl_string const& description )
{
  // call this to ensure that the name doesn't already exist.
  add_parameter( name, description );

  vmap_[name].set_default( default_value );
}


void
config_block
::add( vcl_string const& name, config_block_value const& val )
{
  // call this to ensure that the name doesn't already exist.
  add( name );

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
    //vcl_cout << "first = " << src_it->first << "\nsecond = "
    //    << src_it->second.value() << vcl_endl << vcl_endl;
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
      //vcl_cout << "first = " << src_it->first << "\nsecond = "
      //  << src_it->second.value() << vcl_endl << vcl_endl;
      this->set( src_it->first, src_it->second.value() );
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
    unsigned erase_size = this->vmap_.erase( src_it->first );
    if( erase_size != 1 )
    {
      log_error( "parameter name \"" << src_it->first 
                  << "\" has "<< erase_size << " copies." << vcl_endl );
      throw vcl_exception();
    }
  }
}


void
config_block
::add_subblock( config_block const& blk, vcl_string const& subblock_name )
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
::subblock( vcl_string const& sub_block_name ) const
{
  typedef value_map_type::const_iterator map_iter_type;

  config_block out_blk;

  vcl_string prefix = sub_block_name + ":";
  vcl_string::size_type const n = prefix.length();

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    if( is_prefixed( it->first, prefix ) )
    {
      vcl_string suffix = it->first.substr( n );
      out_blk.add( suffix, it->second );
    }
  }

  return out_blk;
}


checked_bool
config_block
::set( vcl_string const& name, vcl_string const& val )
{
  value_map_type::iterator it = vmap_.find( name );

  if( it == vmap_.end() )
  {
    if ( ( fail_on_missing_block_ ) && ( name_should_throw( name, check_list_ )) )
      return checked_bool( vcl_string("unregistered parameter name ")+name );
    else
    {
      vcl_cout<<"ignoring the parameter (in file): "<<name<<vcl_endl;
      return true;
    }
  }
  
  // Set the value, leaving the "default" unchanged.
  it->second.set_user_value( val );

  return true;
}


bool
config_block
::has( vcl_string const& name ) const
{
  vcl_string tmp;
  return get( name, tmp );
}


checked_bool
config_block
::get( vcl_string const& name, unsigned char& val ) const
{
  int v;
  checked_bool r = this->get( name, v );

  if( r )
  {
    val = (unsigned char)(v);
    return true;
  }
  else
  {
    return r;
  }
}


checked_bool
config_block
::get( vcl_string const& name, signed char& val ) const
{
  int v;
  checked_bool r = this->get( name, v );

  if( r )
  {
    val = (signed char)(v);
    return true;
  }
  else
  {
    return r;
  }
}


checked_bool
config_block
::get( vcl_string const& name, vcl_string& val ) const
{
  value_map_type::const_iterator it = vmap_.find( name );

  if( it == vmap_.end() )
  {
    return checked_bool( vcl_string( "parameter name not found: " ) + name );
  }

  if( ! it->second.has_value() )
  {
    return checked_bool( vcl_string("value not set, and no default available, for ") + name );
  }

  val = it->second.value();
  return true;
}


checked_bool
config_block
::get( vcl_string const& name, bool& val ) const
{
  vcl_string valstr;
  checked_bool res = this->get( name, valstr );
  if( ! res )
  {
    return res;
  }

  // Convert from string to bool
  if( valstr == "true" ||
      valstr == "yes" ||
      valstr == "on" || 
      valstr == "1" )
  {
    val = true;
    return true;
  }
  else if( valstr == "false" ||
           valstr == "no" ||
           valstr == "off" || 
           valstr == "0" )
  {
    val = false;
    return true;
  }
  else
  {
    return checked_bool( vcl_string("failed to convert from string representation to bool for ")+name );
  }
}


checked_bool
config_block
::parse( vcl_string const& filename )
{
  config_block_parser parser;

  checked_bool res = parser.set_filename( filename );
  if( ! res )
  {
    return res;
  }

  return parser.parse( *this );
}



checked_bool
config_block
::parse_arguments( int argc, char** argv )
{
  vcl_stringstream sstr;
  for( int i = 1; i < argc; ++i )
  {
    sstr << argv[i] << "\n";
  }

  config_block_parser parser;

  parser.set_input_stream( sstr );

  return parser.parse( *this );
}


void
config_block
::print( vcl_ostream& ostr ) const
{
  typedef value_map_type::const_iterator map_iter_type;

  ostr << "# Generated at " << vul_get_time_as_string() << "\n\n"
       << "# The following values were set by the user/program.\n"
       << "# A more detailed description is available below.\n\n";

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    if( it->second.has_user_value_ )
    {
      ostr << it->first << " = " << it->second.user_value_ << "\n";
    }
  }

  ostr << "\n\n# The following values have no defaults and must be set.\n"
       << "# A more detailed description is available below.\n\n";

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    if( ! it->second.has_user_value_ &&
        ! it->second.has_default_ &&
        ! it->second.optional_ )
    {
      ostr << it->first << " = \n";
    }
  }


  ostr << "\n\n# The following is the full set of parameters that are available\n";

  for( map_iter_type it = vmap_.begin();
       it != vmap_.end(); ++it )
  {
    ostr << "\n\n";
    if( it->second.has_user_value_ )
    {
      ostr << "# " << it->first << " = " << it->second.user_value_
           << "\n# Set by user\n";
      if( it->second.has_default_ )
      {
        ostr << "# Default = " << it->second.default_ << "\n";
      }
      if( it->second.optional_ )
      {
        ostr << "# Is optional\n";
      }
      ostr << "#\n"
           << format_block( it->second.description_, "# ", 75 );
    }
    else if( it->second.has_default_ )
    {
      ostr << "# " << it->first << " = " << it->second.default_
           << "\n# Set to default\n#\n"
           << format_block( it->second.description_, "# ", 75 );
    }
    else if( it->second.optional_ )
    {
      ostr << "# " << it->first
           << "=\n# Not set, no default, but optional\n#\n"
           << format_block( it->second.description_, "# ", 75 );
    }
    else
    {
      ostr << "# " << it->first
           << "=\n# Not set, no default\n#\n"
           << format_block( it->second.description_, "# ", 75 );
    }
  }
}

void 
config_block
::set_fail_on_missing_block( bool flag )
{
  fail_on_missing_block_ = flag;
}

vcl_map< vcl_string, config_block_value >
config_block
::enumerate_values() const
{
  return vmap_;
}

void
config_block
::add_component_to_check_list( const vcl_string& name )
{
  check_list_.push_back( name );
}


} // end namespace vidtk
