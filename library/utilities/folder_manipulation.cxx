/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "folder_manipulation.h"

#include <boost/filesystem.hpp>

namespace vidtk
{


bool does_folder_exist( const std::string& location )
{
  return boost::filesystem::exists( location ) &&
         boost::filesystem::is_directory( location );
}


bool list_all_subfolders( const std::string& location,
                          std::vector< std::string >& subfolders )
{
  subfolders.clear();

  if( !does_folder_exist( location ) )
  {
    return false;
  }

  boost::filesystem::path dir( location );

  for( boost::filesystem::directory_iterator dir_iter( dir );
       dir_iter != boost::filesystem::directory_iterator();
       ++dir_iter )
  {
    if( boost::filesystem::is_directory( *dir_iter ) )
    {
      subfolders.push_back( dir_iter->path().string() );
    }
  }

  return true;
}


bool list_files_in_folder( const std::string& location,
                           std::vector< std::string >& filepaths )
{
  filepaths.clear();

  if( !does_folder_exist( location ) )
  {
    return false;
  }

  boost::filesystem::path dir( location );

  for( boost::filesystem::directory_iterator file_iter( dir );
       file_iter != boost::filesystem::directory_iterator();
       ++file_iter )
  {
    if( boost::filesystem::is_regular_file( *file_iter ) )
    {
      filepaths.push_back( file_iter->path().string() );
    }
  }

  return true;
}


bool create_folder( const std::string& location )
{
  boost::filesystem::path dir( location );

  return boost::filesystem::create_directories( dir );
}


} // namespace vidtk
