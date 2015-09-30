/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_string.h>
#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <tracking/vsl/track_io.h>
#include <tracking/vsl/image_object_io.h>
#include <utilities/vsl/timestamp_io.h>

#include "vsl_reader.h"

bool
vidtk::vsl_reader
::read( vcl_vector<vidtk::track_sptr> &tracks )
{
  if( !this->bin_ )
  {
    this->bin_ = new vsl_b_ifstream(this->filename_);
    if( !this->bin_->is() )
    {
      delete this->bin_;
      this->bin_ = NULL;
      return false;
    }

    vcl_string header;
    int version;

    vsl_b_read( *(this->bin_), header );
    vsl_b_read( *(this->bin_), version );
    if( !*(this->bin_) )
    {
      return false;
    }
  }

  tracks.clear();
  if( !this->bin_->is() ) { return false; }

  this->bin_->clear_serialisation_records();

  bool hasTracks;
  vsl_b_read( *(this->bin_), hasTracks );
  if( !this->bin_->is() ) { return false; }

  if( hasTracks )
  {
    vsl_b_read( *(this->bin_) , tracks );
    if( !this->bin_->is() ) { return false; }
  }

  bool hasObjs;
  vsl_b_read( *(this->bin_), hasObjs );
  if( !this->bin_->is() ) { return false; }

  if( hasObjs )
  {
    // Not really sure what to do with these yet
    vidtk::timestamp ts;
    vcl_vector<vidtk::image_object_sptr> objs;

    vsl_b_read( *(this->bin_), ts );
    vsl_b_read( *(this->bin_), objs );
    if( !this->bin_->is() ) { return false; }
  }

  return true;
}

