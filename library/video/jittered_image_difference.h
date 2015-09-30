/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_jittered_image_difference_h_
#define vidtk_jittered_image_difference_h_

/// \file

#include <vil/vil_image_view.h>

namespace vidtk
{

/// The parameters for how the jittering is to be done.
/// \sa jittered_image_difference
struct jittered_image_difference_params
{
  typedef jittered_image_difference_params self;

  /// Set the jitter to +-di in the first dimension and to +-dj in the
  /// second dimension.
  self& set_delta( unsigned di, unsigned dj )
  {
    delta_i_ = di;
    delta_j_ = dj;
    return *this;
  }

  unsigned delta_i_;
  unsigned delta_j_;
};


/// \brief Compute the image difference where one of the images is
/// slightly jittered.
///
/// Given \f$\delta i\f$ and \f$\delta j\f$, this function computes
/// \f[
///    D_{A-B}(i,j) = \min_{t \in W, s \in W} \mean_{p} |A(i,j,p)-B(i+t,j+s,p)|
/// \f]
template<class SrcT, class AccumT>
void jittered_image_difference(
  vil_image_view<SrcT> const& srcA,
  vil_image_view<SrcT> const& srcB,
  vil_image_view<AccumT>& diff,
  jittered_image_difference_params const& param );


} // end namespace vidtk


#endif // vidtk_jittered_image_difference_h_
