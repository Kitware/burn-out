/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_old_io.h"

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>

#include <vnl/io/vnl_io_vector_fixed.h>
#include <vnl/io/vnl_io_vector_fixed.txx>
#include <vil/io/vil_io_image_view.h>
#include <vgl/io/vgl_io_box_2d.h>
#include <vgl/io/vgl_io_box_2d.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <utilities/vsl/timestamp_io.h>
#include <tracking/tracking_keys.h>
#include <tracking/vsl/image_object_old_io.h>

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state_old &ts )
{
  vsl_b_write( os, ts.time_ );
  vsl_b_write( os, ts.loc_ );
  vsl_b_write( os, ts.vel_ );
  vsl_b_write( os, ts.amhi_bbox_ );

  // The only thing contained in the propertymap is a collection of
  // image objects
  vcl_vector<vidtk::image_object_sptr> objs;
  if( ts.data_.get(vidtk::tracking_keys::img_objs, objs))
  {
    vcl_vector<vidtk::image_object_old_sptr> objs_old(objs.size());
    for( size_t i = 0; i < objs.size(); ++i )
    {
      objs_old[i] = new vidtk::image_object_old(objs[i].ptr());
    }
    vsl_b_write( os, true );
    vsl_b_write( os, objs_old );
    /*
    size_t nObjs = objs.size();
    vsl_b_write( os, nObjs );
    for(size_t i = 0; i < nObjs; ++i )
    {
      vsl_b_write( os, &(*objs[i]) );
    }
    */
  }
  else
  {
    vsl_b_write( os, false);
  }
}

void vsl_b_read( vsl_b_istream& is, vidtk::track_state_old &ts )
{
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
    vcl_vector<vidtk::image_object_old_sptr> objs_old;
    vsl_b_read( is, objs_old );
    /*
    size_t nObjs = objs.size();
    vsl_b_read( is, nObjs );
    objs.resize(nObjs);
    for(size_t i = 0; i < nObjs; ++i )
    {
      vidtk::image_object *obj = 0;
      vsl_b_read( is, obj );
      objs[i] = obj;
    }
    */
    vcl_vector<vidtk::image_object_sptr> objs(objs_old.size());
    for( size_t i = 0; i < objs_old.size(); ++i )
    {
      objs[i] = objs_old[i].ptr();
    }
    ts.data_.set(vidtk::tracking_keys::img_objs, objs);
  }
}

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state_old *ts )
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

void vsl_b_read( vsl_b_istream& is, vidtk::track_state_old *&ts )
{
  delete ts;
  bool not_null_ptr;
  vsl_b_read(is, not_null_ptr);
  if (not_null_ptr)
  {
    ts = new vidtk::track_state_old();
    vsl_b_read(is, *ts);
  }
  else
    ts = 0;
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::amhi_data_old &a )
{
  vsl_b_write( os, a.ghost_count );
  vsl_b_write( os, a.weight );
  vsl_b_write( os, a.image );
  vsl_b_write( os, a.bbox );
}

void vsl_b_read( vsl_b_istream& is, vidtk::amhi_data_old &a )
{
  vsl_b_read( is, a.ghost_count );
  vsl_b_read( is, a.weight );
  vsl_b_read( is, a.image );
  vsl_b_read( is, a.bbox );
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_old &t )
{
  vsl_b_write( os, t.id() );

  vcl_vector<vidtk::track_state_old_sptr> states_old(t.history().size());
  for( size_t i = 0; i < states_old.size(); ++i )
  {
    states_old[i] = new vidtk::track_state_old(t.history()[i].ptr());
  }
  vsl_b_write( os, states_old );
  //
  //
  // Is the property map even used at all ???
  // vsl_b_write( os, t.data() );
  //
  vidtk::amhi_data_old amhi(const_cast<vidtk::amhi_data*>(&t.amhi_datum()));
  vsl_b_write( os, amhi);

  vsl_b_write( os, t.last_mod_match() );
  vsl_b_write( os, t.false_alarm_likelihood() );
}

void vsl_b_read( vsl_b_istream& is, vidtk::track_old &t )
{
  unsigned int id;
  vsl_b_read( is, id );
  t.set_id(id);

  vcl_vector< vidtk::track_state_old_sptr > states_old;
  vsl_b_read( is, states_old );
  for( size_t i = 0; i < states_old.size(); ++i )
  {
    t.add_state(states_old[i]->new_ptr());
  }

  //
  // Is the property map even used at all ???
  //

  vidtk::amhi_data_old amhi_datum;
  vsl_b_read( is, amhi_datum );
  t.set_amhi_datum(amhi_datum);

  double last_mod_match;
  vsl_b_read( is, last_mod_match );
  t.set_last_mod_match(last_mod_match);

  double false_alarm_likelihood;
  vsl_b_read( is, false_alarm_likelihood );
  t.set_false_alarm_likelihood(false_alarm_likelihood);
}

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_old *t )
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

void vsl_b_read( vsl_b_istream& is, vidtk::track_old *&t )
{
  delete t;
  bool not_null_ptr;
  vsl_b_read(is, not_null_ptr);
  if (not_null_ptr)
  {
    t = new vidtk::track_old();
    vsl_b_read(is, *t);
  }
  else
    t = 0;
}
