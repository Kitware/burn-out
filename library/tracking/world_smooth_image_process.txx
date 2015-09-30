/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/world_smooth_image_process.h>
#include <utilities/log.h>
#include <video/gauss_filter.h>
#include <vil/algo/vil_gauss_filter.h>
#include <vcl_limits.h>

#include <vcl_cassert.h>

namespace vidtk
{

template < class PixType >
world_smooth_image_process<PixType>
::world_smooth_image_process( vcl_string const& name )
  : process( name, "world_smooth_image_process" ),
    src_img_( NULL ),
    world_std_dev_( -1 ),
    max_image_std_dev_( -1 ),
    world_half_width_( -1 ),
    world_units_per_pixel_( 1 ),
    max_image_half_width_( 3 ),
    use_integer_based_smoothing_( false )
{
  config_.add_parameter( "std_dev", "-1",
    "Standard deviation of the Gaussian kernel (in world units)."
    " No smoothing will be performed by default." );
  config_.add_parameter( "half_width", "-1",
    "Radius of the template used in convolution. Will be automatically"
    " computed from std_dev if not specified." );
  config_.add_parameter( "world_units_per_pixel", "1",
    "The scale of the image, e.g. meters-per-pixel. Will get over-"
    "written if the port is connected." );
  config_.add_parameter( "max_image_std_dev", "-1",
    "An upper-bound on the Gaussian kernel std. deviation (in pixels)."
    " This will cap the value derived from std_dev parameter that is in"
    " world coordinates. NOTE: This parameter is deprecated by "
    " max_image_half_width and should be used intead." );
  config_.add_parameter( "max_image_half_width", "4",
    "An upper-bound on the kernel radius (in pixels). This is for "
    "computational efficiency. This parameter is the used after all "
    "others and will over-ride the effect of the GSD and other "
    "parameters." );
  config_.add_parameter( "use_integer_based_smoothing", "false",
    "Use an integer-based kernel for image smoothing" );
}

template < class PixType >
config_block
world_smooth_image_process<PixType>
::params() const
{
  return config_;
}


template < class PixType >
bool
world_smooth_image_process<PixType>
::set_params( config_block const& blk )
{
  blk.get( "std_dev", world_std_dev_ );
  blk.get( "half_width", world_half_width_ );
  blk.get( "max_image_std_dev", max_image_std_dev_ );
  blk.get( "max_image_half_width", max_image_half_width_ );
  blk.get( "world_units_per_pixel", world_units_per_pixel_ );
  blk.get( "use_integer_based_smoothing", use_integer_based_smoothing_ );

  config_.update( blk );

  return true;
}


template < class PixType >
bool
world_smooth_image_process<PixType>
::initialize()
{
  return true;
}

template < class PixType >
bool
world_smooth_image_process<PixType>
::step()
{
  assert( src_img_ != NULL );

  double image_std_dev = world_std_dev_ / world_units_per_pixel_;

  if( max_image_std_dev_ > 0 )
  {
    image_std_dev = vcl_min( max_image_std_dev_, image_std_dev );
  }
  unsigned image_half_width;

  if( world_half_width_ < 0 )
    image_half_width = unsigned(vcl_ceil(3*image_std_dev));
  else
    image_half_width = unsigned(world_half_width_ / world_units_per_pixel_);

  if( image_std_dev > 0 )
  {
    if( image_half_width > max_image_half_width_ )
    {
      log_warning( this->name() << ": Forcing the use of "<< max_image_half_width_
        << " kernel half width (pixels) instead of "<< image_half_width << "\n" );
        image_half_width = max_image_half_width_;
    }
    if ( use_integer_based_smoothing_ )
    {
      bool ok = gauss_filter_2d_int(*src_img_, out_img_, image_std_dev, image_half_width);
      if ( !ok )
      {
        log_warning( this->name() << "Can't use integer kernel for smoothing.  Falling back to double kernel.\n");
        vil_gauss_filter_2d(*src_img_, out_img_, image_std_dev, image_half_width);
      }
    }
    else
    {
      vil_gauss_filter_2d(*src_img_, out_img_, image_std_dev, image_half_width);
    }
  }
  else
  {
    out_img_ = *src_img_;
  }

  return true;
}


template < class PixType >
void
world_smooth_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  src_img_ = &img;
}


template < class PixType >
vil_image_view<PixType> const&
world_smooth_image_process<PixType>
::output_image() const
{
  return out_img_;
}

template < class PixType >
vil_image_view<PixType> const&
world_smooth_image_process<PixType>
::copied_image() const
{
  copied_out_img_ = vil_image_view< PixType >();
  copied_out_img_.deep_copy( out_img_ );
  return copied_out_img_;
}

template < class PixType >
void
world_smooth_image_process<PixType>
::set_world_units_per_pixel( double units_per_pix )
{
  world_units_per_pixel_ = units_per_pix;
}


} // end namespace vidtk
