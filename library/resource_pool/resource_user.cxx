/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "resource_user.h"

#include <resource_pool/resource_pool.h>
#include <resource_pool/resource_pool_exception.h>

#include <boost/foreach.hpp>

#include <logger/logger.h>
VIDTK_LOGGER( "resource_user" );

namespace vidtk {

typedef boost::signals2::signal< void( std::string const& ) >    notify_signal_t;

resource_user::
resource_user()
  : m_pool( resource_pool::instance() )
{
}


// ----------------------------------------------------------------
resource_user::
~resource_user()
{
  // Disconnect all connections in list
  BOOST_FOREACH( boost::signals2::connection & c, this->m_active_connections)
  {
    c.disconnect();
  }

  this->m_active_connections.clear();

  // delete all owned resources
  BOOST_FOREACH( std::string name, this->m_owned_resources )
  {
    m_pool->delete_resource( name );
  }
}


// ----------------------------------------------------------------
bool
resource_user::
create_i( std::string const& name, boost::any const& val )
{
  m_owned_resources.insert( name );
  return m_pool->create_i( name, val );
}


// ----------------------------------------------------------------
void
resource_user::
used_i( std::string const& name )
{
  m_used_resources.insert( name );
}


// ----------------------------------------------------------------
bool
resource_user::
has_resource_i( std::string const& name ) const
{
  return m_pool->has_resource( name );
}


// ----------------------------------------------------------------
std::vector< std::string >
resource_user::
get_owned_list()
{
  std::vector< std::string > the_list;
  BOOST_FOREACH( std::string const& name, m_owned_resources)
  {
    the_list.push_back( name );
  }

  return the_list;
}


// ----------------------------------------------------------------
std::vector< std::string >
resource_user::
get_used_list()
{
  std::vector< std::string > the_list;
  BOOST_FOREACH( std::string const& name, m_used_resources)
  {
    the_list.push_back( name );
  }

  return the_list;
}


// ----------------------------------------------------------------
bool
resource_user::
get_i( std::string const& name, boost::any& val )
{
  if ( this->m_used_resources.count( name ) == 0 )
  {
    std::string msg;
    msg = "Resource \"" + name + "\" has not been used.";
    throw ns_resource_pool::resource_pool_exception( msg );
  }

  return m_pool->get_i( name, val );
}


// ----------------------------------------------------------------
bool
resource_user::
set_i( std::string const& name, boost::any const& val )
{
  // must be in one of the lists, else throw
  if ( this->m_owned_resources.count( name ) == 0
       && this->m_used_resources.count( name ) == 0 )
  {
    std::string msg;
    msg = "Resource \"" + name + "\" has not been created or used.";
    throw ns_resource_pool::resource_pool_exception( msg );
  }

  return m_pool->set_i( name, val );
}


// ----------------------------------------------------------------
void
resource_user::
notify_i( std::string const& name )
{
  // must be in one of the lists, else throw
  if ( this->m_owned_resources.count( name ) == 0
       && this->m_used_resources.count( name ) == 0 )
  {
    std::string msg;
    msg = "Resource \"" + name + "\" has not been created or used.";
    throw ns_resource_pool::resource_pool_exception( msg );
  }

  m_pool->notify_observers( name );
}


// ----------------------------------------------------------------
bool
resource_user::
observe_i( std::string const& name, resource_pool::notify_slot_t const& subscriber)
{
  // must be in one of the lists, else throw
  if ( this->m_owned_resources.count( name ) == 0
       && this->m_used_resources.count( name ) == 0 )
  {
    std::string msg;
    msg = "Resource \"" + name + "\" has not been created or used";
    throw ns_resource_pool::resource_pool_exception( msg );
  }

  try
  {
    // Add observer
    boost::signals2::connection c = m_pool->observe( name, subscriber );

    // Add to list of active connections
    m_active_connections.push_back( c );

    return true;
  }
  catch ( ns_resource_pool::resource_pool_exception const& e )
  {
    LOG_ERROR( "Could not establish observer - " << e.what() );
    return false;
  }
}

} // end namespace vidtk
