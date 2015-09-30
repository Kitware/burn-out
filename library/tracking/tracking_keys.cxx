/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tracking_keys.h"

#include <tracking/image_object.h>
#include <tracking/fg_matcher.h>
#include <utilities/log.h>
#include <utilities/image_histogram.h>
#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <vnl/vnl_double_2x2.h>

namespace vidtk
{

  namespace tracking_keys
{

property_map::key_type img_objs = property_map::key( "img_objs" );

property_map::key_type used_in_track = property_map::key( "used_in_track" );

property_map::key_type pixel_data = property_map::key( "pixel_data" );

property_map::key_type pixel_data_buffer = property_map::key( "pixel_data_buffer" );

property_map::key_type track_type = property_map::key( "track_type" );

property_map::key_type histogram = property_map::key( "histogram" );

property_map::key_type foreground_model = property_map::key( "foreground_model" );

property_map::key_type intensity_distribution = property_map::key( "intensity_distribution" );

property_map::key_type lat_long = property_map::key( "lat_long" );

property_map::key_type gsd = property_map::key( "gsd" );

property_map::key_type loc_cov = property_map::key( "loc_cov" );

property_map::key_type note = property_map::key( "note" );

property_map::key_type tracker_type = property_map::key( "tracker_type" );

property_map::key_type tracker_subtype = property_map::key( "tracker_subtype" );


//Any tracking keys that get added must have a statement here where
//their value is deep copied.
void deep_copy_property_map( property_map const & src,
                             property_map & dest)
{
  if(src.has(tracking_keys::used_in_track))
  {
    bool src_used_in_track = false;
    src.get(tracking_keys::used_in_track,src_used_in_track);
    dest.set(tracking_keys::used_in_track,src_used_in_track);
  }

  if(src.has(tracking_keys::pixel_data_buffer))
  {
    unsigned int src_pixel_data_buffer = 0;
    src.get(tracking_keys::pixel_data_buffer,src_pixel_data_buffer);
    dest.set(tracking_keys::pixel_data_buffer,src_pixel_data_buffer);
  }

  if(src.has(tracking_keys::histogram))
  {
    image_histogram<vxl_byte, bool> src_hist;
    src.get(tracking_keys::histogram, src_hist);
    image_histogram<vxl_byte, bool> dest_hist = src_hist;
    dest.set(tracking_keys::histogram, dest_hist);
  }

  if(src.has(tracking_keys::foreground_model))
  {
    fg_matcher< vxl_byte >::sptr_t src_fgm;
    src.get(tracking_keys::foreground_model, src_fgm);
    fg_matcher< vxl_byte >::sptr_t dst_fgm;
    dst_fgm = src_fgm->deep_copy();
    dest.set(tracking_keys::foreground_model, dst_fgm);
  }

  if(src.has(tracking_keys::pixel_data))
  {
    vil_image_view<vxl_byte> src_pixel_data;
    src.get(tracking_keys::pixel_data,src_pixel_data);
    vil_image_view<vxl_byte> dest_pixel_data;
    dest_pixel_data.deep_copy(src_pixel_data);
    dest.set(tracking_keys::pixel_data,dest_pixel_data);
  }

  if(src.has(tracking_keys::track_type))
  {
    unsigned int src_track_type = 0;
    src.get(tracking_keys::track_type, src_track_type);
    dest.set(tracking_keys::track_type, src_track_type);
  }

  if(src.has(tracking_keys::img_objs))
  {
    vcl_vector<image_object_sptr> src_img_objs;
    vcl_vector<image_object_sptr> dest_img_objs;
    src.get(tracking_keys::img_objs,src_img_objs);
    vcl_vector<image_object_sptr>::iterator iter;
    for(iter = src_img_objs.begin(); iter != src_img_objs.end(); ++iter)
    {
      image_object_sptr src_obj = (*iter);
      image_object_sptr dest_obj = src_obj->clone();
      dest_img_objs.push_back(dest_obj);
    }
    dest.set(tracking_keys::img_objs,dest_img_objs);
  }

  if(src.has(tracking_keys::note))
  {
    vcl_string text;
    src.get(tracking_keys::note, text);
    dest.set(tracking_keys::note, text);
  }

  if(src.has(tracking_keys::lat_long))
  {
    vcl_pair<double, double> lat_long;
    src.get(tracking_keys::lat_long, lat_long);
    dest.set(tracking_keys::lat_long, lat_long);
  }

  if(src.has(tracking_keys::gsd))
  {
    float gsd = 0.0;
    src.get(tracking_keys::gsd, gsd);
    dest.set(tracking_keys::gsd, gsd);
  }

  if(src.has(tracking_keys::loc_cov))
  {
    vnl_double_2x2 cov;
    src.get(tracking_keys::loc_cov, cov);
    dest.set(tracking_keys::loc_cov, cov);
  }

  if (src.has(tracking_keys::tracker_type))
  {
    unsigned type;
    src.get(tracking_keys::tracker_type, type);
    dest.set(tracking_keys::tracker_type, type);
  }

  if (src.has(tracking_keys::tracker_subtype))
  {
    unsigned subtype;
    src.get(tracking_keys::tracker_subtype, subtype);
    dest.set(tracking_keys::tracker_subtype, subtype);
  }

  key_map_type key_map = src.key_map();
  key_map_type::iterator key_iter = key_map.begin();

  //Check to make sure nobody added any tracking keys without adding them to
  // the deep copy
  for( /* nothing */ ;key_iter != key_map.end(); ++key_iter)
  {
    log_assert(src.has((*key_iter).second) == dest.has((*key_iter).second),
               "Deep copying \"" + (*key_iter).first
               + "\" is not yet implemented in tracking_keys::deep_copy_property_map()");

  }
}

} // end namespace tracking_keys

} // end namespace vidtk
