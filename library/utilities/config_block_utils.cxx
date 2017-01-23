/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "config_block_utils.h"

#include <vul/vul_file.h>
#include <boost/algorithm/string.hpp>
#include <logger/logger.h>

#include <fstream>


namespace vidtk {

VIDTK_LOGGER( " config_block" );


void print_to_file( config_block const& block, std::string const& file_name,
                    std::string const& prog, std::string const& githash )
{
  std::ofstream str( file_name.c_str() );
  if ( ! str )
  {
    LOG_ERROR( "Could not open file \"" << file_name << "\" for writing." );
    return;
  }

  std::string const file_ext = vul_file::extension( file_name );
  if ( boost::iequals( file_ext, ".md" ) )
  {
    block.print_as_markdown( str, prog );
  }
  else if ( boost::iequals( file_ext, ".cfg" ) )
  {
    block.print_as_tree( str, prog, githash );
  }
  else
  {
    // All other types get this format
    block.print( str );
  }
}

} // end namespace
