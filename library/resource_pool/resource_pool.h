/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_RESOURCE_POOL_H_
#define _VIDTK_RESOURCE_POOL_H_

#include <resource_pool/resource_pool_exception.h>

#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/any.hpp>

/*
use cases-

1) create and register resource

2) become an observer of a resource

3) update resource value and notify observers

4) delete resource. Could have special notify for deleting
   resource. Could have resource lifetime manager that will delete
   resource when managing object is deleted.

5) process created a set of resources

6) process no longer wants to observe

7) polled mode resource tracking

8) Event driven resource change notification

9) display contents of resource pool

-------
Issues:

- Must be able to document resource providers and consumers on
  pipeline graph. Use triangle and invtriangle for resource usage. Use
  more colours too. aquamarine

  -- options --

  - (*) use dynamic cast in process to see if it inherits from
    resource_user. If so, call get_owned_list() and get_used_list();

  - change process base class. Add virtual method to return resource
    usage. Returns false if not derived from resource user, returns
    true and resource lists otherwise.

  - use std::is_base_of<B,D> - experimental in C++11, problematic

- Should there be a separate signal for deleting a resource.

  The resource_observer class could provide a virtual method that is
  called when the observed resource is deleted. Classes that are
  derived from that can optionally override that method to get
  notification.

- can absorb video_name support. keep same API, use resource_pool as
  impl

*/

namespace vidtk {

// ----------------------------------------------------------------
/**
 * \brief Singleton resource pool
 *
 * This class represents a pool of resources that are shared within an
 * application.
 *
 * The resource pool is thread safe with regard to the data stored in
 * the pool. If the data in the pool is really a pointer to some
 * shared data area, the users will have to manage any synchronization
 * issues themselves.
 *
 * The resource_user class is layered on top of the raw resource
 * manager to provide a more process friendly interface. This class is
 * not designed to be used directly by an application.
 */
class resource_pool
  : boost::noncopyable
{
public:
  // -- TYPES --
  typedef boost::signals2::signal< void( std::string const& ) >::slot_type  notify_slot_t;

  static resource_pool* instance();
  ~resource_pool();


  /**
   * \brief Create new resource.
   *
   * This method creates a new resource with the specified value. \b
   * false is returned if the resource already exists.
   *
   * @param name Name of resource
   * @param resource Resource value
   *
   * @return \b true if resource created; \b false if resource already exists
   */
  template< typename T>
  bool create_resource( std::string const& name, T const& resource )
  {
    return this->create_i( name, resource );
  }


  /**
   * \brief Set value for resource.
   *
   * This method updates the value of a resource. The resource must
   * already exist. All observers are notified.
   *
   * @param string Name of resource
   * @param resource Reference to resource data
   *
   * @return \b true if resource is present and value is updated;
   * \b false otherwise
   */
  template< typename T>
  bool set_resource( std::string const& name, T const& resource )
  {
    return this->set_i( name, resource );
  }


  /**
   * \brief Get resource value.
   *
   * This method returns the value of the named resource. If the
   * requested resource is not present an exception is thrown.
   *
   * @param name Name of resource
   *
   * @return resource value
   *
   * @throw resource_pool_exception if resource is not in pool
   */
  template< typename T>
  T get_resource( std::string const& name )
  {
    boost::any temp_val;
    if ( this->get_i( name, temp_val ) == false)
    {
      // Resource not present.
      std::string msg;
      msg = "Resource \"" + name + "\" not in pool";
      throw ns_resource_pool::resource_pool_exception( msg );
    }

    return boost::any_cast< T >( temp_val );
  }


  /**
   * \brief Get resource value.
   *
   * This method returns the value of the named resource. If the
   * requested resource is not present an exception is thrown.
   *
   * @param[in] name Name of resource
   * @param[out] val Value of resource is returned through this parameter
   *
   * @return \b true if resource is present; \b false otherwise.
   */
  template< typename T>
  bool get_resource( std::string const& name, T& val )
  {
    boost::any temp_val;
    if ( this->get_i( name, temp_val ) == false)
    {
      return false;
    }

    val = boost::any_cast< T >( temp_val );
    return true;
  }


  /**
   * \brief Determine if resource is in the pool.
   *
   * This method checks to see if the specified resource is in the
   * pool.
   *
   * @param name Name of resource
   *
   * @return \b true if resource is present; \b false otherwise.
   */
  bool has_resource( std::string const& name ) const;


  /**
   * \brief Delete resource from pool.
   *
   * The names resource is removed from the pool. If there are any
   * observers of this resource, they are all silently disconnected.
   *
   * @param name Name of resource
   *
   * @return \b true if resource is removed; \b false resource not present in pool.
   */
  bool delete_resource( std::string const& name );


  /**
   * \brief Observe a specific resource
   *
   * The specified observer method is added to the list of observers. \b
   * false is returned if the resource is not present or there are other
   * problems adding the observer.
   *
   * @param name Name of resource to observe
   * @param subscriber Reference to method to call when
   * resource is updated
   *
   * @return Boost Connection object so connection can be managed
   * externally.
   *
   * @throw resource_pool_exception if resource is not in pool
   *
   * Example:
   * \code
   class foo
   {
   public:
      foo()
      {
        // Observe changes in the model resource
        resource_pool::instance()->observe( model_resource, boost::bind( &foo:notify, this, _1 ) );
      }

      void notify( std::string const& res_name)
      {
        // Get resource from pool
        foo_model model = get_resource( res_name );
      }
   }; // end class
   * \endcode
   */
  boost::signals2::connection
  observe( std::string const& name, notify_slot_t const& subscriber);


  /**
   * \brief Notify all observers of resource change.
   *
   * Send notification to all observers of an object.
   *
   * Normally observers are notified when the resource value is set
   * via set_resource(). In those cases where the resource may be a
   * pointer to data, rather than the actual data, it may be useful to
   * manually signal that the data has changed.
   *
   * @param name Name of resource to notify
   */
  void notify_observers( std::string const& name );


  /**
   * \brief Get list of resource names.
   *
   * This method returns the list of currently available resources.
   *
   * @return List of resource names.
   */
  std::vector< std::string > get_resource_list() const;


private:
  friend class resource_user; // mon amie

  // Bridge to internal implementation
  bool create_i( std::string const& name, boost::any const& val );
  bool get_i( std::string const& name, boost::any& val );
  bool set_i( std::string const& name, boost::any const& val );


  resource_pool(); // private CTOR for singleton

  static resource_pool* s_instance; // singleton instance

  // Internal implementation
  class resource_pool_impl;
  boost::scoped_ptr< resource_pool_impl > m_impl;

}; // end class resource_pool

} // end namespace vidtk

#endif /* _VIDTK_RESOURCE_POOL_H_ */
