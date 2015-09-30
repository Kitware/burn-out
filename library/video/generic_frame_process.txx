/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video/generic_frame_process.h>

#include <utilities/log.h>

namespace vidtk
{


template<class PixType>
generic_frame_process<PixType>
::generic_frame_process( vcl_string const& name )
  : frame_process<PixType>( name, "generic_frame_process" ),
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

  blk.add( "type" );
  blk.add( "read_only_first_frame", "false" );

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
  vcl_string type;
  if( ! blk.get( "type", type ) )
  {
    log_error( "generic_frame_process: need an implementation type\n" );
    return false;
  }

  if( ! blk.get( "read_only_first_frame", read_only_first_frame_ ) )
  {
    return false;
  }

  // Search for an implementation with a matching name.  If we find
  // it, select and configure that implementation, and exit early.
  //
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
  log_error( "generic_frame_process: no implementation named \""
             << type << "\"\n" );
  return false;
}


template<class PixType>
bool
generic_frame_process<PixType>
::initialize()
{
  if( this->initialized_ == false )
  {
    log_assert( impl_ != NULL, "no implementation selected" );

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
bool
generic_frame_process<PixType>
::step()
{
  log_assert( impl_ != NULL, "no implementation selected" );

  if( read_only_first_frame_ )
  {
    if( frame_number_ == unsigned(-1) )
    {
      if( ! impl_->step() )
      {
        return false;
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
    return true;
  }
  else
  {
    return impl_->step();
  }
}

template<class PixType>
bool
generic_frame_process<PixType>
::seek( unsigned frame_number )
{
  log_assert( impl_ != NULL, "no implementation selected" );

  return impl_->seek( frame_number );
}


template <class PixType>
timestamp
generic_frame_process<PixType>
::timestamp() const
{
  log_assert( impl_ != NULL, "no implementation selected" );

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
vil_image_view<PixType> const&
generic_frame_process<PixType>
::image() const
{
  log_assert( impl_ != NULL, "no implementation selected" );

  return impl_->image();
}


template<class PixType>
vil_image_view<PixType> const&
generic_frame_process<PixType>
::copied_image() const
{
  log_assert( impl_ != NULL, "no implementation selected" );

  copy_img_ = vil_image_view< PixType >();
  copy_img_.deep_copy( impl_->image() );

  return copy_img_;
}

template<class PixType>
video_metadata const&
generic_frame_process<PixType>
::metadata() const
{
  log_assert( impl_ != NULL, "no implementation selected" );

  if (impl_name_ != "image_metadata_list")
  {
    log_warning("implementation does not supply metadata");
  }

  return impl_->metadata();
}


template <class PixType>
unsigned
generic_frame_process<PixType>
::ni() const
{
  log_assert( impl_ != NULL, "no implementation selected" );

  return impl_->ni();
}


template <class PixType>
unsigned
generic_frame_process<PixType>
::nj() const
{
  log_assert( impl_ != NULL, "no implementation selected" );

  return impl_->nj();
}

template <class PixType>
bool
generic_frame_process<PixType>
::set_roi(vcl_string roi)
{
  log_assert( impl_ != NULL, "no implementation selected" );
  return impl_->set_roi(roi);
}

template <class PixType>
bool
generic_frame_process<PixType>
::set_roi( unsigned int x, unsigned int y,
              unsigned int w, unsigned int h )
{
  log_assert( impl_ != NULL, "no implementation selected" );
  return impl_->set_roi(x,y,w,h);
}

template <class PixType>
void
generic_frame_process<PixType>
::reset_roi()
{
  log_assert( impl_ != NULL, "no implementation selected" );
  impl_->turn_off_roi();
}

// Declare the specializations that are defined in the .cxx file
#define DECL( T )                              \
  template<> void generic_frame_process<bool>  \
    ::populate_possible_implementations()
DECL( bool );
DECL( vxl_byte );
DECL( vxl_uint_16 );
#undef DECL

} // end namespace vidtk
