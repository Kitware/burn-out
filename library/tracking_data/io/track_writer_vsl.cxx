/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_writer_vsl.h"

#include <tracking_data/tracking_keys.h>

#include <logger/logger.h>

#include <utilities/geo_lat_lon.h>

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <tracking_data/vsl/track_io.h>
#include <tracking_data/vsl/image_object_io.h>
#include <utilities/vsl/timestamp_io.h>

namespace vidtk
{

VIDTK_LOGGER ("vsl_writer");

track_writer_vsl
::track_writer_vsl(  )
: bfstr_(NULL)
{
}

track_writer_vsl
::~track_writer_vsl()
{
  this->close();
}

bool
track_writer_vsl
::write( std::vector<vidtk::track_sptr> const& tracks,
         std::vector<vidtk::image_object_sptr> const* ios,
         timestamp const& ts )
{
  if(!bfstr_)
  {
    return false;
  }
  // each step() should result in an independent set of objects.
  bfstr_->clear_serialisation_records();

  vsl_b_write( *bfstr_, true );
  vsl_b_write( *bfstr_, tracks );

  if( ios )
  {
    vsl_b_write( *bfstr_, true );
    vsl_b_write( *bfstr_, ts );
    vsl_b_write( *bfstr_, *ios );
  }
  else
  {
    vsl_b_write( *bfstr_, false );
  }
  bfstr_->os().flush();
  return bfstr_->os().good();
}

bool
track_writer_vsl
::open( std::string const& fname )
{
  if(bfstr_)
  {
    delete bfstr_;
  }
  bfstr_ = new vsl_b_ofstream( fname );
  if( ! *bfstr_ )
  {
    LOG_ERROR( "Couldn't open "
    << fname << " for writing." );
    delete bfstr_;
    bfstr_ = NULL;
    return false;
  }

  vsl_b_write( *bfstr_, std::string("track_vsl_stream") );
  vsl_b_write( *bfstr_, 2 ); // version number
  bfstr_->os().flush();
  if(!this->is_good())
  {
    LOG_ERROR("Could not open " << fname);
    return false;
  }
  return true;
}

bool
track_writer_vsl
::is_open() const
{
  return this->is_good();
}

void
track_writer_vsl
::close()
{
  if(bfstr_)
  {
    bfstr_->os().flush();
    bfstr_->close();
    delete bfstr_;
    bfstr_ = NULL;
  }
}

bool
track_writer_vsl
::is_good() const
{
  return bfstr_ && bfstr_->os().good();
}

}//namespace vidtk
