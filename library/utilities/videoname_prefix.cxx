/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "videoname_prefix.h"

#include <utilities/config_block.h>
#include <logger/logger.h>

#include <vul/vul_file.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

namespace vidtk {

// singleton instance pointer.
videoname_prefix * videoname_prefix::s_singleton_ptr(0);

VIDTK_LOGGER("videoname_prefix");

// ----------------------------------------------------------------
videoname_prefix *
videoname_prefix::
instance()
{
  static boost::mutex instance_lock;

  if ( 0 == s_singleton_ptr )
  {
    boost::lock_guard< boost::mutex > lock( instance_lock );

    if ( 0 == s_singleton_ptr )
    {
      s_singleton_ptr = new videoname_prefix();
    }
  }

  return s_singleton_ptr;
}


videoname_prefix::
videoname_prefix()
{
}


videoname_prefix::
~videoname_prefix()
{
}


// ----------------------------------------------------------------
void
videoname_prefix::
set_videoname_prefix( std::string const& filename )
{
  // extract video base name from full input path and file specification.
  m_video_basename = vul_file::strip_directory(
    vul_file::strip_extension( filename ) );
}


// ----------------------------------------------------------------
bool
videoname_prefix::
add_videoname_prefix( config_block& block, std::string const& key )
{
  if( block.has( key ) )
  {
    std::string const& oldName = block.get<std::string>( key );
    if ( oldName.empty() || m_video_basename.empty() )
    {
      return true;
    }

    std::string newName = vul_file::dirname(oldName) + "/"
      + this->m_video_basename + "_" + vul_file::strip_directory(oldName);
    block.set( key, newName );

    return true;
  }
  else
  {
    LOG_INFO( "Config entry " << key << " not found." );
    return false;
  }
} // bool add_videoname_prefix()


// ----------------------------------------------------------------
std::string const&
videoname_prefix::
get_videoname_prefix() const
{
  return this->m_video_basename;
}

} // end namespace vidtk
