/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "high_pass_filter_process.h"

#include <video_transforms/high_pass_filter.h>

#include <vxl_config.h>
#include <vil/vil_plane.h>

#include <utilities/point_view_to_region.h>

#include <logger/logger.h>


namespace vidtk
{


VIDTK_LOGGER( "high_pass_filter_process" );


template <typename PixType>
high_pass_filter_process<PixType>
::high_pass_filter_process( std::string const& _name )
  : process( _name, "high_pass_filter_process" ),
    source_image_(NULL),
    src_border_(NULL),
    gsd_(-1.0),
    mode_(BOX),
    box_kernel_width_(7),
    box_kernel_height_(7),
    box_interlaced_(false),
    box_kernel_dim_world_(30.0),
    min_kernel_dim_pixels_(5),
    max_kernel_dim_pixels_(201),
    last_valid_gsd_(0.30),
    output_net_only_(false)
{
  config_.add_parameter(
    "mode",
    "BOX",
    "Type of filtering to perform. Acceptable values: DISABLED, BOX, BIDIR or "
    "BOX_WORLD. See class documentation for more information." );

  config_.add_parameter(
    "box_kernel_width",
    "7",
    "Width of the box filter." );

  config_.add_parameter(
    "box_kernel_height",
    "7",
    "Height of the box filter." );

  config_.add_parameter(
    "box_interlaced",
    "false",
    "Treat input frames as interlaced, if set to true when using a box filter." );

  config_.add_parameter(
    "box_kernel_dim_world",
    "6.5",
    "Height and width of the box filter in world units if enabled." );

  config_.add_parameter(
    "min_kernel_dim_pixels",
    "7",
    "Minimum kernel height and width when in world mode." );

  config_.add_parameter(
    "max_kernel_dim_pixels",
    "85",
    "Maximum kernel height and width when in world mode." );

  config_.add_parameter(
    "output_net_only",
    "false",
    "If set to false, the output image will contain multiple planes, each representing "
    "the modal filter applied at different orientations, as opposed to a single plane "
    "image representing the sum of filters applied in all directions." );

  config_.add_parameter(
    "border_ignore_factor",
    "1",
    "Number of pixels to ignore near the border during operation." );
}


template <typename PixType>
high_pass_filter_process<PixType>
::~high_pass_filter_process()
{
}

template <typename PixType>
config_block
high_pass_filter_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
high_pass_filter_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    std::string type_string;
    type_string = blk.get<std::string>( "mode" );
    output_net_only_ = blk.get<bool>( "output_net_only" );

    if( type_string == "DISABLED" )
    {
      mode_ = DISABLED;
    }
    else if( type_string == "BOX" )
    {
      mode_ = BOX;
    }
    else if( type_string == "BIDIR" )
    {
      mode_ = BIDIR;
    }
    else if( type_string == "BOX_WORLD" )
    {
      mode_ = BOX_WORLD;
    }
    else
    {
      throw config_block_parse_error("Invalid mode specified");
    }

    if( mode_ == BOX || mode_ == BIDIR )
    {
      box_kernel_width_ = blk.get<unsigned>("box_kernel_width");
      box_kernel_height_ = blk.get<unsigned>("box_kernel_height");
      box_interlaced_ = blk.get<bool>("box_interlaced");
    }
    else if( mode_ == BOX_WORLD )
    {
      box_kernel_dim_world_ = blk.get<double>("box_kernel_dim_world");
      min_kernel_dim_pixels_ = blk.get<unsigned>("min_kernel_dim_pixels");
      max_kernel_dim_pixels_ = blk.get<unsigned>("max_kernel_dim_pixels");
      box_interlaced_ = blk.get<bool>("box_interlaced");

      if( !( min_kernel_dim_pixels_ & 0x01 ) )
      {
        min_kernel_dim_pixels_ = min_kernel_dim_pixels_ | 0x01;
        LOG_WARN( "Min kernel dimension must be odd, forcing." );
      }

      if( !( max_kernel_dim_pixels_ & 0x01 ) )
      {
        max_kernel_dim_pixels_ = max_kernel_dim_pixels_ | 0x01;
        LOG_WARN( "Max kernel dimension must be odd, forcing." );
      }
    }

    threads_.reset( new thread_sys_t( 1, 1 ) );
    threads_->set_function( boost::bind( &self_type::process_region, this, _1, _2 ) );
    threads_->set_border_mode( true, blk.get<int>("border_ignore_factor") );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "  << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}

template <typename PixType>
bool
high_pass_filter_process<PixType>
::initialize()
{
  return true;
}

template <typename PixType>
bool
high_pass_filter_process<PixType>
::step()
{
  if( mode_ == DISABLED )
  {
    return true;
  }

  if( !source_image_ )
  {
    LOG_ERROR( this->name() << ": Invalid input provided." );
    return false;
  }

  output_image_ = image_t();
  output_image_.set_size( source_image_->ni(), source_image_->nj(), 3 );

  threads_->apply_function( *src_border_, *source_image_, output_image_ );

  if( output_net_only_ && output_image_.nplanes() > 1 )
  {
    output_image_ = vil_plane( output_image_, output_image_.nplanes()-1 );
  }

  source_image_ = NULL;
  src_border_ = NULL;
  return true;
}

template <typename PixType>
void
high_pass_filter_process<PixType>
::process_region( const image_t& input_region, image_t output_region )
{
  if( mode_ == BOX )
  {
    box_high_pass_filter( input_region,
                          output_region,
                          box_kernel_width_,
                          box_kernel_height_,
                          box_interlaced_ );
  }
  else if( mode_ == BIDIR )
  {
    bidirection_box_filter( input_region,
                            output_region,
                            box_kernel_width_,
                            box_kernel_height_,
                            box_interlaced_ );
  }
  else if( mode_ == BOX_WORLD )
  {
    if( gsd_ > 0.0 )
    {
      last_valid_gsd_ = gsd_;
    }
    else
    {
      LOG_WARN( "Invalid GSD received" );
    }

    unsigned box_kernel_dim = static_cast<unsigned>( box_kernel_dim_world_ / last_valid_gsd_ );

    if( box_kernel_dim < min_kernel_dim_pixels_ )
    {
      box_kernel_dim = min_kernel_dim_pixels_;
    }
    if( box_kernel_dim > max_kernel_dim_pixels_ )
    {
      box_kernel_dim = max_kernel_dim_pixels_;
    }

    // Ensure that the kernel size is odd
    box_kernel_dim = box_kernel_dim | 0x01;

    box_high_pass_filter( input_region,
                          output_region,
                          box_kernel_dim,
                          box_kernel_dim,
                          box_interlaced_ );
  }
}

// Input/Output Accessors
template <typename PixType>
void
high_pass_filter_process<PixType>
::set_source_image( image_t const& img )
{
  source_image_ = &img;
}

template <typename PixType>
void
high_pass_filter_process<PixType>
::set_source_border( image_border const& border )
{
  src_border_ = &border;
}

template <typename PixType>
void
high_pass_filter_process<PixType>
::set_source_gsd( double gsd )
{
  gsd_ = gsd;
}

template <typename PixType>
vil_image_view<PixType>
high_pass_filter_process<PixType>
::filtered_image() const
{
  return output_image_;
}

} // end namespace vidtk
