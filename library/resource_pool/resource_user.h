/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_RESOURCE_USER_H_
#define _VIDTK_RESOURCE_USER_H_

#include <set>
#include <string>
#include <boost/any.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/signals2/connection.hpp>
#include <resource_pool/resource_pool_exception.h>


namespace vidtk {

// ----------------------------------------------------------------
/*! \brief Trait for defining resource.
 *
 * All resources are identified by creating a trait which binds the
 * resource name to a type.
 *
 * Example:
 \code
 struct foo_model
 {
    int a;
 };

 // bind "iso_foo_model" to type foo_model
 resource_trait< foo_model > model_resource1( "iso_foo_model" );

 foo_model a_model;  // allocate resource object
 create_resource( model_resource1, a_model );  // create a shared resource
  \endcode
  *
  */
template < typename T >
struct resource_trait {
  resource_trait( std::string const& n ) : m_name( n ) { }
  typedef T type;

  std::string const& name() const { return m_name; }

private:
  std::string m_name;
};



class resource_pool;

// ----------------------------------------------------------------
/**
 * \brief Resource user base class.
 *
 * This is the base class for all processes that are going to use the
 * resource pool. Processes that are going to share resources must be
 * derived from this class. Think of it as a mixin.
 *
 * Design concepts:
 *
 * - Force derived class to create all resources early in process
 *   lifetime so they can be displayed on pipeline graph.
 *
 * - Make sure resources are created before they are used. This avoids
 *   race conditions.
 *
 * Processes that are creating or registering resources must create
 * them early in the process life cycle. In the CTOR would be a good
 * place.
 *
 * Processes that are using or consuming a resource must declare their
 * intention by calling use_resource() in the CTOR. The actual
 * resource can be retrieved using the get_resource() call. It is
 * important for the implementer to assure that all resources are
 * created before they are retrieved, so that a "resource not
 * available" return from the get_resource() really means that it is
 * not available, and not just "not there yet" which indicates a race
 * condition.
 *
 * Note that the method identified as the observer will be called from
 * another thread, so make sure all shared data areas are thread safe.
 */
class resource_user
{
public:
  // -- TYPES --
  typedef boost::signals2::signal< void( std::string const& ) >::slot_type  notify_slot_t;

  resource_user();
  virtual ~resource_user();


  /**
   * \brief Create new resource.
   *
   * This method creates a new resource with the specified value. \b
   * false is returned if the resource already exists.
   *
   * The value supplied to this call is \e copied to the shared
   * resource pool and is then available for all to use. The local
   * copy can be deleted at this point since it si no longer needed.
   *
   * If the application needs to quickly exchange or share some data,
   * a pointer can be the resource.
   *
   * Resources created by this method are \e owned by the process and are
   * deleted when the process is destroyed.
   *
   * The name of the resource is saved locally so it can be shown on
   * the pipeline diagram.
   *
   * @param name Name of resource
   * @param resource Resource value
   *
   * @return \b true if resource created; \b false if resource already exists
   */
  template < typename T >
  bool create_resource( resource_trait< T > const& trait, T const& val )
  {
    return this->create_i( trait.name(), val  );
  }


  /**
   * \brief Declare usage of resource.
   *
   * This method declares that this process is going to use the
   * specified resource. Using a resource implies that the process will
   * get the value of the resource.
   *
   * A resource must be identified using this method before any other
   * resource call can be made.
   *
   * The name of the resource is saved locally so it can be shown on
   * the pipeline diagram.
   *
   * @param trait Resource that will be used.
   */
  template < typename T >
  void use_resource( resource_trait< T > const& trait )
  {
    this->used_i( trait.name() );
  }


