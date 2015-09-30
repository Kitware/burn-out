/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_compute_transformations_h_
#define vidtk_compute_transformations_h_

/**
\file
\brief Functions to compute transformations between different coordinate
systems typically used in the tracking pipeline. 
*/

#include <vgl/vgl_homg_point_2d.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <utilities/log.h>
#include <utilities/homography.h>
#include <utilities/video_metadata.h>
#include <geographic/geo_coords.h>
#include <vcl_utility.h>

namespace vidtk 
{

/// \brief Compute plane-to-plane transformation (Homography) 
///
/// Computes the transformation through the provided set (typically 
/// four image corners) of correspondences. Note that this function returns
/// two homographies H_img2wld is provide and H_img2utm can be constructed.
/// Tracking world will be *normalized* based on the first point in the 
/// provided list.
///
/// Function parameters:
///  src_pts: input set of corresponding points in the source coordinates. 
///
///  dst_pts: input set of corresponding points in the destination coordinates. 
///
///  H_src2dst: output homography transformation from src_pts to dst_pts, i.e.
///             dst_pts = H_src2dst * src_pts
///
bool compute_homography( vcl_vector< vgl_homg_point_2d< double > > const& src_pts,
                         vcl_vector< vgl_homg_point_2d< double > > const& dst_pts,
                         vgl_h_matrix_2d<double> & H_src2dst );

/// \brief Compute image(pixels)-to-geographic(UTM) Homography 
/// transformation.
///
/// Computes the transformation through the provided set (typically 
/// four image corners) of correspondences. Note that this function returns
/// two homographies H_img2wld is provide and H_img2utm can be constructed.
/// Tracking world will be *normalized* based on the first point in the 
/// provided list.
///
/// Function parameters:
///  img_pts: input set of corresponding points in image coordinates. 
///           First correspondence point will be used as the origin of the 
///           wld coordinates.
///  utm_pts: input set of corresponding points in geographic (UTM) 
///           coordinates. Assumes that the all points will be in one UTM zone.
///  H_img2wld: output homography transformation from image to (tracking) 
///             world coordinates, i.e. units from pixels to meters.
///  T_wld2utm: translation transformation for unnormalizing the (tracking) world
///             coordinates to geographic (UTM) coordinates. Image to geographic 
///             transformation is: H_img2utm = T_wld2utm * H_img2wld. Note that
///             the order of multiplication is important.
///
bool compute_image_to_geographic_homography( 
      vcl_vector< vgl_homg_point_2d< double > > const& img_pts,
      vcl_vector< vgl_homg_point_2d< double > > const& utm_pts,
      vgl_h_matrix_2d<double> & H_img2wld,
      vgl_h_matrix_2d<double> & T_wld2utm );


/// \brief Compute image(pixels)-to-geographic(lat/lon) Homography 
/// transformation.
///
/// Computes the transformation through the provided set (typically 
/// four image corners) of correspondences. Note that this function returns
/// two homographies H_img2wld is provide and H_img2utm can be constructed.
/// Tracking world will be *normalized* based on the first point in the 
/// provided list.
///   Note that we do not support H_img2latlon because lat/long coordinates 
/// are not suitable for the planar (homoography) transformation. This 
/// function (with lat/lon inputs) does lat/lon-to-UTM conversion and peforms 
/// some sanity checks before calling the actual that computes the homography.
///
/// Function parameters:
///  img_pts: input set of corresponding points in image coordinates. 
///           First correspondence point will be used as the origin of the 
///           wld coordinates.
///  latlon_pts: input set of corresponding points in geographic (lat/long) 
///               coordinates. 
///  H_img2wld: output homography transformation from image to (tracking) 
///             world coordinates, i.e. units from pixels to meters.
///  T_wld2utm: translation transformation for unnormalizing the (tracking) world
///             coordinates to geographic (UTM) coordinates. Image to geographic 
///             transformation is: H_img2utm = T_wld2utm * H_img2wld. Note that
///             the order of multiplication is important. T_wld2utm also stores 
///             the UTM zone information. 
///
bool compute_image_to_geographic_homography(
      vcl_vector< vgl_homg_point_2d< double > > const& img_pts,
      vcl_vector< geographic::geo_coords > const& latlon_pts,
      vgl_h_matrix_2d<double> & H_img2wld,
      plane_to_utm_homography & T_wld2utm );

bool compute_image_to_geographic_homography(
      vcl_vector< vgl_homg_point_2d< double > > const& img_pts,
      vcl_vector< geographic::geo_coords > const& latlon_pts,
      image_to_plane_homography & H_img2wld,
      plane_to_utm_homography & H_wld2utm );

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
                                             plane_to_utm_homography & H_wld2utm );

} // namespace vidtk 

#endif // vidtk_compute_transformations_h_
