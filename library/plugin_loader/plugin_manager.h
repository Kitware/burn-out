/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_PLUGIN_MANAGER_H_
#define VIDTK_PLUGIN_MANAGER_H_

#include <vector>
#include <string>
#include <map>
#include <ostream>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace vidtk {

// base class of factory hierarchy
class plugin_factory;

typedef boost::shared_ptr< plugin_factory >       plugin_factory_handle_t;
typedef std::vector< plugin_factory_handle_t >    plugin_factory_vector_t;
typedef std::map< std::string, plugin_factory_vector_t > plugin_map_t;

class plugin_manager_impl;

// ----------------------------------------------------------------
/**
 * @brief Manages a set of plugin (singleton)
 *
 * The plugin manager keeps track of all factories from plugins that
 * are discovered on the disk. The list of directories that are
 * searched is specified by the VIDTK_MODULE_PATH environment variable.
 *
 *
 */
class plugin_manager
{
public:
  static plugin_manager* instance();
  ~plugin_manager();

  /**
   * @brief Get list of factories for interface type.
   *
   * This method returns a list of pointer to factory methods that
   * create objects of the desired interface type.
   *
   * @param type_name Type name of the interface required
   *
   * @return Vector of factories. (vector may be empty)
   */
  plugin_factory_vector_t const& get_factories( std::string const& type_name ) const;

  /**
   * @brief Add factory to manager.
   *
   * This method adds the specified plugin factory to the plugin
   * manager. This method is usually called from the plugin
   * registration function in the loadable module to self-register all
   * plugins in a module.
   *
   * Plugin factory objects are grouped under the interface type name,
   * so all factories that create the same interface are together.
   *
   * @param fact Plugin factory object to register
   *
   * @return A pointer is returned to the added factory in case
   * attributes need to be added to the factory.
   *
   * Example:
   \code
   void register_factories( plugin_manager* pm )
   {
     //                 interface type                concrete type
     plugin_factory_handle_t fact = pm->ADD_FACTORY( vidtk::ns_track_reader::track_reader_interface, vidtk::ns_track_reader::track_reader_nit );
     fact.add_attribute( "file-type", "xml mit" );
   }
   \endcode
   */
  plugin_factory_handle_t add_factory( plugin_factory* fact );

  /**
   * @brief Get default path.
   *
   * This method returns the default mudule search path.
   *
   * @return Module path.
   */
  std::string get_default_path() const;

  /**
   * @brief Get map of known plugins.
   *
   * Get the map of all known registered plugins.
   *
   * @return Map of plugins
   */
  plugin_map_t const& get_plugin_map() const;

  /**
   * @brief Get list of files loaded.
   *
   * This method returns the list of shared object file names that
   * successfully re loaded.
   *
   * @return List of file names.
   */
  std::vector< std::string > get_file_list() const;

private:
  plugin_manager();

  boost::scoped_ptr< plugin_manager_impl > m_impl;
  static plugin_manager* s_instance;

}; // end class plugin_manager

} // end namespace

#endif /* VIDTK_PLUGIN_MANAGER_H_ */
