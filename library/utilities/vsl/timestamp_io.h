/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_timestamp_io_h_
#define vidtk_timestamp_io_h_

/// \file
///
/// \brief Binary IO functions for vidtk::timestamp.

#include <vsl/vsl_fwd.h>
#include <utilities/timestamp.h>

void vsl_b_write( vsl_b_ostream& s, vidtk::timestamp const& t );

void vsl_b_read( vsl_b_istream& s, vidtk::timestamp& t );


#endif // vidtk_timestamp_io_h_
