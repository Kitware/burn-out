/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "meds_generator.h"

#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>

#include <vgl/vgl_box_2d.h>

#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <limits>

#include <tracking_data/track_state.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "meds_generator_cxx" );

typedef double float_t;
typedef vgl_box_2d<unsigned> bbox_t;
typedef vidtk::timestamp timestamp_t;
typedef vnl_vector_fixed<double,3> location_t;

// Any data we want stored for each track and each frame
struct meds_frame_data
{
  // Timestamp for this entry
  timestamp_t ts;

  // Object bounding box, image coordinates
  bbox_t bbox;

  // Misc extracted features:
  //  1) velocity magnitude
  //  2) acceleration magnitude
  //  3) velocity angle
  //  4) bbox ratio
  //  5) bbox area
  float_t features[5];

  // Location
  location_t loc;
};

typedef boost::circular_buffer< meds_frame_data > meds_buffer_t;

// Any data we want stored for each individual track we encounter
struct meds_track_data : track_data_storage
{
  timestamp_t last_state_ts;
  float_t max_speed;
  float_t max_acceleration;
  meds_buffer_t history;
  location_t first_loc;
  timestamp_t first_ts;
  bool is_first_frame;

  meds_track_data()
  : last_state_ts(),
    max_speed(0),
    max_acceleration(0),
    history(),
    first_loc(),
    first_ts(),
    is_first_frame(true)
  {}

  virtual ~meds_track_data()
  {}
};

meds_generator
::meds_generator()
{
  this->configure( meds_settings() );
}

std::vector<unsigned>
parse_id_list( const std::string& str )
{
  std::vector<unsigned> ids;
  std::stringstream ss( str );
  std::string tmp;
  while( ss >> tmp )
  {
    ids.push_back( boost::lexical_cast<unsigned>( tmp ) );
  }
  return ids;
}

bool
meds_generator
::configure( const external_settings& settings )
{
  meds_settings const* desc_settings
    = static_cast<meds_settings const*>( &settings );

  if( !desc_settings )
  {
    return false;
  }

  // Create internal copy of inputted external settings
  this->settings_ = *desc_settings;

  // Set internal descriptor running mode options
  generator_settings running_options;
  running_options.thread_count = 1;
  running_options.sampling_rate = 1;
  this->set_operating_mode( running_options );

  return true;
}


external_settings* meds_generator
::create_settings()
{
  return new meds_settings;
}


track_data_sptr
meds_generator
::new_track_routine( track_sptr const& new_track )
{
  meds_track_data* track_data = new meds_track_data();
  track_data_sptr data_sptr( track_data );
  track_data->history.set_capacity( settings_.buffer_length );
  track_update_routine( new_track, data_sptr );
  return data_sptr;
}

float_t
compute_obj_speed( const vnl_vector_fixed<double,3>& velocity )
{
  return velocity.magnitude();
}

float_t
compute_obj_angle( const vnl_vector_fixed<double,3>& velocity )
{
  if( velocity[0] != 0 && velocity[1] != 0 )
  {
    return std::atan2( velocity[1], velocity[0] );
  }
  return 0.0;
}

float_t
compute_bbox_side_ratio( const bbox_t& bbox )
{
  if( bbox.height() == 0 )
  {
    return std::numeric_limits< float_t >::max();
  }
  return static_cast<double>( bbox.width() ) / bbox.height();
}

float_t
location_distance( const location_t& l1, const location_t& l2 )
{
  return vnl_vector_ssd< float_t >( l1, l2 );
}

void
compute_var_measurements( const meds_buffer_t& buffer,
                          const unsigned feature,
                          float_t& mean,
                          float_t& var )
{
  mean = 0.0;
  var = 0.0;

  for( unsigned i = 0; i < buffer.size(); ++i )
  {
    mean += buffer[i].features[feature];
  }

  mean = ( buffer.size() != 0 ? mean / buffer.size() : 0 );

  if( mean != 0 )
  {
    for( unsigned i = 0; i < buffer.size(); ++i )
    {
      double norm_vec = ( buffer[i].features[feature] / mean ) - 1.0;

      var += norm_vec * norm_vec;
    }

    var = ( buffer.size() != 0 ? var / ( buffer.size() ) : 0 );
  }
}

