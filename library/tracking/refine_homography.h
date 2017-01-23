/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef refine_homography_h_
#define refine_homography_h_

#include <vil/vil_image_view.h>
#include <vnl/vnl_double_3x3.h>
#include <vnl/vnl_double_2.h>

#include <vector>

namespace vidtk
{

/// Refines the planar homography that warps from moving -> fixed images
/// \param fixed the destination image of the homography
/// \param moving the source image of the homography
/// \param H the homography to be refined
/// \param grad_thresh the threshold on gradients for edgel computation
/// \param num_iters the number of ICP iterations
/// \param search_radius the max distance to search for correspondences
/// \param normal_cutoff the threshold on cos(angle) between the warped normal and fixed normal
/// \param sigma the smoothing scale of the edgels, 0 means no smoothing of the images
void refine_homography(const vil_image_view<double> &fixed, const vil_image_view<double> &moving,
                       vnl_double_3x3 &H, double grad_thresh, unsigned int num_iters,
                       unsigned int search_radius, double normal_cutoff = 0.8, double sigma = 0.0);


/// Refines the planar homography that warps from moving -> fixed images
/// \param fixed the destination image of the homography
/// \param moving the source image of the homography
/// \param fixed_mask a mask for fixed indicating which pixels to use for registration
/// \param moving_mask a mask for moving indicating which pixels to used for registration
/// \param H the homography to be refined
/// \param grad_thresh the threshold on gradients for edgel computation
/// \param num_iters the number of ICP iterations
/// \param search_radius the max distance to search for correspondences
/// \param normal_cutoff the threshold on cos(angle) between the warped normal and fixed normal
/// \param sigma the smoothing scale of the edgels, 0 means no smoothing of the images
void refine_homography(const vil_image_view<double> &fixed, const vil_image_view<double> &moving,
                       const vil_image_view<bool> &fixed_mask, const vil_image_view<bool> &moving_mask,
                       vnl_double_3x3 &H, double grad_thresh, unsigned int num_iters,
                       unsigned int search_radius, double normal_cutoff = 0.8, double sigma = 0.0);


/// Computes a homography corresponding to a cropped source image.
/// \param H the homography to crop
/// \param i0 the left coordiante of the crop region
/// \param j0 the top coordinate of the crop region
void crop_homography_source(vnl_double_3x3 &H, double i0, double j0);


struct edgel
{
  edgel(double x, double y, double angle, double m);

  vnl_double_2 pos, n;
  double mag;
};

/// Extracts edgels from the moving image into a vector
/// \param img the image to compute edgels on
/// \param mask
/// \param grad_thresh the threshold on gradients for edgel computation
/// \param sigma the smoothing scale of the edgels, 0 means no smoothing of the images
/// \param edgels the vector the contains the edgels
void extract_driving_edgels(const vil_image_view<double> &img,
                            const vil_image_view<bool> &mask,
                            double grad_thresh,
                            double sigma,
                            std::vector<edgel> &edgels);

/// Extracts edgels from the fixed image into a vector and index map
/// \param img the image to compute edgels on
/// \param grad_thresh the threshold on gradients for edgel computation
/// \param sigma the smoothing scale of the edgels, 0 means no smoothing of the images
/// \param edgels the vector the contains the edgels
/// \param pixel map of indicies into the edgel vector, 0 means no edgel
void extract_matchable_edgels(const vil_image_view<double> &img,
                              const vil_image_view<bool> &mask,
                              double grad_thresh,
                              double sigma,
                              std::vector<edgel> &edgels,
                              vil_image_view<unsigned int> &index);

struct edgel_match
{
  edgel_match(unsigned int moving, unsigned int fixed) : m(moving), f(fixed) {}

  //indices into moving and fixed edgel vectors
  unsigned int m, f;
};

/// Warps a normal with H
/// \param H the homography to warp with
/// \param pt the location of the normal
/// \param normal the vector to warp
vnl_double_2 warp_normal(const vnl_double_3x3 &H, const vnl_double_2 &pt, const vnl_double_2 &normal);

/// Matches a moving edgel set with a fixed edgel set
/// \param H the homography that warps moving to fixed
/// \param e_fixed the vector of fixed edgels
/// \param e_moving the vector of moving edgels
/// \param index the pixel map of indicies into e_fixed
/// \param search_radius the max distance to search for correspondences in fixed coord system
/// \param matches vector of matches for the computed matches
/// \param normal_cutoff the threshold on cos(angle) between the warped normal and fixed normal
void match_edgels(const vnl_double_3x3 &H,
                  const std::vector<edgel> &e_fixed,
                  const std::vector<edgel> &e_moving,
                  const vil_image_view<unsigned int> &index,
                  const double search_rad,
                  std::vector<edgel_match> &matches,
                  double normal_cutoff = 0.8);

/// Uses Levenberg-Marquardt to estimate a new homography using normal distances
/// \param corresp the edgel matches
/// \param e_fixed the fixed edgels
/// \param e_moving the moving edgels
/// \param H the homography
void estimate_homog_lm(const std::vector<edgel_match> &corresp,
                       const std::vector<edgel> &e_fixed,
                       const std::vector<edgel> &e_moving,
                       vnl_double_3x3 &H);

} // end namespace vidtk

#endif // refine_homography_h_
