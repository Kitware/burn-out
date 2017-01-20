/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "timestamp_io.h"

#include <vsl/vsl_binary_io.h>


void
vsl_b_write( vsl_b_ostream& s, vidtk::timestamp const& t )
{
  vsl_b_write( s, 1 ); // version number

  vsl_b_write( s, t.has_time() );
  if( t.has_time() )
  {
    vsl_b_write( s, t.time() );
  }

  vsl_b_write( s, t.has_frame_number() );
  if( t.has_frame_number() )
  {
    vsl_b_write( s, t.frame_number() );
  }
}


void
vsl_b_read( vsl_b_istream& s, vidtk::timestamp& ts )
{
  int ver;
  vsl_b_read( s, ver );

  if( ver != 1 )
  {
    // unknown version number.
    s.is().setstate( s.is().failbit );
    return;
  }

  bool has_t, has_f;
  double t;
  unsigned f;

  vsl_b_read( s, has_t );
  if( has_t )
  {
    vsl_b_read( s, t );
    ts.set_time( t );
  }

  vsl_b_read( s, has_f );
  if( has_f )
  {
    vsl_b_read( s, f );
    ts.set_frame_number( f );
  }
}
