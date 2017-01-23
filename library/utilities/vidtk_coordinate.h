/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef __vidtk_coordinate_h__
#define __vidtk_coordinate_h__

#include <utilities/geo_coordinate.h>

namespace vidtk
{

///Creates name for type double.
typedef double float_type;

/// A 2-d vnl_vector of float_type.
typedef vnl_vector_fixed< float_type, 2 > vidtk_pixel_coord_type;

/// A 3-d vnl_vector of float_type.
typedef vnl_vector_fixed< float_type, 3 > tracker_world_coord_type;


class vidtk_coordinate
{


private:
  geo_coord::geo_coordinate geo_coord_;

}; // class vidtk_coordinate

} //namespace vidtk
#endif
