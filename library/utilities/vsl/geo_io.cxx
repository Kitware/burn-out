/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "geo_io.h"

#include <vsl/vsl_binary_io.h>

void vsl_b_write( vsl_b_ostream& os, const vidtk::geo_coord::geo_UTM &utm )
{
  vsl_b_write( os, utm.get_zone() );
  vsl_b_write( os, utm.is_north() );
  vsl_b_write( os, utm.get_easting() );
  vsl_b_write( os, utm.get_northing() );
}

void vsl_b_read( vsl_b_istream& is, vidtk::geo_coord::geo_UTM &utm )
{
  int zone;
  bool is_north;
  double easting, northing;

  vsl_b_read( is, zone );
  vsl_b_read( is, is_north );
  vsl_b_read( is, easting );
  vsl_b_read( is, northing );
  utm = vidtk::geo_coord::geo_UTM( zone, is_north, easting, northing );
}
