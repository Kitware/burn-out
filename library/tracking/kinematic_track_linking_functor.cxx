/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//kinematic_track_linking_functor.cxx

#include "kinematic_track_linking_functor.h"

namespace vidtk
{

double
kinematic_track_linking_functor
::operator()( TrackTypePointer track_i, TrackTypePointer track_j )
{
  double cost = 0.0;

  TrackStateList track_i_states = track_i->history();
  TrackStateList track_j_states = track_j->history();

  TrackState track_i_last_state = track_i_states.back();
  TrackState track_j_first_state = track_j_states.front();

  // Check temporal constraint.
  // track_i must end before track_j begins.

  if( track_j_first_state->time_.time_in_secs() < track_i_last_state->time_.time_in_secs() )
  {

    cost = impossible_match_cost_;

  }
  else if(back_step_==0)
  {

    // Predict each track and compare predicted locations
    double deltaTSec = track_j_first_state->time_.diff_in_secs( track_i_last_state->time_ );

    // Track i location is in world units.
    LocationVector track_i_last_loc = track_i_last_state->loc_;

    // Velocity is in world units/sec
    VelocityVector track_i_last_vel = track_i_last_state->vel_;

    // Predicted position.
    LocationVector track_i_predicted_loc = track_i_last_loc + track_i_last_vel * deltaTSec;

    // Track j location in world units.
    LocationVector track_j_first_loc = track_j_first_state->loc_;

    // Track j velocity in world units/sec
    VelocityVector track_j_first_vel = track_j_first_state->vel_;

    // Predict track j backwards in time, hence the -1.0 on the velocity.
    LocationVector track_j_predicted_loc = track_j_first_loc + track_j_first_vel * ( -1.0 * deltaTSec );

    // Difference vector between predicted locations.
    //LocationDifferenceVector loc_diff = track_i_predicted_loc - track_j_predicted_loc;
    // Length of difference vector.
    //double scalar_diff = loc_diff.two_norm();

    // Take average of ||pi - pi'|| and ||pj - pj'||
    LocationDifferenceVector track_i_err = track_i_predicted_loc - track_j_first_loc;
    LocationDifferenceVector track_j_err = track_j_predicted_loc - track_i_last_loc;
    double scalar_diff = 0.5 * ( track_i_err.two_norm() + track_j_err.two_norm() );


    // Check that the velocity vectors are not headed in nearly opposite directions
    VelocityVector track_i_last_vel_norm = track_i_last_vel / track_i_last_vel.two_norm();
    VelocityVector track_j_first_vel_norm = track_j_first_vel / track_j_first_vel.two_norm();

    double dp1 = dot_product( track_i_last_vel_norm, track_j_first_vel_norm );

    // Check that tracks do not overlap.
    // A vector from end of first track to start of second track
    // should be roughly aligned with the velocity vector of each track.

    LocationDifferenceVector end_of_i_to_begin_of_j = track_j_first_loc - track_i_last_loc;
    double distance_between_i_and_j = end_of_i_to_begin_of_j.two_norm();
    end_of_i_to_begin_of_j.normalize();

    double dp2 = dot_product( end_of_i_to_begin_of_j, track_i_last_vel_norm );
    double dp3 = dot_product( end_of_i_to_begin_of_j, track_j_first_vel_norm );

    // Cost is length of the difference vector
    cost = scalar_diff +
           vcl_exp(distance_between_i_and_j - exp_transition_point_distance_) +
           ( 1.0 - dp1 ) * direction_penalty_ +
           ( 1.0 - dp2 ) * direction_penalty_ +
           ( 1.0 - dp3 ) * direction_penalty_;

  }
  else
  {
    // Predict each track and compare predicted locations
    double deltaTSec = track_j_first_state->time_.diff_in_secs( track_i_last_state->time_ );

    // Track i location is in world units.
    LocationVector track_i_last_loc = track_i_last_state->loc_;

    // Velocity is in world units/sec
    VelocityVector track_i_last_vel = track_i_last_state->vel_;

    // Predicted position.
    LocationVector track_i_predicted_loc = track_i_last_loc + track_i_last_vel * deltaTSec;

    // Track j location in world units.
    LocationVector track_j_first_loc = track_j_first_state->loc_;

    // Track j velocity in world units/sec
    VelocityVector track_j_first_vel = track_j_first_state->vel_;

    // Predict track j backwards in time, hence the -1.0 on the velocity.
    LocationVector track_j_predicted_loc = track_j_first_loc + track_j_first_vel * ( -1.0 * deltaTSec );

    // Difference vector between predicted locations.
    //LocationDifferenceVector loc_diff = track_i_predicted_loc - track_j_predicted_loc;
    // Length of difference vector.
    //double scalar_diff = loc_diff.two_norm();

    // Take average of ||pi - pi'|| and ||pj - pj'||
    LocationDifferenceVector track_i_err = track_i_predicted_loc - track_j_first_loc;
    LocationDifferenceVector track_j_err = track_j_predicted_loc - track_i_last_loc;
    double scalar_diff = 0.5 * ( track_i_err.two_norm() + track_j_err.two_norm() );


    // Check that the velocity vectors are not headed in nearly opposite directions
    VelocityVector track_i_last_vel_norm = track_i_last_vel / track_i_last_vel.two_norm();
    VelocityVector track_j_first_vel_norm = track_j_first_vel / track_j_first_vel.two_norm();

    double dp1 = dot_product( track_i_last_vel_norm, track_j_first_vel_norm );

    // Check that tracks do not overlap.
    // A vector from end of first track to start of second track
    // should be roughly aligned with the velocity vector of each track.

    LocationDifferenceVector end_of_i_to_begin_of_j = track_j_first_loc - track_i_last_loc;
    double distance_between_i_and_j = end_of_i_to_begin_of_j.two_norm();
    end_of_i_to_begin_of_j.normalize();

    double dp2 = dot_product( end_of_i_to_begin_of_j, track_i_last_vel_norm );
    double dp3 = dot_product( end_of_i_to_begin_of_j, track_j_first_vel_norm );

    // Cost is length of the difference vector
    cost = scalar_diff +
           vcl_exp(distance_between_i_and_j - exp_transition_point_distance_) +
           ( 1.0 - dp1 ) * direction_penalty_ +
           ( 1.0 - dp2 ) * direction_penalty_ +
           ( 1.0 - dp3 ) * direction_penalty_;
  }

  if(cost > impossible_match_cost_)
  {
    return impossible_match_cost_;
  }

  return cost;
}

kinematic_track_linking_functor
::kinematic_track_linking_functor( double sigma,
                                   double direction_penalty,
                                   double exp_transition_point_distance )
    : track_linking_functor(1000.0), sigma_squared_( sigma*sigma ), back_step_(0),
      direction_penalty_(direction_penalty),
      exp_transition_point_distance_(exp_transition_point_distance)
{
  norm_const_ = 1.0 / sqrt( 2.0 * vnl_math::pi * sigma_squared_ );
}

}
