/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "online_descriptor_helper_functions.h"

#include <logger/logger.h>

VIDTK_LOGGER("online_descriptor_helper_functions_cxx");

namespace vidtk
{

// ---------------------------------------------------------------------------------
// Make a new descriptor sptr
descriptor_sptr_t
create_descriptor( const std::string& id )
{
  return raw_descriptor::create( id );
}


// ---------------------------------------------------------------------------------
descriptor_sptr_t
create_descriptor( const std::string& id, track::track_id_t track_id )
{
  descriptor_sptr_t output = create_descriptor( id );
  output->add_track_id( track_id );
  return output;
}


// ---------------------------------------------------------------------------------
descriptor_sptr_t
create_descriptor( const std::string& id, const track_sptr& track )
{
  descriptor_sptr_t output = create_descriptor( id, track->id() );
  return output;
}

// ---------------------------------------------------------------------------------
// Normalize descriptor contents
bool
normalize_descriptor( const descriptor_sptr_t& ptr )
{
  double sum = 0.0;

  descriptor_data_t& data = ptr->get_features();

  for( unsigned i = 0; i < data.size(); i++ )
  {
    sum += data[i];
  }

  if( sum != 0.0 )
  {
    for( unsigned i = 0; i < data.size(); i++ )
    {
      data[i] = data[i] / sum;
    }
    return true;
  }

  return false;
}


// ---------------------------------------------------------------------------------
// Scale a bbox and then shift (expand) it by some set number of pixels
void
scale_region( image_region& bbox, double scale, unsigned shift )
{
  vgl_box_2d< int > signed_bbox;
  convert_cast_bbox( bbox, signed_bbox );
  scale_region( signed_bbox, scale, shift );

  if( signed_bbox.min_x() < 0 )
  {
    signed_bbox.set_min_x( 0 );
  }
  if( signed_bbox.min_y() < 0 )
  {
    signed_bbox.set_min_y( 0 );
  }

  convert_cast_bbox( signed_bbox, bbox );
}


// ---------------------------------------------------------------------------------
// Scale a bbox and then shift (expand) it by some set number of pixels
void
scale_region( vgl_box_2d< int >& bbox, double scale, unsigned shift )
{
  bbox.scale_about_centroid( scale );

  if( shift != 0.0 )
  {
    bbox.expand_about_centroid( shift );
  }
}


// ---------------------------------------------------------------------------------
// Retrieve the boolean tracker mask from a track, if it exists
vil_image_view<bool>
get_tracking_mask( track_sptr const& track )
{
  vidtk::image_object_sptr image_obj;

  if( !track->last_state()->image_object(image_obj) )
  {
    return vil_image_view<bool>();
  }

  vil_image_view< bool > mask;
  image_object::image_point_type origin;

  if( !image_obj->get_object_mask(mask, origin) )
  {
    return vil_image_view<bool>();
  }

  return mask;
}


// ---------------------------------------------------------------------------------
// Return latest track bbox
descriptor_history_entry::image_bbox_t
get_latest_bbox( const track_sptr& track )
{
  // Standard case
  descriptor_history_entry::image_bbox_t output;
  if( track->last_state()->bbox( output ) )
  {
    return output;
  }

  // Backup case, return last known bbox [shouldn't be required]
  for( int i = static_cast<int>(track->history().size()) - 1; i >= 0; --i )
  {
    if( track->history()[i]->bbox( output ) )
    {
      return output;
    }
  }

  LOG_WARN( "No bounding box found for track " << track->id() );
  return output;
}


// ---------------------------------------------------------------------------------
// Generate a recommended descriptor history entry for this frame and track
descriptor_history_entry
generate_history_entry( const frame_data_sptr& frame, const track_sptr& track )
{
  timestamp history_ts;
  descriptor_history_entry::image_bbox_t bbox = get_latest_bbox( track );
  descriptor_history_entry::world_bbox_t w_bbox;

  if( frame->was_timestamp_set() )
  {
    history_ts = frame->frame_timestamp();
  }

  if( frame->was_world_homography_set() )
  {
    const vnl_double_3x3& transform = frame->world_homography().get_matrix();

    vgl_point_2d< double > corners[4];

    corners[0] = vgl_point_2d< double >( bbox.min_x(), bbox.min_y() );
    corners[1] = vgl_point_2d< double >( bbox.min_x(), bbox.max_y() );
    corners[2] = vgl_point_2d< double >( bbox.max_x(), bbox.min_y() );
    corners[3] = vgl_point_2d< double >( bbox.max_x(), bbox.max_y() );

    for( int i = 0; i < 4; i++ )
    {
      vnl_double_3 hp( corners[i].x(), corners[i].y(), 1.0 );
      vnl_double_3 wp = transform * hp;

      if( !wp[2] )
      {
        w_bbox.add( vgl_point_2d< double >( wp[0] / wp[2], wp[1] / wp[2] ) );
      }
    }
  }

  // Create new entry
  descriptor_history_entry new_entry( history_ts, bbox, w_bbox);

  return new_entry;
}


// ---------------------------------------------------------------------------------
// Copy auxiliary history and track id information between two descriptors
void
copy_aux_descriptor_info( const descriptor_sptr& from_ptr, descriptor_sptr& to_ptr )
{
  to_ptr->set_history( from_ptr->get_history() );
  to_ptr->add_track_ids( from_ptr->get_track_ids() );
}

} // end namespace vidtk
