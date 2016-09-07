/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
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
#include <vgl/io/vgl_io_box_2d.hxx> // to implicitly instantiate for unsigned

#include <utilities/log.h>

#include <tracking/tracking_keys.h>


void
vsl_b_write( vsl_b_ostream& s, vidtk::image_object const& obj )
{
  vsl_b_write( s, 3 ); // version number

  vsl_b_write( s, obj.boundary_ );
  vsl_b_write( s, obj.bbox_ );
  vsl_b_write( s, obj.img_loc_ );
  vsl_b_write( s, obj.world_loc_ );
  vsl_b_write( s, obj.area_ );
  vsl_b_write( s, obj.mask_i0_ );
  vsl_b_write( s, obj.mask_j0_ );
  vsl_b_write( s, obj.mask_ );

  // For the moment, we don't write out the property map.  The
  // property map can contain any data, and so it's not clear how to
  // serialize it out properly.  For the moment, we punt and don't
  // write it out.
/*  if( ! (obj.data_.size() == 2) )
  {
    log_error( "attempt to serialize image_object with properties. The properties will not be written out.\n" );
  }*/
  vil_image_view<vxl_byte> data; //NOTE: might want to template
  if( obj.data_.get(vidtk::tracking_keys::pixel_data, data))
  {
    vsl_b_write( s, true );
    vsl_b_write( s, data );
    unsigned int buffer = static_cast<unsigned int>(-1);
    if( obj.data_.get(vidtk::tracking_keys::pixel_data_buffer, buffer) )
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
//   vcl_cout << ver << "  " << (ver != 1 && ver != 2) << vcl_endl;

  if( ver != 1 && ver != 2 && ver != 3 )
  {
    // unknown version number.
    s.is().setstate( s.is().failbit );
    return;
  }

  vsl_b_read( s, obj.boundary_ );
  vsl_b_read( s, obj.bbox_ );
  vsl_b_read( s, obj.img_loc_ );
  vsl_b_read( s, obj.world_loc_ );
  vsl_b_read( s, obj.area_ );
  vsl_b_read( s, obj.mask_i0_ );
  vsl_b_read( s, obj.mask_j0_ );
  vsl_b_read( s, obj.mask_ );

  if(ver == 2 || ver == 3)
  {
    bool has_img_data;
    vsl_b_read( s, has_img_data);
    if(has_img_data)
    {
      vil_image_view<vxl_byte> data;
      vsl_b_read(s, data);
      obj.data_.set(vidtk::tracking_keys::pixel_data,data);
      if(ver == 2)
      {
        unsigned int buffer;
        vsl_b_read( s, buffer );
        obj.data_.set(vidtk::tracking_keys::pixel_data_buffer, buffer);
      }
      else if(ver == 3)
      {
        bool has_buf_data;
        vsl_b_read( s, has_buf_data );
        if( has_buf_data )
        {
          unsigned int buffer;
          vsl_b_read( s, buffer );
          obj.data_.set(vidtk::tracking_keys::pixel_data_buffer, buffer);
        }
      }
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
