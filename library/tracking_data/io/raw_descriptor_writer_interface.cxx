/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "raw_descriptor_writer_interface.h"

#include <boost/foreach.hpp>

#include <logger/logger.h>


namespace vidtk {
namespace ns_raw_descriptor_writer {


VIDTK_LOGGER ("raw_descriptor_writer_interface");


raw_descriptor_writer_interface::
raw_descriptor_writer_interface()
  : output_stream_( 0 )
{
}


raw_descriptor_writer_interface::
~raw_descriptor_writer_interface()
{
  output_stream_ = 0; // reset stream pointer
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer_interface::
write( raw_descriptor::vector_t const& desc_list )
{
  BOOST_FOREACH( descriptor_sptr_t desc, desc_list )
  {
    if ( ! write( desc ) )
    {
      return false;
    }
  }// end foreach

  return true;
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer_interface::
initialize( std::ostream* str )
{
  // Store supplied stream pointer
  this->output_stream_ = str;

  // Call derived class initializer
  return initialize();
}


// ----------------------------------------------------------------
std::ostream&
raw_descriptor_writer_interface::
output_stream()
{
  if ( 0 == this->output_stream_ )
  {
    LOG_ERROR( "Output stream is not active" );
    return std::cerr; // just to return something
  }

  return *(this->output_stream_);
}


// ----------------------------------------------------------------
bool
raw_descriptor_writer_interface::
is_good() const
{
  return output_stream_->good();
}

} // end namespace raw_descriptor_writer
} // end namespace vidtk
