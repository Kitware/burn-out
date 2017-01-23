/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "compute_transformations.h"

#include <utilities/homography_util.h>
#include <utilities/geo_coordinate.h>

#include <vgl/algo/vgl_h_matrix_2d_compute_linear.h>
#include <vnl/vnl_math.h>

#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_homg_point_2d.h>
#include <vgl/vgl_homg_point_3d.h>
#include <vgl/algo/vgl_rotation_3d.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vnl/vnl_double_3x4.h>
#include <vnl/vnl_double_3x3.h>
#include <vpgl/vpgl_calibration_matrix.h>
#include <vpgl/vpgl_perspective_camera.h>

#include <sstream>

#include <iomanip>

#include <logger/logger.h>

VIDTK_LOGGER( "compute_transformations_cxx" );


namespace vidtk
{

  // -----------------------------------------------------------------------
//For now this function is only local to this file
/** Computes the UTM corner of image points from metadata
 *
 * @param[in] md - Metadata for the image
 * @param[in] ni - Image Width
 * @param[in] nj - Image Height
 * @param[out] utm - The Easting and northings
 * @param[out] is_north - Is this utm in the northern hemesphere
 * @param[out] zone - The utm zone number
 */
void compute_utm_corners( video_metadata const& md,
                          unsigned int ni, unsigned int nj,
                          std::vector< vgl_homg_point_2d< double > > & utm, bool & is_north, int & zone );


// -----------------------------------------------------------------------
//see h file for description
bool compute_homography( std::vector< vgl_homg_point_2d< double > > const& src_pts,
                         std::vector< vgl_homg_point_2d< double > > const& dst_pts,
                         vgl_h_matrix_2d<double> & H_src2dst )
{
  LOG_ASSERT( src_pts.size() >= 4 && src_pts.size() == dst_pts.size(),
    "homography computation: Input data is not valid." );

  vgl_h_matrix_2d_compute_linear compute_linear;
  bool h_computed = compute_linear.compute( src_pts, dst_pts, H_src2dst );

  if ( ! h_computed )
  {
    LOG_ERROR( "Could not compute homography." );
    return false;
  }

  return true;
}

// ----------------------------------------------------------------
//see h file for description
bool compute_image_to_geographic_homography(
      std::vector< vgl_homg_point_2d< double > > const& img_pts,
      std::vector< vgl_homg_point_2d< double > > const& utm_pts,
      vgl_h_matrix_2d<double> & H_img2wld,
      vgl_h_matrix_2d<double> & T_wld2utm )
{
  LOG_ASSERT( img_pts.size() >= 4 && img_pts.size() == utm_pts.size(),
    "img2utm homography computation: Input data is not valid." );

#ifdef DEBUG_PRINT
  IS_INFO_ENABLED()
  {
    std::stringstream oss;
    oss.precision( 12 );
    oss <<"UL UTM : " << utm_pts[0]
        << "UR UTM : " << utm_pts[1]
        << "LR UTM : " << utm_pts[2]
        << "LL UTM : " << utm_pts[3];
    LOG_INFO (oss.str());
  }
#endif

  // Normalization for the (tracking) world coordinates based on the image
  // center, because center is preferred than a corner in case four corners
  // are absent.
  vgl_h_matrix_2d<double> normalize;
  normalize.set_identity();

  double mean_pt[2] = {0.0, 0.0};
  for( unsigned i = 0; i < utm_pts.size(); i++ )
  {
    mean_pt[0] += utm_pts[i].x()/utm_pts[i].w();
    mean_pt[1] += utm_pts[i].y()/utm_pts[i].w();
  }
  mean_pt[0] /= utm_pts.size();
  mean_pt[1] /= utm_pts.size();
  normalize.set_translation( -mean_pt[0], -mean_pt[1] );

  double unnormalize[2];
  unnormalize[0] = mean_pt[0];
  unnormalize[1] = mean_pt[1];

  std::vector< vgl_homg_point_2d<double> > wld_pts( utm_pts.size() );

  for( unsigned i = 0; i < utm_pts.size(); i++ )
  {
    wld_pts[i] = normalize * utm_pts[i];
  }

  if( ! compute_homography( img_pts, wld_pts, H_img2wld ) )
  {
    LOG_ERROR( "Could not compute homography from pixels to UTM." );
    return false;
  }

  // Un-normalize the translation offset
  T_wld2utm.set_identity();
  T_wld2utm.set_translation( unnormalize[0], unnormalize[1] );

  return true;
}


// -----------------------------------------------------------------------


bool synthesize_corner_point(
  const vgl_homg_point_2d< double >& input,
  const homography::transform_t& homog,
  geographic::geo_coords& output )
{
  vgl_homg_point_2d< double > warped = homog * input;

  if( warped.w() != 0 )
  {
    output = geographic::geo_coords( warped.x()/warped.w(),
                                     warped.y()/warped.w() );
    return true;
  }
  return false;
}


// -----------------------------------------------------------------------
//see h file for description
bool compute_image_to_geographic_homography(
      std::vector< vgl_homg_point_2d< double > > const& img_pts,
      std::vector< geographic::geo_coords > const& latlon_pts,
      vgl_h_matrix_2d<double> & H_img2wld,
      plane_to_utm_homography & H_wld2utm )
{
  LOG_ASSERT( img_pts.size() >= 4 && img_pts.size() == latlon_pts.size(),
    "img2utm homography computation: Input data is not valid." );

  const int ref_zone = latlon_pts[0].zone();
  const bool ref_is_north = latlon_pts[0].is_north();

  std::vector< vgl_homg_point_2d< double > > utm_pts( latlon_pts.size() );

  for( unsigned i = 0; i < latlon_pts.size(); i++ )
  {
    // Test whether all correspondence points are in the same UTM zone.
    if( ref_zone != latlon_pts[i].zone()
     || ref_is_north != latlon_pts[i].is_north() )
    {
      LOG_ERROR( "Could not compute image-to-world and image-to-utm "
        "transformation, because the provided geographic locations are"
        " not in a single UTM zone." );
      return false;
    }

    // Convert lat/long to UTM through vidtk::geographic::geocoord
    utm_pts[i].set( latlon_pts[i].easting(), latlon_pts[i].northing() );
  }

  homography::transform_t T_wld2utm;
  if( ! compute_image_to_geographic_homography( img_pts,
                                                utm_pts,
                                                H_img2wld,
                                                T_wld2utm ) )
  {
    return false;
  }

  H_wld2utm.set_transform( T_wld2utm );
  H_wld2utm.set_dest_reference( utm_zone_t( ref_zone, ref_is_north ) );

  return true;
}

// -----------------------------------------------------------------------
//see h file for description
bool compute_image_to_geographic_homography(
      std::vector< vgl_homg_point_2d< double > > const& img_pts,
      std::vector< geographic::geo_coords > const& latlon_pts,
      image_to_plane_homography & H_img2wld,
      plane_to_utm_homography & H_wld2utm )
{
  vgl_h_matrix_2d<double> temp;
  if( !compute_image_to_geographic_homography( img_pts, latlon_pts, temp, H_wld2utm ) )
  {
    H_img2wld.set_valid( false );
    H_wld2utm.set_valid( false );
    return false;
  }

  H_img2wld.set_transform(temp);

  return true;
}

// -----------------------------------------------------------------------
//please see definition for description
void compute_utm_corners( video_metadata const& md,
                          unsigned int ni, unsigned int nj,
                          std::vector< vgl_homg_point_2d< double > > & utm, bool & is_north, int & zone )
{
  const double invalid_default = 0;
  // Set up the corner pixel locations as a 3x4
  vnl_matrix_fixed<double,3,4> src_pts;

  src_pts(0,0) = 0.0;      src_pts(1,0) = 0.0;    // source upper left
  src_pts(0,1) = ni-1;     src_pts(1,1) = 0.0;    // source upper right
  src_pts(0,2) = ni-1;     src_pts(1,2) = nj-1;   // source lower right
  src_pts(0,3) = 0.0;      src_pts(1,3) = nj-1;   // source lower left
  for (unsigned i=0; i<4; ++i)
  {
    src_pts(2,i) = 1.0;
  }

  double horizontalFocalLength = static_cast<double>(ni) / (2 * std::tan( md.sensor_horiz_fov() * vnl_math::pi / 180.0 / 2.0 ) );
  double verticalFocalLength = horizontalFocalLength;
  if(md.sensor_vert_fov() != invalid_default)
  {
    verticalFocalLength = static_cast<double>(nj) / (2 * std::tan( md.sensor_vert_fov() * vnl_math::pi / 180.0 / 2.0 ) );
  }

  // Assume principal point at the center of the image
  vgl_point_2d<double> pp(ni/2.0, nj/2.0);

  // Assuming zero skew and unit aspect ratio
  double aspect_ratio = verticalFocalLength / horizontalFocalLength;
  vpgl_calibration_matrix<double> K(horizontalFocalLength, pp, 1, aspect_ratio);

  // Convert and platform location and image center to UTM
  geographic::geo_coords platform_loc( md.platform_location().get_latitude(),
                                       md.platform_location().get_longitude() );
  vgl_point_3d<double> center( platform_loc.easting(), platform_loc.northing(), md.platform_altitude() );
  is_north = platform_loc.is_north();
  zone = platform_loc.zone();
  geographic::geo_coords center_utm;
  center_utm = geographic::geo_coords( md.frame_center().get_latitude(),
                                       md.frame_center().get_longitude() );

  //identity rotation
  vgl_rotation_3d<double> I;
  // Construct the camera
  vpgl_perspective_camera<double> camera(K, center, I);

  // frame center on ground plane
  vgl_homg_point_3d<double> gnd_center(center_utm.easting(), center_utm.northing(), 0.0);
  // point the camera at the ground point assuming Z is up
  camera.look_at(gnd_center);

  LOG_INFO( "Camera=\n" << camera << "");

  // The cleaner and faster way is to get the world_to_image homography
  // directly from the camera matrix.  Given the 3x4 camera matrix,
  // the homography is the 1st, 2nd, and 4th columns (since Z=0).
  vnl_double_3x4 P = camera.get_matrix();
  vnl_double_3x3 H = P.extract(3,3);
  H.set_column(2, P.get_column(3));
  vgl_h_matrix_2d<double> wld_to_img(H);

  // of course image_to_world homography is just the inverse of this
  vgl_h_matrix_2d<double> img_to_wld = wld_to_img.get_inverse();

  // Image points and correspondences
  utm.resize( 4 );
  for( unsigned i = 0; i < utm.size(); i++ )
  {
    vgl_homg_point_2d<double> src( src_pts(0,i), src_pts(1,i) );
    utm[i] = img_to_wld * src;
  }
}

// -----------------------------------------------------------------------
//see h file for description
bool compute_image_to_geographic_homography( video_metadata const& md,
                                             std::pair<unsigned, unsigned> img_wh,
                                             vgl_h_matrix_2d<double> & H_img2wld,
                                             plane_to_utm_homography & H_wld2utm )
{
    const double invalid_default = 0;

    if( !md.is_valid() )
    {
      // Cannot compute any output homographies because the metadata is invalid.
      return false;
    }

    std::vector< vgl_homg_point_2d< double > >img_pts( 4 );
    unsigned ni = img_wh.first;
    unsigned nj = img_wh.second;


    // Prefer to compute the four lat/lon corners, as opposed to using the four lat/lon corners in the metadata
    // Required metadata for computing
    if(  md.platform_altitude() == invalid_default
      || !md.frame_center().is_valid()
      || !md.platform_location().is_valid()
      || md.sensor_horiz_fov() == invalid_default )
    {
      LOG_INFO( "Missing metadata to compute corner points." );

      img_pts[0].set(    0, 0    ); // tl
      img_pts[1].set( ni-1, 0    ); // tr
      img_pts[2].set( ni-1, nj-1 ); // br
      img_pts[3].set(    0, nj-1 ); // bl

      // Prefer using corner points in the metadata if it exists, otherwise rely on internal md homography
      if( md.has_valid_corners() )
      {
        LOG_INFO( "Metadata has four corner locations, using them for computing "
                  "image-to-world transformation." );

        std::vector< geographic::geo_coords > geo_pts( 4 );
        //tl
        geo_pts[0] = geographic::geo_coords( md.corner_ul().get_latitude(),
                                             md.corner_ul().get_longitude() );
        //tr
        geo_pts[1]= geographic::geo_coords( md.corner_ur().get_latitude(),
                                            md.corner_ur().get_longitude() );
        //br
        geo_pts[2]= geographic::geo_coords( md.corner_lr().get_latitude(),
                                            md.corner_lr().get_longitude() );
        //bl
        geo_pts[3]= geographic::geo_coords( md.corner_ll().get_latitude(),
                                            md.corner_ll().get_longitude() );

        // Call the appropriate function to do the actual computation.
        return compute_image_to_geographic_homography( img_pts,
                                                       geo_pts,
                                                       H_img2wld,
                                                       H_wld2utm );
      }
      else if( md.has_valid_img_to_wld_homog() )
      {
        LOG_INFO( "Metadata has an internal image to world homog specified." );

        homography::transform_t homog = md.img_to_wld_homog().get_transform();

        std::vector< geographic::geo_coords > geo_pts( 4 );

        if( !synthesize_corner_point( img_pts[0], homog, geo_pts[0] ) ||
            !synthesize_corner_point( img_pts[1], homog, geo_pts[1] ) ||
            !synthesize_corner_point( img_pts[2], homog, geo_pts[2] ) ||
            !synthesize_corner_point( img_pts[3], homog, geo_pts[3] ) )
        {
          LOG_ERROR( "Invalid homography, unable to synthesize corner points.");
          return false;
        }

        // Call the appropriate function to do the actual computation.
        return compute_image_to_geographic_homography( img_pts,
                                                       geo_pts,
                                                       H_img2wld,
                                                       H_wld2utm );
      }
      else if( md.has_valid_gsd() && md.frame_center().is_valid() )
      {
        LOG_INFO( "Metadata has a valid GSD - location pair, attempting to use." );

        H_img2wld.set_identity();
        H_img2wld.set_rotation( vnl_math::pi );
        H_img2wld.set_scale( md.horizontal_gsd() );

        vgl_homg_point_2d< double > image_pt( img_wh.first / 2.0, img_wh.second / 2.0 );
        vgl_homg_point_2d< double > wld_pt = H_img2wld * image_pt;

        geo_coord::geo_coordinate ll( md.frame_center() );
        geo_coord::geo_UTM geo_reference_utm = ll.get_utm();

        double offset_e = geo_reference_utm.get_easting()  - wld_pt.x();
        double offset_n = geo_reference_utm.get_northing() - wld_pt.y();

        H_wld2utm.set_identity( true );
        vidtk::homography::transform_t xform = H_wld2utm.get_transform();
        xform.set_translation( offset_e, offset_n );

        H_wld2utm.set_dest_reference(
          vidtk::utm_zone_t(
            geo_reference_utm.get_zone(),
            geo_reference_utm.is_north() ) ).set_transform( xform );

        return true;
      }
      else
      {
        LOG_WARN( "Also missing corner points in metadata with time: " << md.timeUTC() );
        H_wld2utm.set_valid( false );
        return false;
      }
    }

    img_pts[0].set(    0, 0    ); // tl
    img_pts[1].set( ni-1, 0    ); // tr
    img_pts[2].set( ni-1, nj-1 ); // br
    img_pts[3].set(    0, nj-1 ); // bl
    std::vector< vgl_homg_point_2d< double > > computed_utm_pts;
    bool is_north;
    int zone;

    compute_utm_corners( md, ni, nj, computed_utm_pts, is_north, zone );

    homography::transform_t T_wld2utm;
    if( ! compute_image_to_geographic_homography( img_pts,
                                                  computed_utm_pts,
                                                  H_img2wld,
                                                  T_wld2utm ) )
    {
      return false;
    }
    H_wld2utm.set_transform(T_wld2utm);
    H_wld2utm.set_dest_reference( utm_zone_t( zone, is_north ) );
    H_wld2utm.set_valid( true );

    return true;
}

// -----------------------------------------------------------------------
//see h file for description
bool extract_latlon_corner_points( video_metadata const& md,
                                   unsigned int ni, unsigned int nj,
                                   std::vector<  geo_coord::geo_lat_lon > & latlon_pts )
{
  const double invalid_default = 0;
  latlon_pts.clear();
  if(  md.platform_altitude() == invalid_default
    || !md.frame_center().is_valid()
    || !md.platform_location().is_valid()
    || md.sensor_horiz_fov() == invalid_default )
  {
    if( md.has_valid_corners() )
    {
      latlon_pts.resize(4);
      //tl
      latlon_pts[0] =  md.corner_ul();
      //tr
      latlon_pts[1]=  md.corner_ur();
      //br
      latlon_pts[2]=  md.corner_lr();
      //bl
      latlon_pts[3]=  md.corner_ll();
    }
    else
    {
      return false;
    }
  }
  else
  {
    std::vector< vgl_homg_point_2d< double > > computed_utm_pts;
    bool is_north;
    int zone;
    latlon_pts.resize(4);

    compute_utm_corners( md, ni, nj, computed_utm_pts, is_north, zone );
    for(unsigned int i = 0; i < 4; ++i)
    {
      geographic::geo_coords pt = geographic::geo_coords( zone, is_north,
                                                          computed_utm_pts[i].x()/computed_utm_pts[i].w(),
                                                          computed_utm_pts[i].y()/computed_utm_pts[i].w() );
      latlon_pts[i] =  geo_coord::geo_lat_lon( pt.latitude(), pt.longitude() );
    }


  }
  return true;
}


// -----------------------------------------------------------------------
//see h file for description
bool compute_image_to_geographic_homography(
  std::vector< vgl_homg_point_2d< double > > const& img_pts,
  std::vector< geographic::geo_coords > const& latlon_pts,
  utm_to_image_homography & H_utm2img )
{
  image_to_plane_homography H_img2wld;
  plane_to_utm_homography   H_wld2utm;
  if(!compute_image_to_geographic_homography(img_pts,latlon_pts, H_img2wld, H_wld2utm))
  {
    return false;
  }
  image_to_utm_homography i2uH =  H_wld2utm*H_img2wld;
  H_utm2img = i2uH.get_inverse();
  return true;
}

// -----------------------------------------------------------------------

} // namespace vidtk
