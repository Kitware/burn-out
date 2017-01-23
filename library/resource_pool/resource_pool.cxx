/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "resource_pool.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

namespace vidtk {
namespace ns_resource_pool { // local namespace

typedef boost::signals2::signal< void( std::string const& ) >    notify_signal_t;

// ----------------------------------------------------------------
/*! \brief Resource manager data.
 *
 * could be managed via smart pointers
 */
class resource_data
{
public:
  resource_data()
    : m_observers( new ns_resource_pool::notify_signal_t )
  {
  }

  ~resource_data() { }

  // Add observer to this resource
  boost::signals2::connection observe( resource_pool::notify_slot_t const& subscriber ) const
  {
    return (*this->m_observers).connect( subscriber );
  }


  void notify()
  {
    (*this->m_observers)( this->m_name );
  }


  std::string m_name; // name of resource should match trait
  boost::any m_resource_data;

  boost::shared_ptr< ns_resource_pool::notify_signal_t>  m_observers; ///< list of observers
}; // end class

} // end namespace ns_resource_pool

typedef std::map< std::string, ns_resource_pool::resource_data > resource_pool_t;
typedef resource_pool_t::iterator resource_iterator_t;


// ================================================================
  class resource_pool::resource_pool_impl
{
public:
  resource_pool_impl()
  { }

  ~resource_pool_impl()
  { }

  resource_pool_t m_resource_pool;
  boost::mutex m_lock;

}; // end class resource_pool_impl



// ================================================================
//
// Static data for singleton
//
  resource_pool* resource_pool::s_instance = 0;



// ----------------------------------------------------------------
resource_pool*
resource_pool::
instance()
{
  static boost::mutex instance_lock;

  if ( 0 == s_instance )
  {
    boost::lock_guard< boost::mutex > lock( instance_lock );

    if ( 0 == s_instance )
    {
      s_instance = new resource_pool();
    }
  }

  return s_instance;
}


// ----------------------------------------------------------------
resource_pool::
resource_pool()
  : m_impl( new resource_pool::resource_pool_impl )
{
}


// ----------------------------------------------------------------
bool
resource_pool::
has_resource( std::string const& name ) const
{
  boost::lock_guard< boost::mutex > lock( m_impl->m_lock );

  if ( 0 == m_impl->m_resource_pool.count( name ) )
  {
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
bool
resource_pool::
delete_resource( std::string const& name )
{
  boost::lock_guard< boost::mutex > lock( m_impl->m_lock );

  resource_iterator_t it = m_impl->m_resource_pool.find( name );

  if ( it == m_impl->m_resource_pool.end() )
  {
    // Resource not present. return error.
    return false;
  }

  m_impl->m_resource_pool.erase( name );
  return true;
}


// ----------------------------------------------------------------
bool
resource_pool::
create_i( std::string const& name, boost::any const& val )
{
  boost::lock_guard< boost::mutex > lock( m_impl->m_lock );

  resource_iterator_t it = m_impl->m_resource_pool.find( name );

  if ( it != m_impl->m_resource_pool.end() )
  {
    // Resource already present. Return error.
    return false;
  }

  // Create new resource entry.
  m_impl->m_resource_pool[name].m_name = name;
  m_impl->m_resource_pool[name].m_resource_data = val;
  return true;
}


// ----------------------------------------------------------------
bool
resource_pool::
get_i( std::string const& name, boost::any& val )
{
  boost::lock_guard< boost::mutex > lock( m_impl->m_lock );

  resource_iterator_t it = m_impl->m_resource_pool.find( name );

  if ( it == m_impl->m_resource_pool.end() )
  {
    // Resource not present. Return error.
    return false;
  }

  val = it->second.m_resource_data; // return value
  return true;
}


// ----------------------------------------------------------------
bool
resource_pool::
set_i( std::string const& name, boost::any const& val )
{
  resource_iterator_t it;
  {
    boost::lock_guard< boost::mutex > lock( m_impl->m_lock );

    it = m_impl->m_resource_pool.find( name );

    if ( it == m_impl->m_resource_pool.end() )
    {
      // Resource not present. return error.
      return false;
    }

    // Update resource data
    it->second.m_resource_data = val;
  } // release lock on resources

  it->second.notify();
  return true;
}


// ----------------------------------------------------------------
boost::signals2::connection
resource_pool::
observe( std::string const& name, resource_pool::notify_slot_t const& subscriber )
{
  boost::lock_guard< boost::mutex > lock( m_impl->m_lock );

  resource_iterator_t it = m_impl->m_resource_pool.find( name );

  if ( it == m_impl->m_resource_pool.end() )
  {
    std::string msg;
    msg = "Resource " + name + " not in pool";
    throw ns_resource_pool::resource_pool_exception( msg );
  }

  // Add observer
  return it->second.observe( subscriber );
}


// ----------------------------------------------------------------
void
resource_pool::
notify_observers( std::string const& name )
{
  resource_iterator_t it;
  {
    boost::lock_guard< boost::mutex > lock( m_impl->m_lock );

    it = m_impl->m_resource_pool.find( name );

    if ( it == m_impl->m_resource_pool.end() )
    {
      std::string msg;
      msg = "Resource " + name + " not in pool";
      throw ns_resource_pool::resource_pool_exception( msg );
    }
  } // release lock on resources

  // Notify observers
  return it->second.notify();
}


// ----------------------------------------------------------------
std::vector< std::string >
resource_pool::
get_resource_list() const
{
  std::vector< std::string > names;
  BOOST_FOREACH( resource_pool_t::value_type val, m_impl->m_resource_pool)
  {
    names.push_back( val.first );
  }

  return names;
}

} // end namespace vidtk
