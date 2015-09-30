/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "gen_world_units_per_pixel_process.h"

#include <utilities/log.h>
#include <utilities/compute_gsd.h>

#include <vnl/vnl_math.h>
#include <vgl/vgl_box_2d.h>

namespace vidtk
{


template <class PixType>
gen_world_units_per_pixel_process<PixType>
::gen_world_units_per_pixel_process( vcl_string const& name )
  : process( name, "gen_world_units_per_pixel_process" ),
    img_( NULL ),
    world_units_per_pixel_( 1 ),
    disabled_ ( false ),
    auto_size_extraction_( false )
{
  config_.add( "tl_x", "1" );
  config_.add( "tl_y", "1" );
  config_.add( "br_x", "760" );
  config_.add( "br_y", "480" );
  config_.add( "disabled", "true" );
  config_.add_parameter( "auto_size_extraction", "true",
    "If true, uses the size of the input image and homography to "
    "compute the GSD" );
  //TODO:config_.add_optional( "filename", "Get GSD from the MAAS system in a file." );
}


template <class PixType>
config_block
gen_world_units_per_pixel_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
gen_world_units_per_pixel_process<PixType>
::set_params( config_block const& blk )
{
  tl_xy.first = blk.get<unsigned>( "tl_x" );
  tl_xy.second = blk.get<unsigned>( "tl_y" );

  br_xy.first = blk.get<unsigned>( "br_x" );
  br_xy.second = blk.get<unsigned>( "br_y" );

  disabled_ = blk.get<bool>( "disabled" );

  auto_size_extraction_ = blk.get<bool>( "auto_size_extraction" );

  config_.update( blk );

  return true;
}


template <class PixType>
bool
gen_world_units_per_pixel_process<PixType>
::initialize()
{
  //TODO:Read in the file if that is the srouce of GSD. 

  return true;
}



template <class PixType>
bool
gen_world_units_per_pixel_process<PixType>
::step()
{
  if( disabled_ ) 
  {
    // Using 1, the processes will ignore the GSD and 
    // effectively use the parameters in image coordinates. 

    world_units_per_pixel_ = 1;
    return true;
  }

  if( im2wld_H_ == NULL )
  {
    log_info( this->name() << " : image-to-world homography not provided" 
                           << vcl_endl );
    return false;
  }

  if( auto_size_extraction_ )
  {
    if( img_ != NULL )
    {
      tl_xy.first = 1;
      tl_xy.second = 1;

      br_xy.first = img_->ni();
      br_xy.second = img_->nj();
    }
    else
    {
      vcl_cout<< name() << ": Couldn't read the input image." 
              << vcl_endl;
      return false;
    }
  }

  vgl_box_2d<unsigned> img_box( tl_xy.first, 
                                br_xy.first, 
                                tl_xy.second,
                                br_xy.second );

  world_units_per_pixel_ = compute_gsd( img_box,
                                        im2wld_H_->get_matrix() );
 
  log_info( this->name() << ": Computed GSD = " << world_units_per_pixel_ << "\n" );

  img_ = NULL;
  im2wld_H_ = NULL;

  return true;
}

template <class PixType>
double 
gen_world_units_per_pixel_process<PixType>
::world_units_per_pixel( ) const
{
  return world_units_per_pixel_;
}


template <class PixType>
void
gen_world_units_per_pixel_process<PixType>
::set_source_homography( vgl_h_matrix_2d<double> const& H )
{
  im2wld_H_ = &H;
}

template <class PixType>
void
gen_world_units_per_pixel_process<PixType>
::set_source_vidtk_homography( image_to_plane_homography const & H )
{
  im2wld_H_ = &H.get_transform();
}

template <class PixType>
void
gen_world_units_per_pixel_process<PixType>
::set_image( vil_image_view<PixType> const& img)
{
  img_ = &img;
}

} // end namespace vidtk
