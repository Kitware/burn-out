/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef dual_rof_denoise_h_
#define dual_rof_denoise_h_

#include <vil/vil_image_view.h>

/// \file Denoise images using total variation with a L1 norm.
///
/// This denoising code is also useful in dense optical flow
/// and dense depth estimation.  The implementation below is
/// based on the equations in the following paper:
///
/// A. Wedel, T. Pock, C. Zach, H. Bischof, and D. Cremers,
/// "An improved algorithm for TV-L1 optical flow",
/// Statistical and Geometrical Approaches to Visual Motion Analysis
/// pg 23-25, Springer, 2009.
/// http://cvpr.in.tum.de/old/pub/pub/DagstuhlOpticalFlowChapter.pdf


namespace vidtk
{

/// Apply several iterations of the dual Rudin, Osher and Fatemi model.
/// Solves the total variation minimization with L1 norm over the image.
/// The resulting image is denoised, but sharp edges are preserved.
/// \param src The source image
/// \retval dest The destination image
/// \param iterations The number of iterations
/// \param theta A tuning parameter to control amount of smoothing.
///              Larger values produce more smoothing.
/// \param step The step size for each iteration
void
dual_rof_denoise(const vil_image_view<double>& src,
                 vil_image_view<double>& dest,
                 unsigned iterations,
                 double theta, double step = 0.25);


/// Add the scaled divergence of a vector field to the source image.
/// Compute dest = src + scale*div(vec)
/// \param vec The vector field (x,y in planes 0,1)
/// \param src The source image
/// \param scale The scale factor applied to the divergence term
/// \retval dest The destination image
void
add_scaled_divergence(const vil_image_view<double>& vec,
                      const vil_image_view<double>& src,
                      double scale,
                      vil_image_view<double>& dest);


/// Add (in place) the scaled gradient of the source image to the vector field.
/// Compute vec += scale*grad(src)
/// \param src The source image
/// \param vec The vector field (x,y in planes 0,1)
/// \param scale The scale factor applied to the gradient vectors
void
add_scaled_gradient(const vil_image_view<double>& src,
                    vil_image_view<double>& vec,
                    double scale);


/// Truncate all vectors greater than unit length to unit length.
/// \retval vec The input vector field, modified in place (x,y in planes 0,1).
void
truncate_vectors(vil_image_view<double>& vec);

}

#endif // dual_rof_denoise_h_
