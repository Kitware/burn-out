/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "dual_rof_denoise.h"

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
                 double theta, double step)
{
  unsigned ni = src.ni(), nj = src.nj();
  assert(src.nplanes() == 1);
  dest.deep_copy(src);

  vil_image_view<double> dual(ni,nj,2);
  dual.fill(0.0);

  for (unsigned i=0; i<iterations; ++i)
  {
    add_scaled_gradient(dest,dual,step/theta);
    truncate_vectors(dual);
    add_scaled_divergence(dual,src,theta,dest);
  }

}


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
                      vil_image_view<double>& dest)
{
  unsigned ni = vec.ni(), nj = vec.nj();
  assert(src.ni()==ni && src.nj()==nj);
  assert(vec.nplanes() == 2);
  assert(src.nplanes() == 1);
  dest.set_size(ni,nj,1);


  vcl_ptrdiff_t istepV=vec.istep(),  jstepV=vec.jstep(),  pstepV=vec.planestep();
  vcl_ptrdiff_t istepS=src.istep(),  jstepS=src.jstep();
  vcl_ptrdiff_t istepD=dest.istep(), jstepD=dest.jstep();


  const double*   rowV = vec.top_left_ptr();
  const double*   rowS = src.top_left_ptr();
  double*         rowD = dest.top_left_ptr();

  // special case for first row
  const double* pixelVx = rowV;
  const double* pixelVy = pixelVx + pstepV;
  const double* pixelS = rowS;
  double*       pixelD = rowD;
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
/// \param scale The scale factor applied to the gradient vectorrs
void
add_scaled_gradient(const vil_image_view<double>& src,
                    vil_image_view<double>& vec,
                    double scale)
{
  unsigned ni = src.ni(), nj = src.nj();
  assert(vec.ni()==ni && vec.nj()==nj);
  assert(src.nplanes() == 1);
  assert(vec.nplanes() == 2);

  vcl_ptrdiff_t istepS=src.istep(),  jstepS=src.jstep();
  vcl_ptrdiff_t istepV=vec.istep(),  jstepV=vec.jstep(),  pstepV=vec.planestep();

  const double*   rowS = src.top_left_ptr();
  double*         rowV = vec.top_left_ptr();

  for (unsigned j=0; j<nj-1; ++j, rowS += jstepS, rowV += jstepV)
  {
    const double* pixelS = rowS;
    double*       pixelVx = rowV;
    double*       pixelVy = pixelVx + pstepV;
    for (unsigned i=1; i<ni-1; ++i, pixelS+=istepS, pixelVx+=istepV, pixelVy+=istepV)
    {
      *pixelVx += scale*(*(pixelS + istepS) - *pixelS);
      *pixelVy += scale*(*(pixelS + jstepS) - *pixelS);
    }
    // *pixelVx is incremented by zero in the last column
    *pixelVy += scale*(*(pixelS + jstepS) - *pixelS);
  }

  // special case for last row
  const double* pixelS = rowS;
  double*       pixelVx = rowV;
  double*       pixelVy = pixelVx + pstepV;
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
void
truncate_vectors(vil_image_view<double>& vec)
{
  unsigned ni = vec.ni(), nj = vec.nj();
  assert(vec.nplanes() == 2);

  vcl_ptrdiff_t istep=vec.istep(),  jstep=vec.jstep(),  pstep=vec.planestep();

  double* row = vec.top_left_ptr();
  for (unsigned j=0; j<nj; ++j, row+=jstep)
  {
    double* pixelX = row;
    double* pixelY = row + pstep;
    for (unsigned i=0; i<ni; ++i, pixelX+=istep, pixelY+=istep)
    {
      double mag = std::sqrt((*pixelX)*(*pixelX) + (*pixelY)*(*pixelY));
      if (mag > 1.0)
      {
        *pixelX /= mag;
        *pixelY /= mag;
      }
    }
  }
}

}
