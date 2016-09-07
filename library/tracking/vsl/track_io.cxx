/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_io.h"

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.hxx>

#include <vnl/io/vnl_io_vector_fixed.h>
#include <vnl/io/vnl_io_vector_fixed.hxx>
#include <vil/io/vil_io_image_view.h>
#include <vgl/io/vgl_io_box_2d.h>
#include <vgl/io/vgl_io_box_2d.hxx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.hxx>
#include <tracking/tracking_keys.h>
#include <tracking/pvo_probability.h>
#include <tracking/vsl/image_object_io.h>
#include <utilities/vsl/timestamp_io.h>
#include <utilities/vsl/image_histogram_io.h>
#include <utilities/vsl/image_histogram_io.txx>

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state &ts )
{
  vsl_b_write( os, 1 ); // version number 
  vsl_b_write( os, ts.time_ );
  vsl_b_write( os, ts.loc_ );
  vsl_b_write( os, ts.vel_ );
  vsl_b_write( os, ts.amhi_bbox_ );

  // The only thing contained in the propertymap is a collection of
  // image objects
  vcl_vector<vidtk::image_object_sptr> objs;
  if( ts.data_.get(vidtk::tracking_keys::img_objs, objs))
  {
    vsl_b_write( os, true );
    vsl_b_write( os, objs );
  }
  else
  {
    vsl_b_write( os, false);
  }
}

void vsl_b_read( vsl_b_istream& is, vidtk::track_state &ts )
{
  unsigned int version;
  vsl_b_read( is, version );
  vsl_b_read( is, ts.time_ );
  vsl_b_read( is, ts.loc_ );
  vsl_b_read( is, ts.vel_ );
  vsl_b_read( is, ts.amhi_bbox_ );
  
  // The only thing contained in the propertymap is a collection of
  // image objects
  bool hasObjs;
  vsl_b_read( is, hasObjs );
  if( hasObjs )
  {
    vcl_vector<vidtk::image_object_sptr> objs;
    vsl_b_read( is, objs );
    ts.data_.set(vidtk::tracking_keys::img_objs, objs);
  }
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state *ts )
{
  if (ts==0)
  {
    vsl_b_write(os, false); // Indicate null pointer stored
  }
  else
  {
    vsl_b_write(os,true); // Indicate non-null pointer stored
    vsl_b_write(os,*ts);
  }
}

void vsl_b_read( vsl_b_istream& is, vidtk::track_state *&ts )
{
  delete ts;
  bool not_null_ptr;
  vsl_b_read(is, not_null_ptr);
  if (not_null_ptr)
  {
    ts = new vidtk::track_state();
    vsl_b_read(is, *ts);
  }
  else
    ts = 0;
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::amhi_data &a )
{
  vsl_b_write( os, 2 ); //version number 
  vsl_b_write( os, a.ghost_count );
  vsl_b_write( os, a.weight );
  vsl_b_write( os, a.image );
  vsl_b_write( os, a.bbox );
}

void vsl_b_read( vsl_b_istream& is, vidtk::amhi_data &a )
{
  unsigned int version;
  vsl_b_read( is, version );
  vsl_b_read( is, a.ghost_count );
  vsl_b_read( is, a.weight );
  vsl_b_read( is, a.image );
  vsl_b_read( is, a.bbox );
  if( version <= 1 )
  {
    vidtk::image_histogram< vxl_byte, float> dummy;
    vsl_b_read( is, dummy );
  }
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track &t )
{
  vsl_b_write( os, 2 ); //version number
  vsl_b_write( os, t.id() );
  vsl_b_write( os, t.history() );
  bool has_pvo = t.has_pvo();
  vsl_b_write( os, has_pvo );
  if( has_pvo )
  {
    vidtk::pvo_probability pvo = t.get_pvo();
    vsl_b_write(os,pvo.get_probability_person());
    vsl_b_write(os,pvo.get_probability_vehicle());
    vsl_b_write(os,pvo.get_probability_other());
  }
  //
  //
  // Is the property map even used at all ???
  // vsl_b_write( os, t.data() );
  //
  vsl_b_write( os, t.amhi_datum() );
  vsl_b_write( os, t.last_mod_match() );
  vsl_b_write( os, t.false_alarm_likelihood() );
  unsigned int track_type = static_cast<unsigned int>(-1);
  if(t.data().get(vidtk::tracking_keys::track_type, track_type))
  {
    vsl_b_write( os, true );
    vsl_b_write( os, track_type );
  }
  else
  {
    vsl_b_write( os, false );
  }
}

void vsl_b_read( vsl_b_istream& is, vidtk::track &t )
{
  unsigned int version;
  vsl_b_read( is, version );
  unsigned int id;
  vsl_b_read( is, id );
  t.set_id(id);

  vcl_vector< vidtk::track_state_sptr > history;
  vsl_b_read( is, history );
  for( size_t i = 0; i < history.size(); ++i )
  {
    t.add_state(history[i]);
  }
  if(version == 2)
  {
    bool has_pvo;
    vsl_b_read( is, has_pvo );
    if( has_pvo )
    {
      double p,v,o;
      vsl_b_read(is, p);
      vsl_b_read(is, v);
      vsl_b_read(is, o);
      t.set_pvo(p,v,o);
    }
  }

  //
  // Is the property map even used at all ??? YES
  //

  vidtk::amhi_data amhi_datum;
  vsl_b_read( is, amhi_datum );
  t.set_amhi_datum(amhi_datum);

  double last_mod_match;
  vsl_b_read( is, last_mod_match );
  t.set_last_mod_match(last_mod_match);

  double false_alarm_likelihood;
  vsl_b_read( is, false_alarm_likelihood );
  t.set_false_alarm_likelihood(false_alarm_likelihood);

  bool has_track_type;
  vsl_b_read(is,has_track_type);
  if(has_track_type)
  {
    unsigned int track_type;
    vsl_b_read(is,track_type);
    t.data().set(vidtk::tracking_keys::track_type, track_type);
  }
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track *t )
{
  if (t==0)
  {
    vsl_b_write(os, false); // Indicate null pointer stored
  }
  else
  {
    vsl_b_write(os,true); // Indicate non-null pointer stored
    vsl_b_write(os,*t);
  }
}

void vsl_b_read( vsl_b_istream& is, vidtk::track *&t )
{
  delete t;
  bool not_null_ptr;
  vsl_b_read(is, not_null_ptr);
  if (not_null_ptr)
  {
    t = new vidtk::track();
    vsl_b_read(is, *t);
  }
  else
    t = 0;
}
