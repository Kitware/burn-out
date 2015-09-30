/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "warp_image_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <video/warp_image.h>

namespace // anonymous ns to hold helper function
{

/// \internal
/// Set B to the same size as A while trying to match the memory layout.
template<class T>
void
set_to_size( vil_image_view<T>& B, vil_image_view<T> const& A )
{
  if( B.ni() != A.ni() ||
      B.nj() != A.nj() ||
      B.nplanes() != A.nplanes() )
  {
    if( A.planestep() == 1 )
    {
      B = vil_image_view<T>( A.ni(), A.nj(), 1, A.nplanes() );
    }
    else
    {
      B = vil_image_view<T>( A.ni(), A.nj(), A.nplanes() );
    }
  }
}

}

namespace vidtk
{


template <class PixType>
warp_image_process<PixType>
::warp_image_process( vcl_string const& name )
  : process( name, "warp_image_process" )
{
  config_.add_parameter( "only_first_image", 
    "false", 
    "If set to true, this process will return false after warping the first "
    "image or step in the pipeline. Otherwise, will continue to warp each "
    "image or step one at a time until the end of the parent node fails." );

  config_.add_parameter( "disabled", 
    "false", 
    "Whether to perform warping on the input image or not?" );

  config_.add_parameter( "reset_internal_buffer",
    "true",
    "Will reset and reallocate internal output buffer after each step." );
}


template <class PixType>
warp_image_process<PixType>
::~warp_image_process()
{
}


template <class PixType>
config_block
warp_image_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
warp_image_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "only_first_image", only_first_ );
    blk.get( "disabled", disabled_ );
    blk.get( "reset_internal_buffer", reset_internal_buffer_ );
  }
  catch( unchecked_return_value& )
  {
    // reset to old values
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
warp_image_process<PixType>
::initialize()
{
  frame_counter_ = 0;
  out_img_.clear();
  return true;
}

template <class PixType>
bool
warp_image_process<PixType>
::reset()
{
  return this->initialize();
}


template <class PixType>
bool
warp_image_process<PixType>
::step()
{
  // If process is disabled exit
  if( disabled_ )
  {
    out_img_ = img_;
    return true;
  }

  // If we only want to process the first frame and are on the second
  if( only_first_ && frame_counter_++ > 0 )
  {
    log_info( this->name() << ": Stopping to warp, because the process is "
      "configured to warp only the first image in the pipeline.\n" );
    return false;
  }

  // Check to make sure image view points to valid contents
  if( img_.top_left_ptr() == 0 )
  {
    log_error( this->name() << ": Invalid Image Entered.\n" );
    return false;
  }

  // Reset the internal buffer (more efficient in async mode)
  if( reset_internal_buffer_ )
  {
    out_img_ = vil_image_view<PixType>();
  }

  // Set output image to the same size as the input
  set_to_size( out_img_, img_ );

  // Perform actual image warping
  warp_image( img_, out_img_, homog_ );

  return true;
}


template <class PixType>
void
warp_image_process<PixType>
::set_destination_to_source_homography( vgl_h_matrix_2d<double> const& h )
{
  homog_ = h;
}


template <class PixType>
void
warp_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  img_ = img;
}


template <class PixType>
vil_image_view<PixType> const&
warp_image_process<PixType>
::warped_image() const
{
  return out_img_;
}


} // end namespace vidtk
