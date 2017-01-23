/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_io/generic_frame_process.h>

#include <logger/logger.h>
VIDTK_LOGGER("generic_frame_process_txx");

namespace vidtk
{

template<class PixType>
generic_frame_process<PixType>
::generic_frame_process( std::string const& _name )
  : frame_process<PixType>( _name, "generic_frame_process" ),
    impl_( NULL ),
    initialized_( false )
{
  populate_possible_implementations();
}


template<class PixType>
generic_frame_process<PixType>
::~generic_frame_process()
{
  for( impl_iter_type it = impls_.begin(); it != impls_.end(); ++it )
  {
    delete it->second;
  }
}


template<class PixType>
config_block
generic_frame_process<PixType>
::params() const
{
  config_block blk;

  blk.add_parameter( "disabled", "false",
                     "To disable or enable this reader process. "
                     "If this process is disabled, it produces no images." );
  blk.add_parameter( "type",
                     "Specifies the implementation type to use for reading images. "
                     "Available types depend on image pixel type required and "
                     "concrete reader types available." );
  blk.add_parameter( "read_only_first_frame", "false",
                     "Only read the first image and return that image for all subsequent cycles." );

  for( impl_const_iter_type it = impls_.begin(); it != impls_.end(); ++it )
  {
    blk.add_subblock( it->second->params(), it->first );
  }

  return blk;
}


template<class PixType>
bool
generic_frame_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );

    if( disabled_ )
    {
      return true;
    }

    std::string type = blk.get< std::string >( "type" );
    read_only_first_frame_ = blk.get< bool >( "read_only_first_frame" );

    // Search for an implementation with a matching name.  If we find
    // it, select and configure that implementation, and exit early.
    for( impl_const_iter_type it = impls_.begin(); it != impls_.end(); ++it )
    {
      if( it->first == type )
      {
        impl_ = it->second;
        impl_name_ = type;
        return impl_->set_params( blk.subblock( type ) );
      }
    }

    // We didn't find a implementation with that name.
    LOG_ERROR( "generic_frame_process: no implementation named \""
               << type << "\"" );
  }
  catch(config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
  }

  return false;
}


template<class PixType>
bool
generic_frame_process<PixType>
::initialize()
{
  if( !this->disabled_ && this->initialized_ == false )
  {
    LOG_ASSERT( impl_ != NULL, "no implementation selected" );

    frame_number_ = unsigned(-1);

    this->initialized_ = true;

    return impl_->initialize();
  }

  return true;
}

template<class PixType>
bool
generic_frame_process<PixType>
::reinitialize()
{
  this->initialized_ = false;
  return this->initialize();
}


template<class PixType>
process::step_status
generic_frame_process<PixType>
::step2()
{
  if( this->disabled_ )
  {
    return process::FAILURE;
  }

  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  if( read_only_first_frame_ )
  {
    if( frame_number_ == unsigned(-1) )
    {
      // Read frame from selected reader
      process::step_status impl_status = impl_->step2();
      if( impl_status != process::SUCCESS )
      {
        return impl_status;
      }
      if( impl_->timestamp().has_frame_number() )
      {
        frame_number_ = impl_->timestamp().frame_number();
      }
      else
      {
        frame_number_ = 0;
      }
    }
    else
    {
      ++frame_number_;
    }
    return process::SUCCESS;
  }
  else
  {
    return impl_->step2();
  }
}

template<class PixType>
bool
generic_frame_process<PixType>
::seek( unsigned frame_number )
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  return impl_->seek( frame_number );
}


template<class PixType>
unsigned
generic_frame_process<PixType>
::nframes() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  return impl_->nframes();
}


template<class PixType>
double
generic_frame_process<PixType>
::frame_rate() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  return impl_->frame_rate();
}


template <class PixType>
timestamp
generic_frame_process<PixType>
::timestamp() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  if( read_only_first_frame_ )
  {
    vidtk::timestamp ts;
    ts.set_frame_number( frame_number_ );
    return ts;
  }
  else
  {
    return impl_->timestamp();
  }
}


template<class PixType>
vil_image_view<PixType>
generic_frame_process<PixType>
::image() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  return impl_->image();
}


template<class PixType>
vil_image_view<PixType>
generic_frame_process<PixType>
::copied_image() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  copy_img_ = vil_image_view< PixType >();
  copy_img_.deep_copy( impl_->image() );

  return copy_img_;
}

template<class PixType>
video_metadata
generic_frame_process<PixType>
::metadata() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  ///@todo Add frame process capabilities methods to support "has
  /// metadata" and "has_seek", and others. The current approach of
  /// checking a list of names will not scale well.
  if (impl_name_ != "image_metadata_list"
      && impl_name_ != "tcp_frame_metadata"
      && impl_name_ != "vidl_ffmpeg_metadata"
      && impl_name_ != "vidl_ffmpeg" )
  {
    //Warn that there probably is not metadata.  The tracker can work without metadata.
    LOG_WARN(this->name() << " implementation does not supply metadata: " << impl_name_);
  }
  video_metadata const& retval = impl_->metadata();
  return retval;
}


template <class PixType>
unsigned
generic_frame_process<PixType>
::ni() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  return impl_->ni();
}


template <class PixType>
unsigned
generic_frame_process<PixType>
::nj() const
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );

  return impl_->nj();
}

template <class PixType>
bool
generic_frame_process<PixType>
::set_roi(std::string & roi)
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );
  return impl_->set_roi(roi);
}

template <class PixType>
bool
generic_frame_process<PixType>
::set_roi( unsigned int x, unsigned int y,
              unsigned int w, unsigned int h )
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );
  return impl_->set_roi(x,y,w,h);
}

template <class PixType>
void
generic_frame_process<PixType>
::reset_roi()
{
  LOG_ASSERT( impl_ != NULL, "no implementation selected" );
  impl_->turn_off_roi();
}


} // end namespace vidtk
