/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_zscore_image_h_
#define vidtk_zscore_image_h_

#include <vil/vil_image_view.h>

namespace vidtk
{

//: Apply threshold such that dest(i,j,p)=true if src(i,j,p)>=t
// step by a factor of \a step in both vertical and horizontal
// when computing image statistics.
template<class srcT>
void zscore_threshold_above( const vil_image_view<srcT>& src,
                             vil_image_view<bool>& dest,  double thresh,
                             unsigned step=2 );


//: Standardize the image in place using global statistics.
//  The mean (mu) and stanndard deviation (sigma) are computed over the whole image.
//  The result is a z-score image z = (x-mu)/sigma.
//  Step by a factor of \a step in both vertical and horizontal
//  when computing image statistics.
template<class srcT>
void zscore_global( vil_image_view<srcT>& img, unsigned step=1 );


//: Standardize the image in place using local statistics over a box.
//  The mean (mu) and stanndard deviation (sigma) are computed for each pixel
//  over a box of radius \a radius centered at each pixel.
//  The result is a z-score image z = (x-mu)/sigma
template<class srcT>
void zscore_local_box( vil_image_view<srcT>& img, unsigned radius );

} // end namespace vidtk

#endif
