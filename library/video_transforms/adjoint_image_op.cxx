/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "adjoint_image_op.h"
#include "adjoint_image_utils.h"
#include <vil/vil_math.h>


namespace vidtk
{

/// Apply the primary followed by adjoint operators
template <typename T>
void
adjoint_image_op<T>
::apply_AtA(const vil_image_view<T>& src,
            vil_image_view<T>& dst) const
{
  vil_image_view<T> tmp(this->dst_ni(), this->dst_nj(), this->dst_nplanes());
  this->apply_A(src, tmp);
  this->apply_At(tmp, dst);
}


/// Estimate the 2-norm of image operator \p op.
/// The algorithm uses a power series to estimate the 2-norm.
template <typename T>
double
adjoint_image_op<T>
::norm_estimation(double tol) const
{
  vil_image_view<T> vec(this->src_ni(), this->src_nj(), this->src_nplanes());
  fill_random(vec, T(0), T(1));
  return this->norm_estimation(vec, tol);
}


/// Estimate the 2-norm of image operator \p op.
/// The algorithm uses a power series to estimate the 2-norm.
template <typename T>
double
adjoint_image_op<T>
::norm_estimation(vil_image_view<T>& vec, double tol) const
{
  // start with a normalized input image
  vil_math_scale_values(vec, 1.0 / image_norm2(vec));

  vil_image_view<T> tmp;
  double e0 = 1.0,  e1 = 1.0;
  do
  {
    e0 = e1;
    this->apply_AtA(vec, tmp);
    std::swap(vec,tmp);
    const double mag = image_norm2(vec);
    vil_math_scale_values(vec, 1.0 / mag);
    e1 = std::sqrt(mag);
  }
  while(std::abs(e1 - e0) > tol * e1);

  return e1;
}


/// Verify that the operators are in fact adjoint.
/// Compares "vec2 A^t A vec1" to "vec1 A^t A vec2" for
/// random images vec1 and vec2
template <typename T>
bool
adjoint_image_op<T>
::is_adjoint(double tol) const
{
  vil_image_view<T> vec1(this->src_ni(), this->src_nj(), this->src_nplanes());
  vil_image_view<T> vec2(this->src_ni(), this->src_nj(), this->src_nplanes());
  fill_random(vec1, T(0), T(1));
  fill_random(vec2, T(0), T(1));
  return this->is_adjoint(vec1, vec2, tol);
}


/// Verify that the operators are in fact adjoint.
/// Compares "vec2 A^t A vec1" to "vec1 A^t A vec2" for
/// specified images vec1 and vec2
template <typename T>
bool
adjoint_image_op<T>
::is_adjoint(const vil_image_view<T>& vec1,
                const vil_image_view<T>& vec2,
                double tol) const
{
  vil_image_view<T> tmp1, tmp2;
  this->apply_AtA(vec1, tmp1);
  this->apply_AtA(vec2, tmp2);

  double sum1 = dot_product(vec1, tmp2);
  double sum2 = dot_product(vec2, tmp1);
  double relative_error = std::abs(sum1 - sum2) / std::abs(sum1);

  return relative_error < tol;
}

//Construct a multi image op from adjoint image ops
template <typename T>
adjoint_multi_image_op<T>
::adjoint_multi_image_op(const std::vector<adjoint_image_op<T>* >& image_ops)
  : image_ops_(image_ops)
{
  assert(image_ops_.size() > 0);
  assert(image_ops_[0]);
  for (unsigned int i=1; i<image_ops_.size(); ++i)
  {
    assert(image_ops_[i]);
    assert(image_ops_[i]->src_ni() == image_ops_.front()->src_ni());
    assert(image_ops_[i]->src_nj() == image_ops_.front()->src_nj());
    assert(image_ops_[i]->src_nplanes() == image_ops_.front()->src_nplanes());
  }
}


/// Apply the primary followed by adjoint operators
/// computed as the sum of apply_AtA on all sub-operations
template <typename T>
void
adjoint_multi_image_op<T>
::apply_AtA(const vil_image_view<T>& src,
            vil_image_view<T>& dst) const
{
  dst.set_size(this->src_ni(), this->src_nj(), this->src_nplanes());
  dst.fill(T(0));

  vil_image_view<T> tmp(this->src_ni(), this->src_nj(), this->src_nplanes());
  for (unsigned int i=0; i<image_ops_.size(); ++i)
  {
    image_ops_[i]->apply_AtA(src, tmp);
    vil_math_image_sum(tmp, dst, dst);
  }
}


/// The width of the image that is operated on.
template <typename T>
unsigned
adjoint_multi_image_op<T>
::src_ni() const
{
  return image_ops_.front()->src_ni();
}


/// The height of the image that is operated on.
template <typename T>
unsigned
adjoint_multi_image_op<T>
::src_nj() const
{
  return image_ops_.front()->src_nj();
}


/// The number of planes in the image that is operated on.
template <typename T>
unsigned
adjoint_multi_image_op<T>
::src_nplanes() const
{
  return image_ops_.front()->src_nplanes();
}



// class instantiations
template class adjoint_image_op<double>;
template class adjoint_image_op<float>;
template class adjoint_multi_image_op<double>;
template class adjoint_multi_image_op<float>;

} // end namespace vidtk
