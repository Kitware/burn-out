/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_CONFIG_BLOCK_UTILS_H_
#define _VIDTK_CONFIG_BLOCK_UTILS_H_

#include <utilities/config_block.h>

namespace vidtk {

/**
 * @brief Print config block to a file.
 *
 * Format the specified config block to the file. The file extension
 * determines the format used to display the config.
 * - .md - format as markdown
 * - .cfg - format as config tree
 * - <other> - print config in flat format.
 *
 * @param block Config block to format
 * @param file_name Name of file to write the config to
 * @param prog Top level program name (optional)
 * @param githash Associated git hash (optional)
 */
void print_to_file( config_block const& block, std::string const& file_name,
                    std::string const& prog = "",
                    std::string const& githash = "N/A");

} // end namespace

#endif /* _VIDTK_CONFIG_BLOCK_UTILS_H_ */