// MEDS Descriptor Indices:
//
// 1. Came from person tracker?
// 2. Came from vehicle tracker?
// 3. Max velocity
// 4. Max acceleration
// 5. Current velocity
// 6. Current acceleration
// 7. Current bbox target area
// 8. Current bbox aspect ratio
// 9. Current estimated target area
// 10. Average of bbox size
// 11. Average of bbox ratio
// 12. Average of target speed
// 13. Average of velocity angle
// 14. Average deviation of velocity magnitude
// 15. Average deviation of veolcity angle
// 16. Average deviation of bbox ratio
// 17. Average deviation of bbox area
// 18. Percent FG coverage score, if available
// 19. Maximum recorded distance from start location
// 20. Current track length (in time)
// 21. Distance traveled during buffering period
bool
meds_generator
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  meds_track_data* data =
    static_cast<meds_track_data*>(track_data.get());

  const double frame_gsd = current_frame()->average_gsd();

  // First, determine if this is a new state
  const track_state_sptr& last_state = active_track->last_state();

  if( data->last_state_ts.has_time() &&
      last_state->time_.time() == data->last_state_ts.time() )
  {
    return true;
  }

  // Update the history for this descriptor
  data->last_state_ts = last_state->time_;

  meds_frame_data frame_data;

  frame_data.ts = last_state->time_;

  if( !last_state->bbox( frame_data.bbox ) )
  {
    return true;
  }

  if( data->is_first_frame )
  {
    data->first_loc = last_state->loc_;
    data->first_ts = last_state->time_;
    data->is_first_frame = false;
  }

  frame_data.loc = last_state->loc_;

  frame_data.features[0] = compute_obj_speed( last_state->vel_ );
  frame_data.features[1] = 0.0;
  frame_data.features[2] = compute_obj_angle( last_state->vel_ );
  frame_data.features[3] = compute_bbox_side_ratio( frame_data.bbox );
  frame_data.features[4] = frame_data.bbox.volume() * frame_gsd;

  if( frame_data.features[0] > data->max_speed )
  {
    data->max_speed = frame_data.features[0];
  }

  if( data->history.size() > 2 )
  {
    const meds_frame_data& s2 = data->history[ data->history.size()-1 ];
    const meds_frame_data& s1 = data->history[ data->history.size()-2 ];

    frame_data.features[1] = ( s2.features[0] - s1.features[0] ) /
                               ( s2.ts.time() - s1.ts.time() );

    if( frame_data.features[1] > data->max_acceleration )
    {
      data->max_acceleration = frame_data.features[1];
    }
  }

  data->history.push_back( frame_data );

  // Compute a descriptor for this entry and output it
  descriptor_history_entry history_entry =
    generate_history_entry( current_frame(), active_track );

  descriptor_sptr descriptor =
    create_descriptor( settings_.descriptor_name, active_track->id() );

  descriptor_data_t& descriptor_values = descriptor->get_features();

  descriptor_values.resize( 21, 0.0 );

  // Descriptor indices 1-2: Tracker IDs
  for( unsigned i = 0; i < settings_.person_tracker_ids.size(); ++i )
  {
    if( active_track->get_tracker_id() == settings_.person_tracker_ids[i] )
    {
      descriptor_values[0] = 1.0;
      break;
    }
  }
  for( unsigned i = 0; i < settings_.vehicle_tracker_ids.size(); ++i )
  {
    if( active_track->get_tracker_id() == settings_.vehicle_tracker_ids[i] )
    {
      descriptor_values[1] = 1.0;
      break;
    }
  }

  // Descriptor indices 3-6: Simple velocity information
  descriptor_values[2] = data->max_speed;
  descriptor_values[3] = data->max_acceleration;
  descriptor_values[4] = frame_data.features[0];
  descriptor_values[5] = frame_data.features[1];

  // Descriptor indices 7-9:
  descriptor_values[6] = frame_data.features[4];
  descriptor_values[7] = frame_data.features[3];
  descriptor_values[8] = descriptor_values[6];

  // Descriptor indices 10-17
  compute_var_measurements( data->history, 0, descriptor_values[9], descriptor_values[10] );
  compute_var_measurements( data->history, 2, descriptor_values[11], descriptor_values[12] );
  compute_var_measurements( data->history, 3, descriptor_values[13], descriptor_values[14] );
  compute_var_measurements( data->history, 4, descriptor_values[15], descriptor_values[16] );

  // Descriptor indices 9 and 18
  descriptor_values[17] = 1.0;

  image_object_sptr obj;

  if( last_state->image_object( obj ) )
  {
    descriptor_values[8] = obj->get_area();

    if( frame_data.bbox.volume() != 0 && obj->get_area() != -1 )
    {
      descriptor_values[17] = obj->get_area() / frame_data.bbox.volume();
    }
  }

  // Descriptor indices 19-21
  descriptor_values[18] = location_distance( last_state->loc_, data->first_loc );
  descriptor_values[19] = last_state->time_.time() - data->first_ts.time();
  descriptor_values[20] = location_distance( last_state->loc_, data->history[0].loc );

  // Output single state descriptor
  descriptor->add_history_entry( history_entry );
  add_outgoing_descriptor( descriptor );
  return true;
}


bool
meds_generator
::terminated_track_routine( track_sptr const& finished_track,
                            track_data_sptr track_data )
{
  return track_update_routine( finished_track, track_data );
}

} // end namespace vidtk
