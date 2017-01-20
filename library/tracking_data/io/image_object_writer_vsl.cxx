/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "image_object_writer_vsl.h"

#include <logger/logger.h>

#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <vul/vul_file.h>
#include <utilities/vsl/timestamp_io.h>
#include <tracking_data/vsl/image_object_io.h>


namespace vidtk  {

VIDTK_LOGGER ("image_object_writer_vsl");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */

image_object_writer_vsl
::image_object_writer_vsl()
  : bfstr_(0)
{ }

image_object_writer_vsl
::~image_object_writer_vsl()
{
  delete bfstr_;
}


// ----------------------------------------------------------------
 bool
image_object_writer_vsl
::initialize(config_block const& blk)
{
  // initialize the base class
  if (image_object_writer::initialize(blk) == false)
  {
    return false;
  }

  // Force .vsl extension
  std::string fn_only = vul_file::strip_directory( filename_ );
  // Just in case the path includes a '.', strip_extension() is being supplied
  // fn_only
  std::string fn = vul_file::dirname( filename_ ) + "/" +
    vul_file::strip_extension(fn_only) + ".vsl";

  delete bfstr_;
  bfstr_ = new vsl_b_ofstream( fn );
  if( ! *bfstr_ )
  {
    LOG_ERROR( "Couldn't open " << fn << " for writing" );
    delete bfstr_;
    bfstr_ = NULL;
    return false;
  }

  vsl_b_write( *bfstr_, std::string("image_object_vsl_stream") );
  vsl_b_write( *bfstr_, 1 ); // version number

  return true;
}


// ----------------------------------------------------------------
void
image_object_writer_vsl
::write(timestamp const& ts, std::vector< image_object_sptr > const& objs )
{
  if( bfstr_ == NULL )
  {
    LOG_ERROR("Stream is NULL" );
    return;
  }
  // each step() should result in an independent set of objects.
  bfstr_->clear_serialisation_records();

  vsl_b_write( *bfstr_, ts );
  vsl_b_write( *bfstr_, objs );
}

} // end namespace
