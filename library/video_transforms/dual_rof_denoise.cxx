/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "dual_rof_denoise.h"

namespace vidtk
{


/// Apply several iterations of the dual Rudin, Osher and Fatemi model.
/// Solves the total variation minimization with L1 norm over the image.
/// The resulting image is denoised, but sharp edges are preserved.
/// Solves min_u Int { |grad(u(i,j))| + 1/(2*theta) (u(i,j)-v(i,j))^2} di dj
/// where u is \a dest and v is \a src.
/// \param src The source image
/// \retval dest The destination image
/// \param iterations The number of iterations
/// \param theta A tuning parameter to control amount of smoothing.
///              Larger values produce more smoothing.
/// \param step The step size for each iteration
template <typename T>
void
dual_rof_denoise(const vil_image_view<T>& src,
                 vil_image_view<T>& dest,
                 unsigned iterations,
                 T theta, T step)
{
  unsigned ni = src.ni(), nj = src.nj();
  assert(src.nplanes() == 1);

  vil_image_view<T> dual(ni,nj,2);
  dual.fill(0.0);
  dual_rof_denoise(src, dest, dual, iterations, theta, step);
}


/// Apply several iterations of the dual Rudin, Osher and Fatemi model.
/// Solves the total variation minimization with L1 norm over the image.
/// The resulting image is denoised, but sharp edges are preserved.
/// This version starts from and updates an existing dual variable
/// \param src The source image
/// \retval dest The destination image
/// \param dual the dual variable
/// \param iterations The number of iterations
/// \param theta A tuning parameter to control amount of smoothing.
///              Larger values produce more smoothing.
/// \param step The step size for each iteration
template <typename T>
void
dual_rof_denoise(const vil_image_view<T>& src,
                 vil_image_view<T>& dest,
                 vil_image_view<T>& dual,
                 unsigned iterations,
                 T theta, T step)
{
  assert(src.nplanes() == 1);
  dest.deep_copy(src);

  for (unsigned i=0; i<iterations; ++i)
  {
    add_scaled_gradient(dest,dual,step/theta);
    truncate_vectors(dual);
    add_scaled_divergence(dual,src,theta,dest);
  }

}

/// Apply several iterations of the weighted dual Rudin, Osher and Fatemi model.
/// Solves the total variation minimization with L1 norm over the image.
/// The resulting image is denoised, but sharp edges are preserved.
/// A weight image allows control over the amount of denoising at each pixel.
/// Solves min_u Int { w(i,j)*|grad(u(i,j))| + 1/(2*theta) (u(i,j)-v(i,j))^2} di dj
/// where u is \a dest and v is \a src and w is \a weights.
/// \param src The source image
/// \param weights An image of weights in [0,1].  A value of 1 results in denoising
///                as usual.  Smaller values reduce the denoising amount
/// \retval dest The destination image
/// \param iterations The number of iterations
/// \param theta A tuning parameter to control amount of smoothing.
///              Larger values produce more smoothing.
/// \param step The step size for each iteration
template <typename T>
void
dual_rof_weighted_denoise(const vil_image_view<T>& src,
                          const vil_image_view<T>& weights,
                          vil_image_view<T>& dest,
                          unsigned iterations,
                          T theta, T step)
{
  unsigned ni = src.ni(), nj = src.nj();
  assert(src.nplanes() == 1);
  dest.deep_copy(src);

  vil_image_view<T> dual(ni,nj,2);
  dual.fill(0.0);

  for (unsigned i=0; i<iterations; ++i)
  {
    add_scaled_gradient(dest,dual,step/theta);
    truncate_vectors(dual, weights);
    add_scaled_divergence(dual,src,theta,dest);
  }

}


/// Add the scaled divergence of a vector field to the source image.
/// Compute dest = src + scale*div(vec)
/// \param vec The vector field (x,y in planes 0,1)
/// \param src The source image
/// \param scale The scale factor applied to the divergence term
/// \retval dest The destination image
template <typename T>
void
add_scaled_divergence(const vil_image_view<T>& vec,
                      const vil_image_view<T>& src,
                      T scale,
                      vil_image_view<T>& dest)
{
  unsigned ni = vec.ni(), nj = vec.nj();
  assert(src.ni()==ni && src.nj()==nj);
  assert(vec.nplanes() == 2);
  assert(src.nplanes() == 1);
  dest.set_size(ni,nj,1);


  std::ptrdiff_t istepV=vec.istep(),  jstepV=vec.jstep(),  pstepV=vec.planestep();
  std::ptrdiff_t istepS=src.istep(),  jstepS=src.jstep();
  std::ptrdiff_t istepD=dest.istep(), jstepD=dest.jstep();


  const T*   rowV = vec.top_left_ptr();
  const T*   rowS = src.top_left_ptr();
  T*         rowD = dest.top_left_ptr();

  // special case for first row
  const T* pixelVx = rowV;
  const T* pixelVy = pixelVx + pstepV;
  const T* pixelS = rowS;
  T*       pixelD = rowD;
  // special case for pixel i=0, j=0
  *pixelD = *pixelS + scale*(*pixelVx + *pixelVy);
  pixelVy += istepV;
  pixelS += istepS;
  pixelD += istepD;
  for (unsigned i=1; i<ni-1; ++i,
       pixelVy+=istepV, pixelS+=istepS, pixelD+=istepD)
  {
    *pixelD = -(*pixelVx); // subract last Vx value before incrementing
    pixelVx += istepV;
    *pixelD += *pixelVx + *pixelVy;
    *pixelD *= scale;
    *pixelD += *pixelS;
  }
  *pixelD = *pixelS + scale*(-*pixelVx + *pixelVy);

  // the middle rows
  rowV += jstepV;
  rowS += jstepS;
  rowD += jstepD;
  for (unsigned j=1; j<nj-1; ++j, rowV += jstepV, rowS += jstepS, rowD += jstepD)
  {
    pixelVx = rowV;
    pixelVy = pixelVx + pstepV;
    pixelS = rowS;
    pixelD = rowD;
    // special case for pixel i=0
    *pixelD = *pixelS + scale*(*pixelVx + *pixelVy - *(pixelVy-jstepV));
    pixelVy += istepV;
    pixelS += istepS;
    pixelD += istepD;
    for (unsigned i=1; i<ni-1; ++i,
         pixelVy+=istepV, pixelS+=istepS, pixelD+=istepD)
    {
      *pixelD = -(*pixelVx); // subract last Vx value before incrementing
      pixelVx += istepV;
      *pixelD += *pixelVx + *pixelVy - *(pixelVy-jstepV);
      *pixelD *= scale;
      *pixelD += *pixelS;
    }
    *pixelD = *pixelS + scale*(-*pixelVx + *pixelVy - *(pixelVy-jstepV));
  }

  // special case for last row
  pixelVx = rowV - jstepV; // use the last row
  pixelVy = pixelVx + pstepV;
  pixelS = rowS;
  pixelD = rowD;
  // special case for pixel i=0
  *pixelD = *pixelS + scale*(*pixelVx - *pixelVy);
  pixelVy += istepV;
  pixelS += istepS;
  pixelD += istepD;
  for (unsigned i=1; i<ni-1; ++i,
       pixelVy+=istepV, pixelS+=istepS, pixelD+=istepD)
  {
    *pixelD = -(*pixelVx); // subract last Vx value before incrementing
    pixelVx += istepV;
    *pixelD += *pixelVx - *pixelVy;
    *pixelD *= scale;
    *pixelD += *pixelS;
  }
  *pixelD = *pixelS + scale*(-*pixelVx - *pixelVy);
}


/// Add (in place) the scaled gradient of the source image to the vector field.
/// Compute vec += scale*grad(src)
/// \param src The source image
/// \param vec The vector field (x,y in planes 0,1)
/// \param scale The scale factor applied to the gradient vectors
template <typename T>
void
add_scaled_gradient(const vil_image_view<T>& src,
                    vil_image_view<T>& vec,
                    T scale)
{
  unsigned ni = src.ni(), nj = src.nj();
  assert(vec.ni()==ni && vec.nj()==nj);
  assert(src.nplanes() == 1);
  assert(vec.nplanes() == 2);

  std::ptrdiff_t istepS=src.istep(),  jstepS=src.jstep();
  std::ptrdiff_t istepV=vec.istep(),  jstepV=vec.jstep(),  pstepV=vec.planestep();

  const T*   rowS = src.top_left_ptr();
  T*         rowV = vec.top_left_ptr();

  for (unsigned j=0; j<nj-1; ++j, rowS += jstepS, rowV += jstepV)
  {
    const T* pixelS = rowS;
    T*       pixelVx = rowV;
    T*       pixelVy = pixelVx + pstepV;
    for (unsigned i=1; i<ni-1; ++i, pixelS+=istepS, pixelVx+=istepV, pixelVy+=istepV)
    {
      *pixelVx += scale*(*(pixelS + istepS) - *pixelS);
      *pixelVy += scale*(*(pixelS + jstepS) - *pixelS);
    }
    // *pixelVx is incremented by zero in the last column
    *pixelVy += scale*(*(pixelS + jstepS) - *pixelS);
  }

  // special case for last row
  const T* pixelS = rowS;
  T*       pixelVx = rowV;
  T*       pixelVy = pixelVx + pstepV;
  for (unsigned i=1; i<ni-1; ++i, pixelS+=istepS, pixelVx+=istepV, pixelVy+=istepV)
  {
    *pixelVx += scale*(*(pixelS + istepS) - *pixelS);
    // *pixelVy is incremented by zero in the last row
  }
  // *pixelVx is incremented by zero in the last column
  // *pixelVy is incremented by zero in the last row
}


/// Truncate all vectors greater than unit length to unit length.
/// \retval vec The input vector field, modified in place (x,y in planes 0,1).
template <typename T>
void
truncate_vectors(vil_image_view<T>& vec)
{
  unsigned ni = vec.ni(), nj = vec.nj();
  assert(vec.nplanes() == 2);

  std::ptrdiff_t istep=vec.istep(),  jstep=vec.jstep(),  pstep=vec.planestep();

  T* row = vec.top_left_ptr();
  for (unsigned j=0; j<nj; ++j, row+=jstep)
  {
    T* pixelX = row;
    T* pixelY = row + pstep;
    for (unsigned i=0; i<ni; ++i, pixelX+=istep, pixelY+=istep)
    {
      T mag = std::sqrt((*pixelX)*(*pixelX) + (*pixelY)*(*pixelY));
      if (mag > 1.0)
      {
        *pixelX /= mag;
        *pixelY /= mag;
      }
    }
  }
}


/// Truncate all vectors greater in length than \a weight to a magnitude of \a weight.
/// \param weight is a image of weights in [0,1] and varies at each pixel location.
/// \retval vec The input vector field, modified in place (x,y in planes 0,1).
template <typename T>
void
truncate_vectors(vil_image_view<T>& vec,
                 const vil_image_view<T>& weights)
{
  unsigned ni = vec.ni(), nj = vec.nj();
  assert(vec.nplanes() == 2);
  assert(weights.ni() == ni);
  assert(weights.nj() == nj);
  assert(weights.nplanes() == 1);

  std::ptrdiff_t istepV=vec.istep(),      jstepV=vec.jstep(),     pstepV=vec.planestep();
  std::ptrdiff_t istepW=weights.istep(),  jstepW=weights.jstep();

  T* rowV = vec.top_left_ptr();
  const T* rowW = weights.top_left_ptr();
  for (unsigned j=0; j<nj; ++j, rowV+=jstepV, rowW+=jstepW)
  {
    const T* pixelW = rowW;
    T* pixelX = rowV;
    T* pixelY = rowV + pstepV;
    for (unsigned i=0; i<ni; ++i, pixelX+=istepV, pixelY+=istepV, pixelW+=istepW)
    {
      const T mag = std::sqrt((*pixelX)*(*pixelX) + (*pixelY)*(*pixelY));
      if (mag > *pixelW)
      {
        const T s = *pixelW / mag;
        *pixelX *= s;
        *pixelY *= s;
      }
    }
  }
}


#define INSTANTIATE_ROF_DENOISE(T)                           \
template                                                     \
void                                                         \
dual_rof_denoise(const vil_image_view<T >& src,              \
                 vil_image_view<T >& dest,                   \
                 unsigned iterations,                        \
                 T theta, T step);                           \
template                                                     \
void                                                         \
dual_rof_denoise(const vil_image_view<T >& src,              \
                 vil_image_view<T >& dest,                   \
                 vil_image_view<T >& dual,                   \
                 unsigned iterations,                        \
                 T theta, T step);                           \
template                                                     \
void                                                         \
dual_rof_weighted_denoise(const vil_image_view<T >& src,     \
                          const vil_image_view<T >& weights, \
                          vil_image_view<T >& dest,          \
                          unsigned iterations,               \
                          T theta, T step)

INSTANTIATE_ROF_DENOISE(float);
INSTANTIATE_ROF_DENOISE(double);

}
