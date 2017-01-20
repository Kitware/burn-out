/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_reader_vsl.h"

#include <logger/logger.h>

#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <vul/vul_file.h>
#include <utilities/vsl/timestamp_io.h>
#include <tracking_data/vsl/image_object_io.h>

#include <string>


namespace vidtk  {
namespace ns_image_object_reader {

VIDTK_LOGGER ("image_object_reader_vsl");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
image_object_reader_vsl
::image_object_reader_vsl()
  : bfstr_(0),
    current_obj_it_(current_obj_list_.end())
{ }


image_object_reader_vsl
::~image_object_reader_vsl()
{
  delete bfstr_;
}


// ----------------------------------------------------------------
 bool
image_object_reader_vsl
 ::open(std::string const& filename)
{
  if (0 != this->bfstr_) // meaning we are already open
  {
    return true;
  }

  filename_ = filename;

  // test for expected file extension
  std::string const file_ext( filename_.substr(filename_.rfind(".")+1) );
  if (file_ext != "vsl")
  {
    return false;
  }

  bfstr_ = new vsl_b_ifstream( filename_ );
  if( ! *bfstr_ )
  {
    LOG_ERROR( "Couldn't open " << filename_ << " for reading." );

    delete bfstr_;
    bfstr_ = NULL;

    return false;
  }

  std::string header;
  vsl_b_read( *bfstr_, header );
  if( header != "image_object_vsl_stream" )
  {
    LOG_ERROR( "incorrect header. Are you sure this is an image object vsl stream?" );
    return false;
  }

  int version;
  vsl_b_read( *bfstr_, version );
  if( version != 1 )
  {
    LOG_ERROR( "unknown format version " << version);
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
/** Return next ts/image-obj pair.
 *
 * Return the next timestamp and image object pair. The file was
 * written with a time_stamp followed by a vector of image objects. We
 * have to undo this packing.
 *
 * Note that we are implicitly assuming that there will always be an
 * object in the image object vector.
 */
bool
image_object_reader_vsl
::read_next(ns_image_object_reader::datum_t& datum)
{
  if ( bfstr_ == NULL )
  {
    LOG_ERROR("Stream is null");
    return false;
  }

  if (! bfstr_->is().good() ) // reached end of file
  {
    return false;
  }

  if (this->current_obj_it_ == this->current_obj_list_.end())
  {
    bfstr_->clear_serialisation_records();
    this->current_obj_list_.clear();

    vsl_b_read( *bfstr_, this->current_ts_ );
    vsl_b_read( *bfstr_, this->current_obj_list_ );

    if (! bfstr_->is().good() ) // reached end of file
    {
      return false;
    }

    // reset iterator
    this->current_obj_it_ = this->current_obj_list_.begin();
  }

  datum.first = this->current_ts_;
  datum.second = *(this->current_obj_it_);
  ++this->current_obj_it_;

  return true;
}

} // end namespace
} // end namespace
