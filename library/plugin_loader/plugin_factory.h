/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef VIDTK_PLUGIN_FACTORY_H_
#define VIDTK_PLUGIN_FACTORY_H_

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <stdexcept>
#include <typeinfo>

#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace vidtk {

class plugin_manager;
class plugin_factory;

typedef boost::shared_ptr< plugin_factory >       plugin_factory_handle_t;
typedef std::vector< plugin_factory_handle_t >    plugin_factory_vector_t;

// ==================================================================
/**
 * @brief Abstract base class for plugin factory.
 *
 */
class plugin_factory
  : public boost::enable_shared_from_this< plugin_factory >,
    private boost::noncopyable
{
public:
  virtual ~plugin_factory();

  // standard set of attributes
  static const std::string INTERFACE_TYPE;
  static const std::string CONCRETE_TYPE;
  static const std::string PLUGIN_FILE_NAME;
  static const std::string PLUGIN_NAME;
  static const std::string PLUGIN_DESCRIPTION;
  static const std::string CONFIG_HELP;

  /**
   * @brief Get attribute from factory
   *
   * @param[in] attr Attribute code
   * @param[out] val Value of attribute if present
   *
   * @return \b true if attribute is found; \b false otherwise.
   */
  bool get_attribute( std::string const& attr, std::string& val ) const;

  /**
   * @brief Add attribute to factory
   *
   * This method sets the specified attribute
   *
   * @param attr Attribute name.
   * @param val Attribute value.
   */
  plugin_factory& add_attribute( std::string const& attr, std::string const& val );

  /**
   * @brief Returns object of registered type.
   *
   * This method returns an object of the template type if
   * possible. The type of the requested object must match the
   * interface type for this factory. If not, an exception is thrown.
   *
   * @return Object of registered type.
   * @throws std::runtime_error
   */
  template <class T>
  T* create_object()
  {
    // See if the type requested is the type we support.
    if ( typeid( T ).name() != m_interface_type )
    {
      std::stringstream str;
      str << "Can not create object of requested type: " <<  typeid( T ).name()
          <<"  Factory created objects of type: " << m_interface_type;
      throw std::runtime_error( str.str() );
    }

    // Call derived class to create concrete type object
    T* new_object = reinterpret_cast< T* >( create_object_i() );
    if ( 0 == new_object )
    {
      std::stringstream str;

      str << "class_loader:: Unable to create object";
      throw std::runtime_error( str.str() );
    }

    return new_object;
  }

  template < class T > void for_each_attr( T& f )
  {
    BOOST_FOREACH( attribute_map_t::value_type val, m_attribute_map )
    {
      f( val.first, val.second );
    }
  }

  template < class T > void for_each_attr( T const& f ) const
  {
    BOOST_FOREACH( attribute_map_t::value_type val, m_attribute_map )
    {
      f( val.first, val.second );
    }
  }


protected:
  plugin_factory( std::string const& itype);

  std::string m_interface_type;


private:
  // Method to create concrete object
  virtual void* create_object_i() = 0;

  typedef std::map< std::string, std::string > attribute_map_t;
  attribute_map_t m_attribute_map;
};


// ----------------------------------------------------------------
/**
 * @brief Factory for concrete class objects.
 *
 * @tparam T Type of the concrete class created.
 */
template< class T >
class plugin_factory_0
  : public plugin_factory
{
public:
  /**
   * @brief Create concrete factory object
   *
   * @param itype Name of the interface type
   */
  plugin_factory_0( std::string const& itype )
    : plugin_factory( itype )
  {
    // Set concrete type of factory
    this->add_attribute( CONCRETE_TYPE, typeid( T ).name() );
  }

  virtual ~plugin_factory_0() {}

protected:
  virtual void* create_object_i()
  {
    void * new_obj = new T();
    return new_obj;
  }
};

} // end namespace


// ==================================================================
// Support for adding factories

#define ADD_FACTORY( interface_T, conc_T)                               \
  add_factory( new vidtk::plugin_factory_0< conc_T >( typeid( interface_T ).name() ) )

#endif /* VIDTK_PLUGIN_FACTORY_H_ */
