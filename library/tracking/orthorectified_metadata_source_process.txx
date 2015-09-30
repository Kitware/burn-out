/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "orthorectified_metadata_source_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/compute_transformations.h>
#include <vcl_utility.h>
#include <geographic/geo_coords.h>

#include <vgl/algo/vgl_h_matrix_2d_compute_linear.h>
#include <vgl/vgl_homg_point_2d.h>

namespace vidtk
{

/// \brief Defines ref, wld and utm coordinate systems and respective
/// transformations.
template< class PixType >
class orthorectified_metadata_source_impl
{
public:
  // Configuration parameters
  config_block config;
  geographic::geo_coords tl; // top left
  geographic::geo_coords tr; // top right
  geographic::geo_coords br; // bottom right
  geographic::geo_coords bl; // bottom left

  // Input data
  const vil_image_view< PixType > * ref_img;

  // Ouput data
  // - 3x3 Homography transformation matrices
  image_to_plane_homography H_ref2wld; // see header file for description
  plane_to_utm_homography H_wld2utm; // see header file for description

  orthorectified_metadata_source_impl()
    : ref_img( NULL )
  {
    config.add_parameter( "top_left",
      "0.0 0.0",
      "Geographic location ([latitude longitude] vector in degrees) of"
      " the top left corner of the first image." );

    config.add_parameter( "top_right",
      "0.0 0.0",
      "Geographic location ([latitude longitude] vector in degrees) of"
      " the top right corner of the first image." );

    config.add_parameter( "bottom_right",
      "0.0 0.0",
      "Geographic location ([latitude longitude] vector in degrees) of"
      " the bottom right corner of the first image." );

    config.add_parameter( "bottom_left",
      "0.0 0.0",
      "Geographic location ([latitude longitude] vector in degrees) of"
      " the bottom left corner of the first image." );
  }

  /// \brief Use 4 lat/long points to compute H_ref2wld and H_wld2utm
  bool compute_transformations()
  {
    // Image locations of four corners
    vcl_vector< vgl_homg_point_2d< double > > frame_corners_pix( 4 );
    frame_corners_pix[0].set( 0              , 0 ); // tl
    frame_corners_pix[1].set( ref_img->ni()-1, 0 ); // tr
    frame_corners_pix[2].set( ref_img->ni()-1, ref_img->nj()-1 ); // br
    frame_corners_pix[3].set( 0              , ref_img->nj()-1 ); // bl

    vcl_vector< geographic::geo_coords > frame_corners_latlon( 4 );
    frame_corners_latlon[0] = tl;
    frame_corners_latlon[1] = tr;
    frame_corners_latlon[2] = br;
    frame_corners_latlon[3] = bl;

    if( ! compute_image_to_geographic_homography( frame_corners_pix,
            frame_corners_latlon, H_ref2wld, H_wld2utm ) )
    {
      return false;
    }

    return true;
  }
};

template <class PixType>
orthorectified_metadata_source_process< PixType >
::orthorectified_metadata_source_process( vcl_string const& name )
: process( name, "orthorectified_metadata_source_process" ),
  impl_( new orthorectified_metadata_source_impl< PixType > )
{}

template <class PixType>
orthorectified_metadata_source_process< PixType >
::~orthorectified_metadata_source_process()
{
  delete impl_;
}

template <class PixType>
config_block
orthorectified_metadata_source_process< PixType >
::params() const
{
  return impl_->config;
}

template <class PixType>
bool
orthorectified_metadata_source_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    bool isOK = true;
    double tmp[2];

    blk.get( "top_left", tmp );
    isOK = isOK && impl_->tl.reset( tmp[0], tmp[1] );

    blk.get( "top_right", tmp );
    isOK = isOK && impl_->tr.reset( tmp[0], tmp[1] );

    blk.get( "bottom_right", tmp );
    isOK = isOK && impl_->br.reset( tmp[0], tmp[1] );

    blk.get( "bottom_left", tmp );
    isOK = isOK && impl_->bl.reset( tmp[0], tmp[1] );

    if( ! isOK )
    {
      throw unchecked_return_value( "Failed to reset one of the four "
        "corner geo_coords" );
    }

    impl_->config.update( blk );
  }
  catch(unchecked_return_value & e )
  {
    log_error( this->name() << ": couldn't set parameters: "<< e.what() <<"\n" );
    return false;
  }

  return true;
}

template <class PixType>
bool
orthorectified_metadata_source_process< PixType >
::initialize()
{
  return true;
}

template <class PixType>
bool
orthorectified_metadata_source_process< PixType >
::step()
{
  if( ! impl_->ref_img )
    return false;

  bool isOK = true;
  static bool first_frame = true;
  if( first_frame )
  {
    first_frame = false;

    // Do only once, for the first frame. We cannot move this to initialize()
    // because source image is not available there for the size.
    isOK = impl_->compute_transformations();
  }
  // Do nothing!

  // Invalidate input data.
  impl_->ref_img = NULL;

  return isOK;
}

template <class PixType>
void
orthorectified_metadata_source_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  impl_->ref_img = &img;
}

template <class PixType>
image_to_plane_homography const &
orthorectified_metadata_source_process< PixType >
::ref_to_wld_homography() const
{
  return impl_->H_ref2wld;
}

template <class PixType>
plane_to_utm_homography const &
orthorectified_metadata_source_process< PixType >
::wld_to_utm_homography() const
{
  return impl_->H_wld2utm;
}

} // namespace vidtk
