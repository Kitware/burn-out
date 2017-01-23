/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_io_h_
#define vidtk_track_io_h_

/// \file
///
/// \brief Binary IO functions for vidtk::track.

#include <vsl/vsl_binary_io.h>
#include <tracking_data/track.h>

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state &ts );
void vsl_b_read( vsl_b_istream& is, vidtk::track_state &ts );

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state *ts );
void vsl_b_read( vsl_b_istream& is, vidtk::track_state *&ts );

void vsl_b_write( vsl_b_ostream& os, const vidtk::amhi_data &a );
void vsl_b_read( vsl_b_istream& is, vidtk::amhi_data &a );

void vsl_b_write( vsl_b_ostream& os, const vidtk::track &t );
void vsl_b_read( vsl_b_istream& is, vidtk::track &t );

void vsl_b_write( vsl_b_ostream& os, const vidtk::track *t );
void vsl_b_read( vsl_b_istream& is, vidtk::track *&t );


#endif // vidtk_track_io_h_
