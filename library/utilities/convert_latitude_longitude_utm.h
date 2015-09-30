/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_convert_latitude_longitude_utm_h_
#define vidtk_convert_latitude_longitude_utm_h_

#include <utilities/utm_coordinate.h>
#include <vnl/vnl_vector_fixed.h>

namespace vidtk
{

class convert_latitude_longitude_utm
{
public:

  static utm_coordinate convert_latitude_longitude_to_utm( double lat, double lon );

  // Latitude, then longitude in the vector.
  static utm_coordinate convert_latitude_longitude_to_utm( vnl_vector_fixed< double, 2 >& lat_lon );


  static utm_coordinate::ZoneType utm_zone( double lat, double lon );
protected:
  static double equatorial_radius_;
  static double polar_radius_;

};

} // namespace vidtk

#endif // convert_latitude_longitude_utm_h_

