/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "warp_image_process.h"

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_warp_image_process_txx__
VIDTK_LOGGER( "warp_image_process_txx" );


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
::warp_image_process( std::string const& _name )
  : process( _name, "warp_image_process" )
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

  config_.add_parameter( "interp_type",
    "linear",
    "Type of interpolation used for the warping operation. Can be nearest, "
    "linear, or cubic." );

  config_.add_parameter( "unmapped_value",
    "0.0",
    "Unmapped value to use to fill pixels in the output image which have no "
    "corresponding pixels in the input." );
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
    only_first_ = blk.get<bool>( "only_first_image" );
    disabled_ = blk.get<bool>( "disabled" );
    reset_internal_buffer_ = blk.get<bool>( "reset_internal_buffer" );

    std::string interp_type = blk.get<std::string>( "interp_type" );

    if( interp_type == "linear" )
    {
      wip_.set_interpolator( warp_image_parameters::LINEAR );
    }
    else if( interp_type == "nearest" )
    {
      wip_.set_interpolator( warp_image_parameters::NEAREST );
    }
    else if( interp_type == "cubic" )
    {
      wip_.set_interpolator( warp_image_parameters::CUBIC );
    }
    else
    {
      throw config_block_parse_error( "Invalid interp_type = " + interp_type );
    }

    wip_.set_unmapped_value( blk.get<double>( "unmapped_value" ) );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
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
  image_size_set_ = false;
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
    LOG_INFO( this->name() << ": Stopping to warp, because the process is "
      "configured to warp only the first image in the pipeline." );
    return false;
  }

  // Check to make sure image view points to valid contents
  if( img_.top_left_ptr() == 0 )
  {
    LOG_ERROR( this->name() << ": Invalid Image Entered." );
    return false;
  }

  // Reset the internal buffer (more efficient in async mode)
  if( reset_internal_buffer_ )
  {
    out_img_ = vil_image_view<PixType>();
  }

  // Set output image to the same size as the input
  if(image_size_set_)
  {
    if( img_.planestep() == 1 )
    {
      out_img_ = vil_image_view<PixType>(width_,height_, 1, img_.nplanes());
    }
    else
    {
      out_img_ = vil_image_view<PixType>(width_,height_, img_.nplanes());
    }
    image_size_set_ = false;
  }
  else
  {
    set_to_size( out_img_, img_ );
  }

  // Perform actual image warping
  //LOG_DEBUG( "before warp image size: " << out_img_.ni() << ' ' << out_img_.nj() << ' ' << ((out_img_.is_contiguous())?"true":"false") );
  //LOG_DEBUG( "in image size: " << img_.ni() << ' ' << img_.nj() << ' ' << ((img_.is_contiguous())?"true":"false") );
  warp_image( img_, out_img_, homog_, wip_ );
  //LOG_DEBUG( "warp image size: " << out_img_.ni() << ' ' << out_img_.nj() );

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
::set_output_image_size( width_height_vector const & in)
{
  image_size_set_ = true;
  width_ = in[0];
  height_= in[1];
  LOG_DEBUG("Image size: " << width_ << " " << height_ );
}


template <class PixType>
void
warp_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  img_ = img;
}


template <class PixType>
vil_image_view<PixType>
warp_image_process<PixType>
::warped_image() const
{
  return out_img_;
}


} // end namespace vidtk
