/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_jittered_image_difference_h_
#define vidtk_jittered_image_difference_h_


#include <vil/vil_image_view.h>

namespace vidtk
{

/// The parameters for how the jittering is to be done.
/// \sa jittered_image_difference
struct jittered_image_difference_params
{
  typedef jittered_image_difference_params self;

  jittered_image_difference_params()
  : delta_i_( 0 ),
    delta_j_( 0 ),
    signed_diff_( false )
  {}

  /// Set the jitter to +-di in the first dimension and to +-dj in the
  /// second dimension.
  self& set_delta( unsigned di, unsigned dj )
  {
    delta_i_ = di;
    delta_j_ = dj;
    return *this;
  }

  /// Set the is_signed flag, if set a signed difference image will be
  /// returned instead of an unsigned (absolute magnitude) one.
  self& set_signed_flag( bool is_signed = true )
  {
    signed_diff_ = is_signed;
    return *this;
  }

  unsigned delta_i_;
  unsigned delta_j_;
  bool signed_diff_;
};


/// \brief Compute the image difference where one of the images is
/// slightly jittered.
///
/// Given \f$\delta i\f$ and \f$\delta j\f$, this function computes
/// \f[
///    D_{A-B}(i,j) = \min_{t \in W, s \in W} \langle|A(i,j,p)-B(i+t,j+s,p)|\rangle{p}
/// \f]
///
/// \param[in] srcA first source image
/// \param[in] srcB second source image
/// \param[out] diff difference image
/// \param[in] param jittering parameters
template < class SrcT, class AccumT >
void jittered_image_difference( vil_image_view< SrcT > const&           srcA,
                                vil_image_view< SrcT > const&           srcB,
                                vil_image_view< AccumT >&               diff,
                                jittered_image_difference_params const& param );

/// \brief Thread version of function.
///
/// This function is needed for the threads. The boost threading only
/// passes things by value, thus if we used the above function we
/// would get blank results. The function has 2 appended to it to
/// distinguish it from the above function since function points are
/// what is based into the threads.
///
/// \param[in] srcA first source image
/// \param[in] srcB second source image
/// \param[out] diff difference image
/// \param[in] param jittering parameters
template < class SrcT, class AccumT >
void jittered_image_difference2( vil_image_view< SrcT > const*            srcA,
                                 vil_image_view< SrcT > const*            srcB,
                                 vil_image_view< AccumT >*                diff,
                                 jittered_image_difference_params const*  param )
{
  jittered_image_difference( *srcA, *srcB, *diff, *param );
}




} // end namespace vidtk


#endif // vidtk_jittered_image_difference_h_
