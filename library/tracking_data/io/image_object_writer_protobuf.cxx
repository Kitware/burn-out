/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_writer_protobuf.h"

#include <protobuf/protobuf_container.h>
#include <protobuf/protobuf_message_container.h>
#include <protobuf/protobuf_composite_container.h>

#include <protobuf/convert_protobuf.h>

#include <protobuf/ImageObject.pb.h>
#include <protobuf/Timestamp.pb.h>

#include <logger/logger.h>
#include <vul/vul_file.h>

#include <vector>
#include <string>

namespace vidtk
{

VIDTK_LOGGER ("image_object_writer_protobuf");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
image_object_writer_protobuf
::image_object_writer_protobuf()
  : fstr_ (0)
{ }


image_object_writer_protobuf
::~image_object_writer_protobuf()
{
  delete fstr_;
}


// ----------------------------------------------------------------
bool
image_object_writer_protobuf
::initialize(config_block const& blk)
{
  // initialize the base class
  if (image_object_writer::initialize(blk) == false)
  {
    return false;
  }

  // Force .kwpc extension (for Kitware protobuf container
  std::string fn_only = vul_file::strip_directory( filename_ );
  // Just in case the path includes a '.', strip_extension() is being supplied
  // fn_only
  std::string fn = vul_file::dirname( filename_ ) + "/" +
    vul_file::strip_extension(fn_only) + ".kwpc";

  delete fstr_;
  fstr_ = new std::ofstream( fn.c_str() );
  if( ! *fstr_ )
  {
    LOG_ERROR( "Couldn't open " << fn << " for writing." );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
void
image_object_writer_protobuf
::write( timestamp const& ts, std::vector< image_object_sptr > const& objs )
{
  if( fstr_ == NULL )
  {
    LOG_ERROR( "Stream is NULL" );
    return;
  }

  // Due to the general semantics of image object files, we do not
  // write entries that do not have any objects.
  if ( objs.empty() )
  {
    return ;
  }

  // create top level composite container
  composite_message_sptr slice = protobuf_container::create_composite_container();
  slice->set_name("image_object_writer");

  // Convert timestamp into protobuf format
  proto::VidtkTypeUnion* tu = new  proto::VidtkTypeUnion();
  convert_protobuf( ts, *tu);

  // Add timestamp protobuf to container
  message_container_sptr tainer = protobuf_container::create_message_container( tu );
  slice->add_container( tainer );

  // convert image object vector into protobuf format
  tu = new  proto::VidtkTypeUnion();
  convert_protobuf( objs, *tu );

  // Add vector to container
  tainer = protobuf_container::create_message_container( tu );
  slice->add_container( tainer );

  // Serialize container to stream
  slice->serialize_to_stream(*fstr_);

  fstr_->flush();
}

} // end namespace
