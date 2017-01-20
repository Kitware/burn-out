/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


/** \file
 * This file contains homography helper functions that are
 * related to homography class not members of the class.
 */

#ifndef _VIDTK_HOMOGRAPHY_UTIL_H_
#define _VIDTK_HOMOGRAPHY_UTIL_H_

#include <utilities/homography.h>

#include <geographic/geo_coords.h>


namespace vidtk {

  // ----------------------------------------------------------------
  /** \brief Multiply two homographies.
   *
   * Two homographies with a shared type
   * can be multiplied together to get a different type fo
   * homography. For example if you want to go from src -> ref -> wld,
   * a new homography can be created to do that by
   * \code
   src_to_wld = ref_to_wld * src_to_ref;
   * \endcode
   *
   */
  template< class SrcT, class SharedType, class DestType >
  vidtk::homography_T< SrcT, DestType > operator*( vidtk::homography_T< SharedType, DestType > const & l,
                                                   vidtk::homography_T< SrcT, SharedType > const & r );

  // ----------------------------------------------------------------
  /** Transform a geo coordinate using a homography;
   *
   */
  vgl_homg_point_2d< double > operator*( vidtk::utm_to_image_homography const & l,
                                         geographic::geo_coords const & r );

  /** \brief Point transform operator.
   *
   * This operator transforms the specified
   * point using the transform in the homography. Since the input and
   * output points have no further context, it is up to the caller to
   * determine the units. Note that the homography is NOT checked for
   * validity. That is for the caller to do.
   *
   * @param l Homography doing the transform.
   * @param r Point being transformed
   * @return A normalized transformed point.
   */
  vgl_homg_point_2d< double > operator*( vidtk::homography const & l,
                                         vgl_homg_point_2d< double > const & r );

 // ----------------------------------------------------------------
 /** Transform a point from image coords to UTM coords.
  *
  */
  geographic::geo_coords operator*( vidtk::image_to_utm_homography const & l,
                                    vgl_homg_point_2d< double > const & r );

  bool
  jacobian_from_homo_wrt_loc( vnl_matrix_fixed<double, 2, 2> & jac,
                              vnl_matrix_fixed<double, 3, 3> const& H,
                              vnl_vector_fixed<double, 2>    const& from_loc );

} // end namespace

#endif /* _VIDTK_HOMOGRAPHY_UTIL_H_ */
