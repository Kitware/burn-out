/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_histogram_io_h_
#define vidtk_image_histogram_io_h_

/// \file
///
/// \brief Binary IO functions for vidtk::image_histogram.

#include <vsl/vsl_fwd.h>
#include <utilities/image_histogram.h>

void vsl_b_write( vsl_b_ostream &os, vidtk::Image_Histogram_Type t );
void vsl_b_read( vsl_b_istream &is, vidtk::Image_Histogram_Type &t );

void vsl_b_write( vsl_b_ostream& os,
                  const vidtk::image_histogram &h );

void vsl_b_read( vsl_b_istream& is,
                 vidtk::image_histogram &h );

#endif // vidtk_image_histogram_io_h_
