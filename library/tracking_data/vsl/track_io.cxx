/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_io.h"

#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>

#include <vnl/io/vnl_io_vector_fixed.h>
#include <vnl/io/vnl_io_matrix_fixed.h>

#include <vil/io/vil_io_image_view.h>
#include <vil/vil_new.h>

#include <vgl/io/vgl_io_box_2d.h>
#include <vgl/io/vgl_io_box_2d.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>

#include <tracking_data/tracking_keys.h>
#include <tracking_data/pvo_probability.h>
#include <tracking_data/track_state_attributes.h>
#include <tracking_data/vsl/image_object_io.h>
#include <tracking_data/vsl/vil_image_resource_sptr_io.h>

#include <utilities/vsl/timestamp_io.h>
#include <utilities/vsl/image_histogram_io.h>
#include <utilities/vsl/geo_io.h>

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track_state &ts )
{
  vsl_b_write( os, 3 ); // version number
  vsl_b_write( os, ts.time_ );
  vsl_b_write( os, ts.loc_ );
  vsl_b_write( os, ts.vel_ );
  vsl_b_write( os, ts.amhi_bbox_ );

  std::vector<vidtk::image_object_sptr> objs;
  if( ts.data_.get(vidtk::tracking_keys::img_objs, objs))
  {
    vsl_b_write( os, true );
    vsl_b_write( os, objs );
  }
  else
  {
    vsl_b_write( os, false);
  }

  //write lat lon
  double lat, lon;
  if(ts.latitude_longitude( lat, lon ))
  {
    vsl_b_write( os, true );
    vsl_b_write( os, lat );
    vsl_b_write( os, lon );
  }
  else
  {
    vsl_b_write( os, false );
  }

  //write attributes
  vidtk::track_state_attributes::raw_attrs_t att =  ts.get_attrs();
  vsl_b_write( os, att );

  //write gsd
  float gsd;
  if(ts.gsd( gsd ))
  {
    vsl_b_write( os, true );
    vsl_b_write( os, gsd );
  }
  else
  {
    vsl_b_write( os, false );
  }

  //write location cov
  vsl_b_write( os, true );
  vsl_b_write( os, ts.location_covariance() );

  //write out utm stuff is it exists
  {
    vidtk::geo_coord::geo_UTM geo;
    ts.get_smoothed_loc_utm( geo );
    vsl_b_write( os, geo );
    ts.get_raw_loc_utm( geo );
    vsl_b_write( os, geo );
    vnl_vector_fixed<double, 2> vel;
    ts.get_utm_vel( vel );
    vsl_b_write( os, vel );
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
  bool has;
  vsl_b_read( is, has );
  if( has )
  {
    std::vector<vidtk::image_object_sptr> objs;
    vsl_b_read( is, objs );
    ts.data_.set(vidtk::tracking_keys::img_objs, objs);
  }

  if(version == 3)
  {
    //read lat lon
    vsl_b_read( is, has );
    if(has)
    {
      double lat, lon;
      vsl_b_read( is, lat );
      vsl_b_read( is, lon );
      ts.set_latitude_longitude( lat, lon );
    }

    //read attributes
    vidtk::track_state_attributes::raw_attrs_t att;
    vsl_b_read( is, att );
    ts.set_attrs(att);

    //read gsd
    vsl_b_read( is, has );
    if(has)
    {
      float gsd;
      vsl_b_read( is, gsd );
      ts.set_gsd( gsd );
    }

    //read location cov
    vsl_b_read( is, has );
    if(has)
    {
      vnl_double_2x2 cov;
      vsl_b_read( is, cov );
      ts.set_location_covariance( cov );
    }
  }

  if(version == 2 || version == 3)
  {
    vidtk::geo_coord::geo_UTM geo;

    vsl_b_read( is, geo );
    ts.set_smoothed_loc_utm( geo );
    vsl_b_read( is, geo );
    ts.set_raw_loc_utm(geo);
    vnl_vector_fixed<double, 2> vel;
    vsl_b_read( is, vel );
    ts.set_utm_vel(vel);
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
  {
    ts = 0;
  }
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::amhi_data &a )
{
  vsl_b_write( os, 3 ); //version number
  vsl_b_write( os, a.ghost_count );
  vsl_b_write( os, a.weight );
  write_img_resource_b( os, a.image );
  vsl_b_write( os, a.bbox );
}

void vsl_b_read( vsl_b_istream& is, vidtk::amhi_data &a )
{
  unsigned int version;
  vsl_b_read( is, version );
  vsl_b_read( is, a.ghost_count );
  vsl_b_read( is, a.weight );
  if(version <= 2)
  {
    vil_image_view<vxl_byte> img;
    vsl_b_read( is, img );
    a.image = vil_new_image_resource_of_view(img);
  }
  else
  {
    read_img_resource_b( is, a.image );
  }
  vsl_b_read( is, a.bbox );
  if( version <= 1 )
  {
    vidtk::image_histogram dummy;
    vsl_b_read( is, dummy );
  }
}

//************************************************************************

void vsl_b_write( vsl_b_ostream& os, const vidtk::track &t )
{
  vsl_b_write( os, 2 ); //version number
  vsl_b_write( os, t.id() );
  vsl_b_write( os, t.history() );
  vidtk::pvo_probability pvo;
  bool has_pvo = t.get_pvo(pvo);
  vsl_b_write( os, has_pvo );
  if( has_pvo )
  {
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

  std::vector< vidtk::track_state_sptr > history;
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
      t.set_pvo(vidtk::pvo_probability(p, v, o));
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
