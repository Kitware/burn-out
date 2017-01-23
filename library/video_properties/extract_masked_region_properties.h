/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_extract_masked_region_properties_h_
#define vidtk_extract_masked_region_properties_h_

#include <vil/vil_image_view.h>

namespace vidtk
{

/// \brief Returns the average values over some masked region.
///
/// \param input The input image.
/// \param mask The input mask.
template< typename PixType >
PixType compute_average( const vil_image_view< PixType >& input,
                         const vil_image_view< bool >& mask );

/// \brief Returns the minimum values over some masked region.
///
/// \param input The input image.
/// \param mask The input mask.
template< typename PixType >
PixType compute_minimum( const vil_image_view< PixType >& input,
                         const vil_image_view< bool >& mask );

}

#endif // vidtk_extract_masked_region_properties_h_
