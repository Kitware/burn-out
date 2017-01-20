/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_PLUGIN_CONFIG_UTIL_H_
#define _VIDTK_PLUGIN_CONFIG_UTIL_H_

#include <string>


namespace vidtk {

  class plugin_factory;
  class config_block;

  /**
   * @brief Add config entry to plugin factory attributes
   *
   * This function sets the specified attribute that is to be later added
   * to the config block.
   *
   * @param fact Apply attribute to this foctory
   * @param name Config parameter name
   * @param description Config parameter description
   */
  void add_config_attribute( plugin_factory& fact, std::string const& name, std::string const& description );

  /**
   * @brief Add config entry to plugin factory attributes
   *
   * This function sets the specified attribute that is to be later added
   * to the config block.
   *
   * @param fact Apply attribute to this foctory
   * @param name Config parameter name
   * @param default_value Config parameter default value
   * @param description Config parameter description
   */
  void add_config_attribute( plugin_factory&    fact,
                             std::string const& name,
                             std::string const& default_value,
                             std::string const& description );

  /**
   * @brief Collect config entries from attributes and add to config block.
   *
   * This function applies all config attributes from the factory to
   * the config block. The config entry block level is taken directly
   * from the attribute key string with no other magic transforms. You
   * will have to craft that part of the entry carefully.
   *
   * @param[in] fact Plugin factory that has the attributes
   * @param[in,out] block Config block to update
   */
  void collect_plugin_config( plugin_factory& fact, config_block& block );
}

#endif /* _VIDTK_PLUGIN_CONFIG_UTIL_H_ */
