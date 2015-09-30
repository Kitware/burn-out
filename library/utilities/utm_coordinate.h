/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_utm_h_
#define vidtk_utm_h_

#include <string>

namespace vidtk
{

class utm_coordinate
{
public:
  typedef double CoordinateType;
  typedef std::string   ZoneType;

  utm_coordinate( CoordinateType n, CoordinateType e, ZoneType z )
  {
    northing_ = n;
    easting_ = e;
    zone_ = z;
  }

  std::string to_string() const;

  //based on http://www.uwgb.edu/dutchs/UsefulData/UTMFormulas.htm
  // and http://www.uwgb.edu/dutchs/UsefulData/UTMConversions1.xls
  void convert_to_lat_long( double & lat, double & lon );

  CoordinateType northing_;
  CoordinateType easting_;
  ZoneType       zone_;

};

}


#endif // vidtk_utm_h_

