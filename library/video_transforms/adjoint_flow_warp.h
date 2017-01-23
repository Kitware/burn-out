/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef adjoint_flow_warp_h_
#define adjoint_flow_warp_h_

#include <vil/vil_image_view.h>
#include <vil/vil_bilin_interp.h>
#include <vil/vil_bicub_interp.h>
#include <vnl/vnl_math.h>


namespace vidtk
{


/// Warp the target image to the reference using an optical flow field.
/// If \p flow was computed from I0 to I1 then setting \p input to I1
/// would warp I1 back to reference image I0.
/// \param input is the target image (destination of the flow vectors)
/// \param flow is the dense optical flow field (2-planes for x and y)
/// \param warped is the resulting warped image.
/// \param interp is an interpolator function/functor with the signature
///        \code
///        double interp(const vil_image_view<srcT>&, double, double, double)
///        \endcode
template <typename srcT, typename flowT, typename destT, typename InterpFunc>
void warp_back_with_flow( const vil_image_view<srcT> &input,
                          const vil_image_view<flowT> &flow,
                          vil_image_view<destT> &warped,
                          InterpFunc interp)
{
  const unsigned ni = flow.ni();
  const unsigned nj = flow.nj();
  const unsigned np = input.nplanes();
  assert(flow.nplanes() == 2);
  warped.set_size(ni, nj, np);

  for (unsigned int j = 0; j < nj; ++j)
  {
    for (unsigned int i = 0; i < ni; ++i)
    {
      if (vnl_math::isnan(flow(i,j,0)) || vnl_math::isnan(flow(i,j,1)))
      {
        for (unsigned int p = 0; p < np; ++p)
        {
          warped(i, j, p) = destT(0);
        }
        continue;
      }

      const double x = static_cast<double>(i) + flow(i,j,0);
      const double y = static_cast<double>(j) + flow(i,j,1);
      for (unsigned int p = 0; p < np; ++p)
      {
        warped(i, j, p) = static_cast<destT>(interp(input, x, y, p));
      }
    }
  }
}


/// Warp the target image to the reference using an optical flow field.
/// If \p flow was computed from I0 to I1 then setting \p input to I1
/// would warp I1 back to reference image I0.
/// \note this version is hardcoded to bilinear interpolation
/// \param input is the target image (destination of the flow vectors)
/// \param flow is the dense optical flow field (2-planes for x and y)
/// \param warped is the resulting warped image.
template <typename srcT, typename flowT, typename destT>
void warp_back_with_flow_bilin( const vil_image_view<srcT> &input,
                                const vil_image_view<flowT> &flow,
                                vil_image_view<destT> &warped)
{
  typedef double (*func_t)(const vil_image_view<srcT>&,
                           double, double, unsigned int);
  warp_back_with_flow(input, flow, warped,
                      func_t(vil_bilin_interp_safe));
}


/// Warp the target image to the reference using an optical flow field.
/// If \p flow was computed from I0 to I1 then setting \p input to I1
/// would warp I1 back to reference image I0.
/// \note this version is hardcoded to bicubic interpolation
/// \param input is the target image (destination of the flow vectors)
/// \param flow is the dense optical flow field (2-planes for x and y)
/// \param warped is the resulting warped image.
template <typename srcT, typename flowT, typename destT>
void warp_back_with_flow_bicub( const vil_image_view<srcT> &input,
                                const vil_image_view<flowT> &flow,
                                vil_image_view<destT> &warped)
{
  typedef double (*func_t)(const vil_image_view<srcT>&,
                           double, double, unsigned int);
  warp_back_with_flow(input, flow, warped,
                      func_t(vil_bicub_interp_safe));
}


/// Warp the reference image to the target using an optical flow field.
/// If \p flow was computed from I0 to I1 then setting \p input to I0
/// would warp I0 to target image I1.
/// \note for now, this is hardcoded to bilinear interpolation
/// \param input is the reference image (source of the flow vectors).
/// \param flow is the dense optical flow field (2-planes for x and y)
/// \param warped is the resulting warped image,
///               should be pre-allocated to the requested size.
template <typename srcT, typename flowT, typename destT>
void warp_forward_with_flow_bilin( const vil_image_view<srcT> &input,
                                   const vil_image_view<flowT> &flow,
                                   vil_image_view<destT> &warped )
{
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  const unsigned wni = warped.ni();
  const unsigned wnj = warped.nj();
  assert(flow.ni() == ni);
  assert(flow.nj() == nj);
  assert(flow.nplanes() == 2);
  assert(warped.ni() > 0);
  assert(warped.nj() > 0);
  assert(warped.nplanes() == np);
  warped.fill(destT(0));

  for (unsigned int j = 0; j < nj; ++j)
  {
    for (unsigned int i = 0; i < ni; ++i)
    {
      const double x = static_cast<double>(i) + flow(i,j,0);
      const double y = static_cast<double>(j) + flow(i,j,1);
      // Note: This check must also work for NaN values for example,
      // !(x>=0 && y>=0) works while (x<0 || y<0) fails for the NaN case
      if ( !(x>=0 && y>=0 && x<=wni-1 && y<=wnj-1) )
      {
        continue;
      }
      const int ix = static_cast<int>(x);
      const int iy = static_cast<int>(y);
      const double x1 = x - ix;
      const double y1 = y - iy;

      // special boundary cases can be handled more quickly first;
      // also avoids accessing an invalid pix1[t] which is going to have weight 0.
      if (x1 == 0.0 && y1 == 0.0)
      {
        for (unsigned int p = 0; p < np; ++p)
        {
          warped(ix, iy, p) += input(i,j,p);;
        }
        continue;
      }
      if (x1 == 0.0)
      {
        const double y0 = 1.0 - y1;
        for (unsigned int p = 0; p < np; ++p)
        {
          const srcT& val = input(i,j,p);
          warped(ix,   iy,   p) += y0 * val;
          warped(ix,   iy+1, p) += y1 * val;
        }
        continue;
      }
      if (y1 == 0.0)
      {
        const double x0 = 1.0 - x1;
        for (unsigned int p = 0; p < np; ++p)
        {
          const srcT& val = input(i,j,p);
          warped(ix,   iy,   p) += x0 * val;
          warped(ix+1, iy,   p) += x1 * val;
        }
        continue;
      }
      const double x0 = 1.0 - x1;
      const double y0 = 1.0 - y1;
      const double w00 = x0 * y0;
      const double w01 = x0 * y1;
      const double w10 = x1 * y0;
      const double w11 = x1 * y1;
      for (unsigned int p = 0; p < np; ++p)
      {
        const srcT& val = input(i,j,p);
        warped(ix,   iy,   p) += w00 * val;
        warped(ix,   iy+1, p) += w01 * val;
        warped(ix+1, iy,   p) += w10 * val;
        warped(ix+1, iy+1, p) += w11 * val;
      }
    }
  }
}




/// Warp the reference image to the target using an optical flow field.
/// If \p flow was computed from I0 to I1 then setting \p input to I0
/// would warp I0 to target image I1.
/// \note Splat using bicubic interpolation coefficients.
/// \param input is the reference image (source of the flow vectors).
/// \param flow is the dense optical flow field (2-planes for x and y)
/// \param warped is the resulting warped image,
///               should be pre-allocated to the requested size.
template <typename srcT, typename flowT, typename destT>
void warp_forward_with_flow_bicub( const vil_image_view<srcT> &input,
                                   const vil_image_view<flowT> &flow,
                                   vil_image_view<destT> &warped )
{
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  const unsigned wni = warped.ni();
  const unsigned wnj = warped.nj();
  assert(flow.ni() == ni);
  assert(flow.nj() == nj);
  assert(flow.nplanes() == 2);
  assert(warped.ni() > 0);
  assert(warped.nj() > 0);
  assert(warped.nplanes() == np);
  warped.fill(destT(0));

  // memory offsets for each of the 16 pixels in a 4x4 neighborhood
  const std::ptrdiff_t istep = warped.istep();
  const std::ptrdiff_t jstep = warped.jstep();
  const std::ptrdiff_t offset[] =
    { -jstep - istep,  -jstep,  -jstep + istep,  -jstep + 2*istep,
              -istep,       0,           istep,           2*istep,
       jstep - istep,   jstep,   jstep + istep,   jstep + 2*istep,
     2*jstep - istep, 2*jstep, 2*jstep + istep, 2*jstep + 2*istep};

  for (unsigned int j = 0; j < nj; ++j)
  {
    for (unsigned int i = 0; i < ni; ++i)
    {
      const double x = static_cast<double>(i) + flow(i,j,0);
      const double y = static_cast<double>(j) + flow(i,j,1);
      // Note: This check must also work for NaN values for example,
      // !(x>=1 && y>=1) works while (x<1 || y<1) fails for the NaN case
      if ( !(x>=1 && y>=1 && x<=wni-2 && y<=wnj-2) )
      {
        continue;
      }
      const int ix = static_cast<int>(x);
      const int iy = static_cast<int>(y);
      const double x1 = x - ix;
      const double y1 = y - iy;

      // special boundary cases can be handled more quickly first;
      // also avoids accessing an invalid pix1[t] which is going to have weight 0.
      if (x1 == 0.0 && y1 == 0.0)
      {
        for (unsigned int p = 0; p < np; ++p)
        {
          warped(ix, iy, p) += input(i,j,p);;
        }
        continue;
      }

      // coefficients for interpolation
      double s0=-1.0, s1=-1.0, s2=-1.0, s3=-1.0;      // in the x-direction
      double t0=-1.0, t1=-1.0, t2=-1.0, t3=-1.0;      // in the y-direction

      if (x1 != 0.0)
      {
        s0 = 0.5 * ((2-x1)*x1-1)*x1;    // -1
        s1 = 0.5 * ((3*x1-5)*x1*x1+2);  //  0
        s2 = 0.5 * ((4-3*x1)*x1+1)*x1;  // +1
        s3 = 0.5 * (x1-1)*x1*x1;        // +2
      }

      if (y1 != 0.0)
      {
        t0 = 0.5 * ((2-y1)*y1-1)*y1;    // -1
        t1 = 0.5 * ((3*y1-5)*y1*y1+2);  //  0
        t2 = 0.5 * ((4-3*y1)*y1+1)*y1;  // +1
        t3 = 0.5 * (y1-1)*y1*y1;        // +2
      }

      if (x1 == 0.0)
      {
        for (unsigned int p = 0; p < np; ++p)
        {
          const srcT& val = input(i,j,p);
          destT* out = &warped(ix, iy, p);
          *(out + offset[1])  += t0 * val;  // (0, -1)
          *(out + offset[5])  += t1 * val;  // (0,  0)
          *(out + offset[9])  += t2 * val;  // (0, +1)
          *(out + offset[13]) += t3 * val;  // (0, +2)
        }
        continue;
      }
      if (y1 == 0.0)
      {
        for (unsigned int p = 0; p < np; ++p)
        {
          const srcT& val = input(i,j,p);
          destT* out = &warped(ix, iy, p);
          *(out + offset[4]) += s0 * val;  // (-1, 0)
          *(out + offset[5]) += s1 * val;  // ( 0, 0)
          *(out + offset[6]) += s2 * val;  // (+1, 0)
          *(out + offset[7]) += s3 * val;  // (+2, 0)
        }
        continue;
      }

      // weights for each of the 16 pixels in a 4x4 neighborhood
      const double w[] ={s0*t0, s1*t0, s2*t0, s3*t0,
                         s0*t1, s1*t1, s2*t1, s3*t1,
                         s0*t2, s1*t2, s2*t2, s3*t2,
                         s0*t3, s1*t3, s2*t3, s3*t3};
      for (unsigned int p = 0; p < np; ++p)
      {
        const srcT& val = input(i,j,p);
        destT* out = &warped(ix, iy, p);
        for(unsigned int k=0; k<16; ++k)
        {
          *(out + offset[k]) += w[k] * val;
        }
      }
    }
  }
}


} // end namespace vidtk

#endif //adjoint_flow_warp_h_
