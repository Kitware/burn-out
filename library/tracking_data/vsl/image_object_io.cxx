/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_io.h"


#include <vsl/vsl_binary_io.h>

#include <vil/io/vil_io_image_view.h>
#include <vnl/io/vnl_io_vector_fixed.h>
#include <vnl/io/vnl_io_vector_fixed.h>
#include <vgl/io/vgl_io_polygon.h>
#include <vgl/io/vgl_io_box_2d.h>
#include <vgl/io/vgl_io_box_2d.txx> // to implicitly instantiate for unsigned

#include <tracking_data/tracking_keys.h>
#include <tracking_data/image_object.h>

#include <tracking_data/vsl/vil_image_resource_sptr_io.h>
#include <vil/vil_new.h>

void
vsl_b_write( vsl_b_ostream& s, vidtk::image_object const& obj )
{
  vsl_b_write( s, 4 ); // version number

  vil_image_view< bool > mask;
  vidtk::image_object::image_point_type origin;
  obj.get_object_mask( mask, origin );

  vsl_b_write( s, obj.get_boundary() );
  vsl_b_write( s, obj.get_bbox() );
  vsl_b_write( s, obj.get_image_loc() );
  vsl_b_write( s, obj.get_world_loc() );
  vsl_b_write( s, obj.get_area() );
  vsl_b_write( s, origin.x() );
  vsl_b_write( s, origin.y() );
  vsl_b_write( s, mask );

  // For the moment, we don't write out the property map.  The
  // property map can contain any data, and so it's not clear how to
  // serialize it out properly.  For the moment, we punt and don't
  // write it out.
  vil_image_resource_sptr data;
  unsigned int buffer = static_cast<unsigned int>(-1);

  if( obj.get_image_chip( data, buffer) )
  {
    vsl_b_write( s, true );
    write_img_resource_b( s, data );
    if( buffer > static_cast<unsigned int>(-1) )
    {
      vsl_b_write( s, true );
      vsl_b_write( s, buffer );
    }
    else
    {
      vsl_b_write( s, false );
    }
  }
  else
  {
    vsl_b_write( s, false );
  }
}


void
vsl_b_read( vsl_b_istream& s, vidtk::image_object& obj )
{
  int ver = -1;
  vsl_b_read( s, ver );
//   LOG_INFO( ver << "  " << (ver != 1 && ver != 2) );

  if( ver != 1 && ver != 2 && ver != 3 && ver != 4)
  {
    // unknown version number.
    s.is().setstate( s.is().failbit );
    return;
  }

  vgl_polygon< vidtk::image_object::float_type > boundary;
  vgl_box_2d< unsigned > bbox;
  vidtk::vidtk_pixel_coord_type img_loc;
  vidtk::tracker_world_coord_type world_loc;
  vidtk::image_object::float_type area;

  vidtk::image_object::image_point_type origin;
  vil_image_view< bool > mask;

  vsl_b_read( s, boundary );
  vsl_b_read( s, bbox );
  vsl_b_read( s, img_loc );
  vsl_b_read( s, world_loc );
  vsl_b_read( s, area );
  vsl_b_read( s, origin.x() );
  vsl_b_read( s, origin.y() );
  vsl_b_read( s, mask );

  obj.set_boundary( boundary );
  obj.set_bbox( bbox );
  obj.set_image_loc( img_loc );
  obj.set_world_loc( world_loc );
  obj.set_area( area );
  obj.set_object_mask( mask, origin );

  if(ver == 2 || ver == 3)
  {
    bool has_img_data;
    vsl_b_read( s, has_img_data);
    if(has_img_data)
    {
      vil_image_view<vxl_byte> tmp;
      vsl_b_read(s, tmp);
      vil_image_resource_sptr chip = vil_new_image_resource_of_view(tmp);
      unsigned int buffer = -1;
      if(ver == 2)
      {
        vsl_b_read( s, buffer );
      }
      else if(ver == 3)
      {
        bool has_buf_data;
        vsl_b_read( s, has_buf_data );
        if( has_buf_data )
        {
          vsl_b_read( s, buffer );
        }
      }

      obj.set_image_chip( chip, buffer );
    }
  }
  else if(ver==4)
  {
    bool has_img_data;
    vsl_b_read( s, has_img_data);
    if(has_img_data)
    {
      vil_image_resource_sptr chip;
      read_img_resource_b(s, chip);
      bool has_buf_data;
      vsl_b_read( s, has_buf_data );
      unsigned int buffer = -1;
      if( has_buf_data )
      {
        vsl_b_read( s, buffer );
      }

      obj.set_image_chip( chip, buffer );
    }
  }
}


void vsl_b_write( vsl_b_ostream& os, vidtk::image_object const* p )
{
  if( p == 0 )
  {
    vsl_b_write(os, false); // Indicate null pointer stored
  }
  else
  {
    vsl_b_write( os, true ); // Indicate non-null pointer stored
    vsl_b_write( os, *p );
  }
}


void vsl_b_read( vsl_b_istream& is, vidtk::image_object*& p )
{
  delete p;
  bool not_null_ptr;
  vsl_b_read( is, not_null_ptr );
  if( not_null_ptr )
  {
    p = new vidtk::image_object;
    vsl_b_read( is, *p );
  }
  else
  {
    p = 0;
  }
}
