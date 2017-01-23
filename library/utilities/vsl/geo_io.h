/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_geo_io_h_
#define vidtk_geo_io_h_

#include <vsl/vsl_fwd.h>

///VSL IO for geo_UTM
#include <utilities/geo_UTM.h>

void vsl_b_write( vsl_b_ostream& os, const vidtk::geo_coord::geo_UTM &utm );
void vsl_b_read( vsl_b_istream& is, vidtk::geo_coord::geo_UTM &utm );

#endif // vidtk_geo_io_h_
