/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef adjoint_resample_h_
#define adjoint_resample_h_

#include <vil/vil_image_view.h>

namespace vidtk
{

/// Down sample an image by selecting a subset of pixels.
/// No averaging or bluring of pixels is done.
/// \note This function is adjoint to \c up_sample
/// \param input The input image.
/// \param output The down sampled output image.
/// \param scale The down sampling factor (number of pixels to skip).
/// \param offset_x The number of pixels to shift in the horizontal direction
///                 in the input image before sampling (less than \p scale).
/// \param offset_y The number of pixels to shift in the vertical direction.
///                 in the input image before sampling (less than \p scale).
template <typename T>
void down_sample(const vil_image_view<T> &input,
                 vil_image_view<T> &output,
                 unsigned scale = 2,
                 unsigned offset_x = 0,
                 unsigned offset_y = 0)
{
  assert(offset_x < scale);
  assert(offset_y < scale);
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  const unsigned sni = ni / scale;
  const unsigned snj = nj / scale;

  output.set_size(sni, snj, np);

  for (unsigned int p = 0; p < np; ++p)
  {
    for (unsigned int j=0, n=offset_y; j<snj; ++j, n+=scale)
    {
      n = (n < nj) ? n : nj - 1;
      for (unsigned int i=0, m=offset_x; i<sni; ++i, m+=scale)
      {
        m = (m < ni) ? m : ni - 1;
        output(i,j,p) = input(m,n,p);
      }
    }
  }
}


/// Up sample an image by distributing pixels over a larger grid.
/// No averaging or interpolating of pixels is done.
/// Between the filled in pixels, all values will be zero (black).
/// \note This function is adjoint to \c down_sample
/// \param input The input image.
/// \param output The up sampled output image.  If the output is already
///               a size that could have been down sampled by \p scale to
///               produce \p input, the size will be kept.  Otherwise, the
///               size is set to \p input size times \p scale.
/// \param scale The up sampling factor (number of pixels to skip).
/// \param offset_x The number of pixels to shift in the horizontal direction
///                 when inserting pixels in the output (less than \p scale).
/// \param offset_x The number of pixels to shift in the vertical direction.
///                 when inserting pixels in the output (less than \p scale).
template <typename T>
void up_sample(const vil_image_view<T> &input,
               vil_image_view<T> &output,
               unsigned scale = 2,
               unsigned offset_x = 0,
               unsigned offset_y = 0)
{
  assert(offset_x < scale);
  assert(offset_y < scale);
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  unsigned sni_min = ni * scale;
  unsigned snj_min = nj * scale;
  unsigned sni_max = (ni + 1) * scale - 1;
  unsigned snj_max = (nj + 1) * scale - 1;
  if ( output.ni() < sni_min ||
       output.ni() > sni_max ||
       output.nj() < snj_min ||
       output.nj() > snj_max ||
       output.nplanes() != np )
  {
    output.set_size(sni_min, snj_min, np);
  }
  const unsigned sni = output.ni();
  const unsigned snj = output.nj();
  output.fill(T(0));

  for (unsigned int p = 0; p < np; ++p)
  {
    for (unsigned int j=0, n=offset_y; j<nj; ++j, n+=scale)
    {
      n = (n < snj) ? n : snj - 1;
      for (unsigned int i=0, m=offset_x; i<ni; ++i, m+=scale)
      {
        m = (m < sni) ? m : sni - 1;
        output(m,n,p) = input(i,j,p);
      }
    }
  }
}


/// Down scale an image by averaging all pixels in each \p scale square block.
/// No bluring of pixels between blocks is done
/// \note This function is adjoint to \c up_scale
/// \param input The input image.
/// \param output The down scaled output image.
/// \param scale The down scaling factor (number of pixels to step).
template <typename T>
void down_scale(const vil_image_view<T> &input,
                vil_image_view<T> &output,
                unsigned scale = 2)
{
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  const unsigned sni = ni / scale;
  const unsigned snj = nj / scale;

  output.set_size(sni, snj, np);

  for (unsigned int p = 0; p < np; ++p)
  {
    for (unsigned int j=0, ns=0, ne=scale; j<snj; ++j, ns=ne, ne+=scale)
    {
      ne = (ne < nj) ? ne : nj;
      for (unsigned int i=0, ms=0, me=scale; i<sni; ++i, ms=me, me+=scale)
      {
        me = (me < ni) ? me : ni;
        double sum = 0.0;
        for (unsigned n=ns; n<ne; ++n)
        {
          for (unsigned m=ms; m<me; ++m)
          {
            sum += input(m,n,p);
          }
        }
        output(i,j,p) = sum / ((me-ms)*(ne-ns));
      }
    }
  }
}


/// Up scale an image by weighted distribution of pixels over a larger grid.
/// Pixels are distributed over a \p scale square block with uniform weight.
/// The total "energy" of image is preserved, so this image will be darker.
/// \note This function is adjoint to \c down_scale
/// \param input The input image.
/// \param output The up scaled output image.  If the output is already
///               a size that could have been down scaled by \p scale to
///               produce \p input, the size will be kept.  Otherwise, the
///               size is set to \p input size times \p scale.
/// \param scale The down scaling factor (number of pixels to step).
template <typename T>
void up_scale(const vil_image_view<T> &input,
              vil_image_view<T> &output,
              unsigned scale = 2)
{
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  unsigned sni_min = ni * scale;
  unsigned snj_min = nj * scale;
  unsigned sni_max = (ni + 1) * scale - 1;
  unsigned snj_max = (nj + 1) * scale - 1;
  if ( output.ni() < sni_min ||
       output.ni() > sni_max ||
       output.nj() < snj_min ||
       output.nj() > snj_max ||
       output.nplanes() != np )
  {
    output.set_size(sni_min, snj_min, np);
  }
  const unsigned sni = output.ni();
  const unsigned snj = output.nj();

  for (unsigned int p = 0; p < np; ++p)
  {
    for (unsigned int j=0, ns=0, ne=scale; j<nj; ++j, ns=ne, ne+=scale)
    {
      ne = (ne < snj) ? ne : snj;
      for (unsigned int i=0, ms=0, me=scale; i<ni; ++i, ms=me, me+=scale)
      {
        me = (me < sni) ? me : sni;
        const T val = static_cast<T>( static_cast<double>(input(i,j,p))
                                      / ((me-ms)*(ne-ns)) );
        for (unsigned n=ns; n<ne; ++n)
        {
          for (unsigned m=ms; m<me; ++m)
          {
            output(m,n,p) = val;
          }
        }
      }
    }
  }
}

} // end namespace vidtk

#endif //adjoint_resample_h_
