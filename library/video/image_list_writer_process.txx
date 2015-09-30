/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_list_writer_process.h"

#include <vcl_sstream.h>
#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{


template <class PixType>
image_list_writer_process<PixType>
::image_list_writer_process( vcl_string const& name )
  : process( name, "image_list_writer_process" ),
    step_count_( static_cast<unsigned int>(-1) ),
    img_( NULL )
{
  config_.add( "disabled", "false" );

  // If this is set, then it is not an error for the source image not
  // to be supplied.  Instead, the step() will behave as a no-op, not
  // incrementing counts, etc.
  config_.add( "skip_unset_images", "false" );
  
  config_.add( "image_list", "list.txt" );

  config_.add( "pattern", "out%2$04d.png" );
}


template <class PixType>
image_list_writer_process<PixType>
::~image_list_writer_process()
{
  list_out.close();
}


template <class PixType>
config_block
image_list_writer_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
image_list_writer_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", this->disabled_ );
    
    blk.get( "image_list", this->image_list_ );

    using namespace boost::io;

    vcl_string pat;
    if( ! blk.get( "pattern", pat ) && ! disabled_ )
    {
      log_error( this->name() << ": needs an output pattern\n" );
      return false;
    }
    pattern_ = boost::format( pat );
    pattern_.exceptions( all_error_bits & ~too_many_args_bit );

    blk.get( "skip_unset_images", this->skip_unset_images_ );
  }
  catch( unchecked_return_value& )
  {
    // restore previous state
    this->set_params( config_ );
    return false;
  }
  catch( boost::io::format_error& )
  {
    log_error( this->name() << ": couldn't parse pattern\n" );
    // restore previous state
    this->set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
image_list_writer_process<PixType>
::initialize()
{
  step_count_ = static_cast<unsigned int>(-1);
  img_ = NULL;

	if( !disabled_ )
	{
		list_out.open( image_list_.c_str() );
		if( ! list_out.is_open() )
		{
			vcl_cout << "Error opening the "<< image_list_ <<" file." << vcl_endl;
			return false;
		}
	}
  return true;
}


template <class PixType>
bool
image_list_writer_process<PixType>
::step()
{
  if( disabled_ )
  {
    return true;
  }

  if( img_ == NULL )
  {
    if( skip_unset_images_ )
    {
      return true;
    }
    else
    {
      log_error( name() << ": image not supplied\n" );
      return false;
    }
  }

  ++step_count_;

  if( ! ts_.has_frame_number() )
  {
    ts_.set_frame_number( step_count_ );
  }
  if( ! ts_.has_time() )
  {
    ts_.set_time( ts_.frame_number() );
  }

  vcl_ostringstream filename;
  filename << ( this->pattern_ % ts_.time() % ts_.frame_number() % step_count_ );

  vil_image_view< vxl_byte > tmp;

  vil_convert_stretch_range( *img_, tmp );

  bool okay = vil_save( tmp, filename.str().c_str() );
  
  list_out << filename.str() << vcl_endl;

  img_ = NULL;
  return okay;
}


template <class PixType>
void
image_list_writer_process<PixType>
::set_image( vil_image_view<PixType> const& img )
{
  img_ = &img;
}


template <class PixType>
void
image_list_writer_process<PixType>
::set_timestamp( timestamp const& ts )
{
  ts_ = ts;
}

template <class PixType>
bool 
image_list_writer_process<PixType>
::is_disabled() const
{
  return this->disabled_;
}

} // end namespace vidtk
