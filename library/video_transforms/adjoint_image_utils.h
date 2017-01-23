/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef adjoint_image_utils_h_
#define adjoint_image_utils_h_

#include <vil/vil_image_view.h>
#include <vil/vil_math.h>
#include <cstdlib>

namespace vidtk
{

/// fill an image with random values between min_v and max_v
template <typename T>
void fill_random(vil_image_view<T>& img, T min_v, T max_v)
{
  const T scale = max_v - min_v;
  for (unsigned p=0; p < img.nplanes(); ++p)
  {
    for (unsigned j=0; j < img.nj(); ++j)
    {
      for (unsigned i=0; i < img.ni(); ++i)
      {
        img(i,j,p) = static_cast<T>(std::rand()) / RAND_MAX * scale + min_v;
      }
    }
  }
}


/// compute the 2-norm of an image
template <typename T>
double image_norm2(const vil_image_view<T>& img)
{
  double sum = 0.0;
  for (unsigned p=0; p < img.nplanes(); ++p)
  {
    for (unsigned j=0; j < img.nj(); ++j)
    {
      for (unsigned i=0; i < img.ni(); ++i)
      {
        const T& val = img(i,j,p);
        sum += val * val;
      }
    }
  }
  return std::sqrt(sum);
}


/// compute the dot product between two vil_image_views
template <typename T>
double dot_product(const vil_image_view<T>& img1,
                   const vil_image_view<T>& img2)
{
  assert(img1.ni() == img2.ni());
  assert(img1.nj() == img2.nj());
  assert(img1.nplanes() == img2.nplanes());
  const unsigned np = img1.nplanes();

  vil_image_view<double> tmp;
  vil_math_image_product(img1, img2, tmp);
  double sum = 0.0;
  for (unsigned p=0; p<np; ++p)
  {
    double s = 0;
    vil_math_sum(s, tmp, p);
    sum += s;
  }
  return sum;
}

} // end namespace vidtk

#endif //adjoint_image_utils_h_
