/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "compute_transformations.h"

#include <utilities/log.h>
#include <utilities/homography.h>

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

#include <vcl_iostream.h>

namespace vidtk
{


// ----------------------------------------------------------------
/** Compute homography
 *
 * This function computes homograpies based on two sets of coordinates
 * for the same box.
 *
 * @param[in] img_pts - corner points in image space
 * @param[in] utm_pts - cormer points in UTM coordinates
 * @param[out] H_src2dst - source to destination transform
 */
bool compute_homography( vcl_vector< vgl_homg_point_2d< double > > const& src_pts,
                         vcl_vector< vgl_homg_point_2d< double > > const& dst_pts,
                         vgl_h_matrix_2d<double> & H_src2dst )
{
  log_assert( src_pts.size() >= 4 && src_pts.size() == dst_pts.size(), 
    "homography computation: Input data is not valid.\n" );

  vgl_h_matrix_2d_compute_linear compute_linear;
  bool h_computed = compute_linear.compute( src_pts, dst_pts, H_src2dst );

  if ( ! h_computed )
  {
    log_error( "Could not compute homography.\n" );
    return false;
  }

  return true;
}

// ----------------------------------------------------------------
/** Compute homography
 *
 * This function computes homograpies based on two sets of coordinates
 * for the same box.
 *
 * @param[in] img_pts - corner points in image space
 * @param[in] utm_pts - cormer points in UTM coordinates
 * @param[out] H_img2wld - image to world homography
 * @param[out] T_wld2utm - world to UTM homography
 */
bool compute_image_to_geographic_homography( 
      vcl_vector< vgl_homg_point_2d< double > > const& img_pts,
      vcl_vector< vgl_homg_point_2d< double > > const& utm_pts,
      vgl_h_matrix_2d<double> & H_img2wld,
      vgl_h_matrix_2d<double> & T_wld2utm )
{
  log_assert( img_pts.size() >= 4 && img_pts.size() == utm_pts.size(), 
    "img2utm homography computation: Input data is not valid.\n" );

#ifdef DEBUG_PRINT
  vcl_cout.precision( 12 );
  vcl_cout << "UL UTM : " << utm_pts[0] << vcl_endl;
  vcl_cout << "UR UTM : " << utm_pts[1] << vcl_endl;
  vcl_cout << "LR UTM : " << utm_pts[2] << vcl_endl;
  vcl_cout << "LL UTM : " << utm_pts[3] << vcl_endl;
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

  vcl_vector< vgl_homg_point_2d<double> > wld_pts( utm_pts.size() );

  for( unsigned i = 0; i < utm_pts.size(); i++ )
  {
    wld_pts[i] = normalize * utm_pts[i];
  }

  if( ! compute_homography( img_pts, wld_pts, H_img2wld ) )
  {
    log_error( "Could not compute homography from pixels to UTM.\n" );
    return false;
  }

  // Un-normalize the translation offset
  T_wld2utm.set_identity();
  T_wld2utm.set_translation( unnormalize[0], unnormalize[1] );

  return true;
}

// -----------------------------------------------------------------------
/** Compute homography
 *
 * This function computes homograpies based on two sets of coordinates
 * for the same box.
 *
 * @param[in] img_pts - corner points in image space
 * @param[in] latlon_pts - cormer points in lat/long coordiantes
 * @param[out] H_img2wld - image to world homography
 * @param[out] T_wld2utm - world to UTM homography
 */
bool compute_image_to_geographic_homography( 
      vcl_vector< vgl_homg_point_2d< double > > const& img_pts,
      vcl_vector< geographic::geo_coords > const& latlon_pts,
      vgl_h_matrix_2d<double> & H_img2wld,
      plane_to_utm_homography & H_wld2utm )
{
  log_assert( img_pts.size() >= 4 && img_pts.size() == latlon_pts.size(),
    "img2utm homography computation: Input data is not valid.\n" );

  const int ref_zone = latlon_pts[0].zone();
  const bool ref_is_north = latlon_pts[0].is_north();

  vcl_vector< vgl_homg_point_2d< double > > utm_pts( latlon_pts.size() );

  for( unsigned i = 0; i < latlon_pts.size(); i++ )
  {
    // Test whether all correspondence points are in the same UTM zone.
    if( ref_zone != latlon_pts[i].zone()
     || ref_is_north != latlon_pts[i].is_north() )
    {
      log_error( "Could not compute image-to-world and image-to-utm "
        "transformation, because the provided geographic locations are"
        " not in a single UTM zone.\n" );
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

bool compute_image_to_geographic_homography(
      vcl_vector< vgl_homg_point_2d< double > > const& img_pts,
      vcl_vector< geographic::geo_coords > const& latlon_pts,
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
/** Compute homography
 *
 * This function computes homograpy between image and the ground plane 
 * using the metadata packet.
 *
 * @param[in] md - video metadata metadata 
 * @param[in] img_wh - Width and Height of the image
 * @param[out] H_img2wld - image to world homography
 * @param[out] T_wld2utm - world to UTM homography
 */
bool compute_image_to_geographic_homography( video_metadata const& md, 
                                             vcl_pair<unsigned, unsigned> img_wh,
                                             vgl_h_matrix_2d<double> & H_img2wld,
                                             plane_to_utm_homography & H_wld2utm )
{
    const double invalid_default = 0;

    if( !md.is_valid() )
    {
      // Cannot compute any output homographies because the metadata is invalid.
      return false;
    }

    vcl_vector< vgl_homg_point_2d< double > >img_pts( 4 );
    unsigned ni = img_wh.first;
    unsigned nj = img_wh.second;

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

    // Prefer to compute the four lat/lon corners, as opposed to using the four lat/lon corners in the metadata
    // Required metadata for computing
    if(  md.platform_altitude() == invalid_default
      || !md.frame_center().is_valid()
      || !md.platform_location().is_valid()
      || md.sensor_horiz_fov() == invalid_default )
    {
      log_info( "Missing metadata to compute corner points.\n" );
      if( md.has_valid_corners() )
      {
        log_info( "Metadata has four corner locations, using them for computing "
                  "image-to-world transformation.\n" );

        img_pts[0].set(    0, 0    ); // tl
        img_pts[1].set( ni-1, 0    ); // tr
        img_pts[2].set( ni-1, nj-1 ); // br
        img_pts[3].set(    0, nj-1 ); // bl

        vcl_vector< geographic::geo_coords > geo_pts( 4 );
        //tl
        geo_pts[0] = geographic::geo_coords( md.corner_ul().lat(),
                                             md.corner_ul().lon() );
        //tr
        geo_pts[1]= geographic::geo_coords( md.corner_ur().lat(),
                                            md.corner_ur().lon() );
        //br
        geo_pts[2]= geographic::geo_coords( md.corner_lr().lat(),
                                            md.corner_lr().lon() );
        //bl
        geo_pts[3]= geographic::geo_coords( md.corner_ll().lat(),
                                            md.corner_ll().lon() );

        // Call the appropriate function to do the actual computation.
        return compute_image_to_geographic_homography( img_pts,
                                                       geo_pts,
                                                       H_img2wld,
                                                       H_wld2utm );
      }
      else
      {
        log_info( "Also missing corner points in metadata fields. No corner points available.\n" );
        H_wld2utm.set_valid( false );
        return false;
      }
    }

    double horizontalFocalLength = (double)ni / (2 * vcl_tan( md.sensor_horiz_fov() * vnl_math::pi / 180.0 / 2.0 ) );

    // Assume principal point at the center of the image
    vgl_point_2d<double> pp(ni/2.0, nj/2.0);

    // Assuming zero skew and unit aspect ratio
    vpgl_calibration_matrix<double> K(horizontalFocalLength, pp);

    // // More general calibration
    // double skew = 0.0;
    // double x_scale = 1.0;
    // double y_scale =  vert_focal_length / horiz_focal_length;
    // vpgl_calibration_matrix<double> K(horiz_focal_length, pp, x_scale,
    //                                   y_scale, skew);

    // Convert and platform location and image center to UTM
    geographic::geo_coords platform_loc( md.platform_location().lat(),
                                         md.platform_location().lon() );
    vgl_point_3d<double> center( platform_loc.easting(), platform_loc.northing(), md.platform_altitude() );
    geographic::geo_coords center_utm;
    center_utm = geographic::geo_coords( md.frame_center().lat(),
                                         md.frame_center().lon() );

    //identity rotation
    vgl_rotation_3d<double> I;
    // Construct the camera
    vpgl_perspective_camera<double> camera(K, center, I);

    // frame center on ground plane
    vgl_homg_point_3d<double> gnd_center(center_utm.easting(), center_utm.northing(), 0.0);
    // point the camera at the ground point assuming Z is up
    camera.look_at(gnd_center);

    // // backproject image point (i,j) into a line
    // vgl_homog_point_2d<double> image_pt(i,j);
    // vgl_homog_line_3d_2_points<double> line = camera.backproject(image_pt);
    // // convert to a different line class (there are too many line classes)
    // vgl_infinite_line_3d inf_line(line.point_finite(), line.direction());
    // // intersect with the ground plane
    // vgl_plane_3d<double> gnd_plane(0.0, 0.0, 1.0, 0.0);
    // vgl_point_3d<double> gnd_pt;
    // bool is_valid = vgl_intersection(gnd_plane, inf_line, gnd_pt);

    vcl_cout << "Camera=\n" << camera << "\n";

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
    vcl_vector< vgl_homg_point_2d< double > > computed_utm_pts( img_pts.size() );
    for( unsigned i = 0; i < computed_utm_pts.size(); i++ )
    {
      vgl_homg_point_2d<double> src( src_pts(0,i), src_pts(1,i) );
      computed_utm_pts[i] = img_to_wld * src;
      //vcl_cout << "img " << src << ";  utm " << computed_utm_pts[i] << "\n";
    }

    img_pts[0].set(    0, 0    ); // tl
    img_pts[1].set( ni-1, 0    ); // tr
    img_pts[2].set( ni-1, nj-1 ); // br
    img_pts[3].set(    0, nj-1 ); // bl

    homography::transform_t T_wld2utm;
    if( ! compute_image_to_geographic_homography( img_pts,
                                                  computed_utm_pts,
                                                  H_img2wld,
                                                  T_wld2utm ) )
    {
      return false;
    }
    H_wld2utm.set_transform(T_wld2utm);
    H_wld2utm.set_dest_reference( utm_zone_t( center_utm.zone(),
                                              center_utm.is_north() ) );
    H_wld2utm.set_valid( true );

    return true;
}

// -----------------------------------------------------------------------

} // namespace vidtk 
