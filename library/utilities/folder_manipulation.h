/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_utilities_folder_manipulation_
#define vidtk_utilities_folder_manipulation_

#include <vector>
#include <string>


namespace vidtk
{


/** \brief Does a folder at the given location exist?
 *
 *  \param location Path to folder
 *  \return true on success, false otherwise
 */
bool does_folder_exist( const std::string& location );


/** \brief List all subdirectories in the given root folder.
 *
 *  \param location Path to directory
 *  \param subfolders Output subfolders
 *  \return true if root folder exists, false otherwise
 */
bool list_all_subfolders( const std::string& location,
                          std::vector< std::string >& subfolders );


/** \brief List all files in the given root folder.
 *
 *  \param location Path to directory
 *  \param subfolders Output subfolders
 *  \return true if root folder exists, false otherwise
 */
bool list_files_in_folder( const std::string& location,
                           std::vector< std::string >& filepaths );


/** \brief Create a new directory in the given location.
 *
 *  \param location Path to folder to create
 *  \return true on success, false otherwise
 */
bool create_folder( const std::string& location );


} // namespace vidtk

#endif // vidtk_utilities_folder_manipulation
