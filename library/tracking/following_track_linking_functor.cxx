/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//following_track_linking_functor.cxx

#include "following_track_linking_functor.h"

#include <vnl/vnl_double_3.h>

namespace vidtk
{

following_track_linking_functor
::following_track_linking_functor( double bbox_size_sigma,
                                   double speed_sigma,
                                   double time_sigma,
                                   double direction_sigma,
                                   double pred_dist_sigma,
                                   double angle_velocity_sigma,
                                   double angle_velocity_threshold
                                 )
  : bbox_size_sigma_(bbox_size_sigma), speed_sigma_(speed_sigma),
    time_sigma_(time_sigma), direction_sigma_(direction_sigma),
    pred_dist_sigma_(pred_dist_sigma),
    angle_velocity_sigma_(angle_velocity_sigma),
    angle_velocity_threshold_(angle_velocity_threshold)
{
}

double
following_track_linking_functor
::operator()( TrackTypePointer track_i, TrackTypePointer track_j )
{
  if ( !track_i || !track_j || track_i->id() == track_j->id() )
  {
    return impossible_match_cost_;
  }
  TrackStateList track_i_states = track_i->history();
  TrackStateList track_j_states = track_j->history();
  if(track_i_states.empty() || track_j_states.empty())
  {
    return impossible_match_cost_;
  }

  if(track_i_states.size()<10 || track_j_states.size()<10)
  {
    return impossible_match_cost_;
  }

  TrackState track_i_last_state = track_i_states.back();
  TrackState track_j_first_state = track_j_states.front();

  if( track_j_first_state->time_ <= track_i_last_state->time_ )
  {
    return impossible_match_cost_;
  }

  //spatial ordering constraint
  //end of last track must be closer to start of next track
  //than to end of next track
  double dpos2 = (track_i_last_state->loc_-track_j_first_state->loc_).squared_magnitude();
  if( dpos2 > (track_i_last_state->loc_ - track_j_states.back()->loc_).squared_magnitude() )
  {
    return impossible_match_cost_;
  }
  //compair bounding box size
  vgl_box_2d<unsigned> box;
  track_i_last_state->bbox(box);
  double size_i = (track_i_last_state->vel_[0] > track_i_last_state->vel_[1])?box.width():box.height();
  track_j_first_state->bbox(box);
  double size_j = (track_j_first_state->vel_[0] > track_j_first_state->vel_[1])?box.width():box.height();
  double tmp = (size_i - size_j) / bbox_size_sigma_;
  double neg_log_size_weight = tmp*tmp;
  double dt = track_j_first_state->time_.diff_in_secs(track_i_last_state->time_);
  double track_i_last_state_mag = track_i_last_state->vel_.magnitude();
  //compair speed difference through turn
  tmp =(track_i_last_state_mag - vcl_sqrt(dpos2) / dt)/speed_sigma_;
  double neg_log_speed_weight = tmp*tmp;
  //amount of time that passed component
  tmp = dt / time_sigma_;
  double neg_log_time_weight = tmp*tmp;

  //Distance between the estimated half way points (more stable than projecting the full time either direction)
  tmp = ((track_i_last_state->vel_*(dt*.5)+track_i_last_state->loc_) - (track_j_first_state->loc_-track_j_first_state->vel_*(.5*dt))).magnitude()/pred_dist_sigma_;
  double neg_log_distance_weight = tmp*tmp;

  //look at the angel between start and end
  vnl_double_3 t_i_end_vel = (track_i_states[track_i_states.size()-1]->loc_-track_i_states[track_i_states.size()-5]->loc_)/
                             track_i_states[track_i_states.size()-1]->time_.diff_in_secs(track_i_states[track_i_states.size()-5]->time_);
  vnl_double_3 t_j_start_vel = (track_j_states[4]->loc_-track_j_states[0]->loc_)/
                                track_j_states[4]->time_.diff_in_secs(track_j_states[0]->time_);
  double ma_mb = t_i_end_vel.magnitude() * t_j_start_vel.magnitude();
  double dir_dot = dot_product( t_i_end_vel, t_j_start_vel );
  // 1 - cos(angle last_vel first_vel )
  tmp = ( 1 - (dir_dot/ma_mb) ) / direction_sigma_;
  double neg_log_direction_weight = tmp*tmp;

  double angle = vcl_acos(dir_dot/ma_mb);
  double a_vel = angle/dt;
//   if(a_vel>angle_velocity_threshold_)
//   {
//     return impossible_match_cost_;
//   }
  tmp = a_vel/angle_velocity_sigma_;
  double neg_log_angle_vel = tmp*tmp;
//   vcl_cout << "PAIR" << track_i->id() << "," << track_j->id() << ","
//                      << dt << ","
//                      << t_i_end_vel.magnitude() << "," << t_j_start_vel.magnitude()
//                      << ","
//                      << (size_i - size_j)
//                      << "," << (track_i_last_state_mag - vcl_sqrt(dpos2) / dt)
//                      << "," << ((track_i_last_state->vel_*(dt*.5)+track_i_last_state->loc_) - (track_j_first_state->loc_-track_j_first_state->vel_*(.5*dt))).magnitude()
//                      << "," << ( 1 - (dir_dot/ma_mb) ) << "," << angle << ","<< a_vel << vcl_endl;

  return (neg_log_size_weight + neg_log_speed_weight + neg_log_time_weight +
          neg_log_direction_weight + neg_log_distance_weight /*+ neg_log_angle_vel*/ ) / 5 /*6*/;
}

}
