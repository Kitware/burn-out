/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "vil_image_resource_sptr_io.h"

#include <vsl/vsl_binary_io.h>

#include <vil/io/vil_io_image_view.h>
#include <vil/vil_new.h>

void write_img_resource_b( vsl_b_ostream& s, vil_image_resource_sptr const & v )
{
  vsl_b_write( s, 1 ); //version number;
  if(v != NULL && v->get_view() != NULL)
  {
    vsl_b_write( s, true );
    vsl_b_write( s, v->get_view() );
  }
  else
  {
    vsl_b_write( s, false );
  }
}

void read_img_resource_b( vsl_b_istream& s, vil_image_resource_sptr & v )
{
  int ver = -1;
  vsl_b_read( s, ver );
  if(ver != 1)
  {
    s.is().setstate( s.is().failbit );
    return;
  }
  bool has_img_data = false;
  v = NULL;
  vsl_b_read( s, has_img_data);
  if(has_img_data)
  {
    vil_image_view_base_sptr img;
    vsl_b_read( s, img );
    v = vil_new_image_resource_of_view( *img );
  }
}
