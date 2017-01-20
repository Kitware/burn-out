/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_object_io_h_
#define vidtk_image_object_io_h_

/// \file
///
/// \brief Binary IO functions for vidtk::image_object.

#include <vsl/vsl_binary_io.h>
#include <tracking_data/image_object.h>


void vsl_b_write( vsl_b_ostream& s, vidtk::image_object const& o );

void vsl_b_read( vsl_b_istream& s, vidtk::image_object& o );

void vsl_b_write( vsl_b_ostream& os, vidtk::image_object const* p );

void vsl_b_read( vsl_b_istream& is, vidtk::image_object*& p );


#endif // vidtk_image_object_io_h_
