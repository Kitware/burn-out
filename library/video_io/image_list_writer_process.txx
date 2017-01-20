/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_list_writer_process.h"

#include <vil/vil_convert.h>
#include <vil/vil_save.h>

#include <sstream>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_image_list_writer_process_txx__
VIDTK_LOGGER( "image_list_writer_process_txx" );


namespace vidtk
{


template <class PixType>
image_list_writer_process<PixType>
::image_list_writer_process( std::string const& _name )
  : process( _name, "image_list_writer_process" ),
    step_count_( static_cast<unsigned int>( -1 ) ),
    img_( NULL )
{
  config_.add_parameter( "disabled", "false",
                         "false: write output images; true: do nothing produce success." );

  config_.add_parameter( "skip_unset_images", "false",
                         "If this is set, then it is not an error for the source image not "
                         "to be supplied.  Instead, the step() will behave as a no-op, not"
                         "incrementing counts, etc." );

  config_.add_parameter( "force_8bit_images", "false",
                         "false: directly use vil_save() with the input image."
                         "true: apply *stretch_range* transform to 8-bit images before saving." );

  config_.add_parameter( "image_list", "",
                         "ASCII file with a list of filenames produced by the process." );

  config_.add_parameter( "pattern", "out%2$04d.png",
                         "Output filename pattern for the output images." );
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
    this->disabled_ = blk.get<bool>( "disabled" );

    if( !this->disabled_ )
    {
      this->image_list_ = blk.get<std::string>( "image_list" );

      this->force_8bit_images_ = blk.get<bool>( "force_8bit_images" );

      using namespace boost::io;

      std::string pat = blk.get<std::string>( "pattern" );
      pattern_ = boost::format( pat );
      pattern_.exceptions( all_error_bits & ~too_many_args_bit );

      this->skip_unset_images_ = blk.get<bool>( "skip_unset_images" );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }
  catch( boost::io::format_error& )
  {
    LOG_ERROR( this->name() << ": couldn't parse pattern" );
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

  if( !disabled_ && !image_list_.empty() )
  {
    list_out.open( image_list_.c_str() );
    if( ! list_out.is_open() )
    {
      LOG_ERROR("Error opening the "<< image_list_ <<" file." );
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

  if( img_ == NULL || !( *img_ ) || img_->size() == 0 )
  {
    if( skip_unset_images_ )
    {
      return true;
    }
    else
    {
      LOG_ERROR( name() << ": image not supplied" );
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

  std::ostringstream filename;
  filename << ( this->pattern_ % ts_.time() % ts_.frame_number() % step_count_ );
  bool okay = false;
  if(!this->force_8bit_images_ || vil_pixel_format_of(PixType()) == vil_pixel_format_of(vxl_byte()) )
  {
    okay = vil_save( *img_, filename.str().c_str() );
  }
  else
  {
    vil_image_view< vxl_byte > tmp;
    vil_convert_stretch_range( *img_, tmp );
    okay = vil_save( tmp, filename.str().c_str() );
  }

  if( !this->image_list_.empty() )
  {
    list_out << filename.str() << std::endl;
  }

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
