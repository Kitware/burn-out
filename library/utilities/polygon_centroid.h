/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_polygon_centroid_h_
#define vidtk_polygon_centroid_h_

#include <vgl/vgl_polygon.h>
#include <vnl/vnl_vector_fixed.h>

namespace vidtk
{

/// Compute a centroid for the vgl polygon.
vnl_vector_fixed< double, 2 >
polygon_centroid( vgl_polygon< double > const& poly );


} // end namespace vidtk

#endif // vidtk_polygon_centroid_h_