  /**
   * \brief Set value for resource.
   *
   * This method updates the value of a resource. The resource must
   * already exist.
   *
   * @param string Name of resource
   * @param resource Reference to resource data
   *
   * @return \b true if resource is present and value is updated;
   * \b false otherwise
   *
   * @throw resource_pool_exception if resource is not been specified
   * in a prior create_resource() or use_resource() call.
   */
  template< typename T >
  bool set_resource( resource_trait< T > const& trait, T const& resource )
  {
    return this->set_i( trait.name(), resource );
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
   * @throw resource_pool_exception if resource is not been specified
   * in a prior create_resource() or use_resource() call.
   * @throw resource_pool_exception if resource is not in pool
   */
  template< typename T >
  T get_resource( resource_trait< T > const& trait )
  {
    boost::any temp_val;
    if ( this->get_i( trait.name(), temp_val ) == false)
    {
      // Resource not present.
      std::string msg;
      msg = "Resource " + trait.name() + " not in pool";
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
   *
   * @throw resource_pool_exception if resource is not been specified
   * in a prior use_resource() call.
   * @throw resource_pool_exception if resource is not in pool
  */
  template< typename T >
  bool get_resource( resource_trait< T > const& trait, T& val )
  {
    boost::any temp_val;
    if ( this->get_i( trait.name(), temp_val ) == false)
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
   * @throw resource_pool_exception if resource is not been specified
   * in a use_resource() call.
   */
  template< typename T>
  bool resource_available( resource_trait< T > const& trait ) const
  {
    return this->has_resource_i( trait.name() );
  }


  /**
   * \brief Get list of owned resources.
   *
   * The names of all resources created by this process are
   * returned. This list is built from the names of resources passed
   * to the create_resource() call.
   *
   * @return List of resource names.
   */
  std::vector< std::string > get_owned_list();


 /**
  * \brief Get list of used resources.
  *
  * The names of all resources used by this process are returned. This
  * list is built from the names of resources passed to the use_resource() call.
  *
  * @return List of resource names being used.
  */
  std::vector< std::string > get_used_list();


  /**
   * \brief Monitor resource for changes.
   *
   * This method establishes a method to be called when the specified
   * resource changes.
   *
   * \note The resource must have been previously created for this
   * call to succeed.
   *
   * @param name Name of the resource to monitor
   * @param subscriber Method to be called when resource is updated
   *
   * @return \b true if observer has been established; \b false otherwise.
   *
   * Example:
   * \code
   // define resource trait
   resource_trait< foo_model > model_resource( "foo_model" );

   class foo_process : public process, public resource_user
   {
   public:
      foo_process()
      {
        use_resource( model_resource ); // declare intent to use resource
        // Observe changes in the model resource
        observe_resource( model_resource, boost::bind( &foo_process::notify, this, _1 ) );
      }

      void notify( std::string const& res_name)
      {
        if (res_name == model_resource.name() ) // doesn't hurt to check
        {
          // Get resource from pool
          foo_model model = get_resource( model_resource );
        }
      }
   }; // end class
   * \endcode
   */
  template< typename T>
  bool observe_resource( resource_trait< T > const& trait, notify_slot_t const& subscriber)
  {
    return this->observe_i( trait.name(), subscriber );
  }


  /**
   * \brief Notify all observers of resource change.
   *
   * Normally observers are notified when the resource value is set
   * via set_resource(). In those cases where the resource may be a
   * pointer to data, rather than the actual data, it may be useful to
   * manually signal that the data has changed.
   *
   * @param name Name of resource to notify
   */
  template< typename T>
  void notify_observers( resource_trait< T > const& trait )
  {
    this->notify_i( trait.name() );
  }


private:
  bool create_i( std::string const& name, boost::any const& val );
  bool get_i( std::string const& name, boost::any& val );
  bool set_i( std::string const& name, boost::any const& val );
  bool observe_i( std::string const& name, notify_slot_t const& subscriber );
  void notify_i( std::string const& name );
  void used_i( std::string const& name );
  bool has_resource_i( std::string const& name ) const;

  resource_pool* m_pool;

  std::set< std::string > m_owned_resources;
  std::set< std::string > m_used_resources;

  /// List of resources being observed
  std::vector < boost::signals2::connection > m_active_connections;

}; // end class resource_user

} // end namespace vidtk

#endif /* _VIDTK_RESOURCE_USER_H_ */
