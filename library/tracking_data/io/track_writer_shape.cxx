/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_writer_shape.h"

#include <tracking_data/io/shape_file_writer.h>

#include <vul/vul_sprintf.h>
#include <vul/vul_file.h>

#include <logger/logger.h>

using namespace boost::posix_time;

namespace vidtk
{

VIDTK_LOGGER ("shape_writer");

track_writer_shape
::track_writer_shape( bool apix_way, std::string dir, std::string pre )
 : directory_(dir), prefix_(pre), apix_way_(apix_way)
{
}

track_writer_shape
::~track_writer_shape()
{
}

void
track_writer_shape
::set_options(track_writer_options const& options)
{
  directory_ = options.output_dir_;
}

bool
track_writer_shape
::open( std::string const& /*fname*/ )
{
  vul_file::make_directory_path( directory_ );
  if(!is_good())
  {
    LOG_ERROR("Output directory does not exists for shape files");
    return false;
  }
  return true;
}

bool
track_writer_shape
::write( std::vector<vidtk::track_sptr> const& tracks )
{
  for( std::vector<vidtk::track_sptr>::const_iterator iter = tracks.begin(); iter != tracks.end(); ++iter )
  {
    shape_file_writer shape_writer(apix_way_);
    std::string track_id = vul_sprintf( "%i", (*iter)->id() );
    std::string fname = directory_ + "/" + prefix_ +track_id;
    shape_writer.open(fname);
    if(!shape_writer.write(*iter))
    {
      LOG_ERROR("Could not write track" );
      shape_writer.close();
      return false;
    }
    shape_writer.close();
  }
  return true;
}


bool
track_writer_shape
::is_open() const
{
  return this->is_good();
}

bool
track_writer_shape
::is_good() const
{
  return vul_file::is_directory(directory_);
}

}//namespace vidtk
