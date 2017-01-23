/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "string_reader_process.h"

#include "tcp_string_reader_impl.h"
#include "dir_string_reader_impl.h"
#include "file_string_reader_impl.h"

#include <map>

#include <logger/logger.h>
VIDTK_LOGGER("string_reader_process");

namespace vidtk {

typedef std::map< std::string, vidtk::string_reader_impl::string_reader_impl_base* > impl_list_type;
typedef impl_list_type::iterator impl_iter_type;
typedef impl_list_type::const_iterator impl_const_iter_type;

// ----------------------------------------------------------------
/** \brief Private class for string reader process.
 *
 *
 */
  class string_reader_process::priv
  {
  public:
    priv()
      : parent(0),
        reader_impl(0)
    { }

    ~priv()
    {
      for( impl_iter_type it = impl_list.begin(); it != impl_list.end(); ++it )
      {
        delete it->second;
      }
    }

    bool set_params( config_block const& blk )
    {
      if ( ! this->reader_impl )
      {
        LOG_ERROR( parent->name() << ": no implementation selected" );
        return false;
      }
      return this->reader_impl->set_params( blk );
    }

    bool initialize()
    {
      if ( ! this->reader_impl )
      {
        LOG_ERROR( parent->name() << ": no implementation selected" );
        return false;
      }

      return this->reader_impl->initialize();
    }


    bool step()
    {
      if ( ! this->reader_impl )
      {
        LOG_ERROR( parent->name() << ": no implementation selected" );
        return false;
      }

      return this->reader_impl->step();
    }


    // list of implementation options
    vidtk::process* parent;
    impl_list_type impl_list;

    // pointer to reader implementation
    vidtk::string_reader_impl::string_reader_impl_base* reader_impl;

  }; // end class string_reader_process::priv




// ----------------------------------------------------------------
string_reader_process
::string_reader_process( std::string const& n )
  : process( n, "string_reader_process" ),
    d( new string_reader_process::priv )
{
  d->parent = this;

  // make list of available readers.
  d->impl_list["file"] =  new vidtk::string_reader_impl::file_string_reader_impl( this->name() + "/file" );
  d->impl_list["tcp"] = new vidtk::string_reader_impl::tcp_string_reader_impl( this->name() + "/tcp" );
  d->impl_list["dir"] = new vidtk::string_reader_impl::dir_string_reader_impl( this->name() + "/dir" );
}


string_reader_process
::~string_reader_process( void )
{
}


// ----------------------------------------------------------------
config_block string_reader_process
::params() const
{
  config_block blk;

  blk.add_parameter( "type", "type of string reader. Valid options are \"file\", "
                     "\"tcp\", or \"dir\". "
                     "\"file\" reads from a file; \"tcp\" reads from a socket; "
    "\"dir\" collects all filenames from the directory.");

  // Add implementation configs as subblocks
  for( impl_const_iter_type it = d->impl_list.begin(); it != d->impl_list.end(); ++it )
  {
    blk.add_subblock( it->second->params(), it->first );
  }

  return blk;
}


// ----------------------------------------------------------------
bool string_reader_process
::set_params( config_block const& blk )
{
  try
  {
    std::string type = blk.get<std::string>( "type" );

    // Search for an implementation with a matching name.  If we find
    // it, select and configure that implementation, and exit early.
    impl_const_iter_type it = d->impl_list.find( type );
    if ( it != d->impl_list.end() )
    {
      d->reader_impl = const_cast< vidtk::string_reader_impl::string_reader_impl_base* >(it->second);
      return d->set_params( blk.subblock( type ) );
    }

    // We didn't find a implementation with that name.
    LOG_ERROR( name()<< ": no implementation named \"" << type << "\"" );
  }
  catch(config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
  }

  return false;
}


// ----------------------------------------------------------------
bool string_reader_process
::initialize()
{
  return d->initialize();
}


// ----------------------------------------------------------------
bool string_reader_process
::step()
{
  return d->step();
}

// ----------------------------------------------------------------
// -- OUTPUTS --
std::string
string_reader_process
::str(void) const
{
  return d->reader_impl->str_;
}

} // end namespace vidtk
