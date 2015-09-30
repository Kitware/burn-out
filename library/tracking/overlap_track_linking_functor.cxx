/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//overlap_track_linking_functor.cxx

#include "overlap_track_linking_functor.h"

#include <vgl/vgl_intersection.h>


namespace vidtk
{


overlap_track_linking_functor
::overlap_track_linking_functor( double bbox_size_sigma,
                                 double direction_sigma,
                                 double overlap_sigma,
                                 double overlap_mean,
                                 double distance_sigma,
                                 double bbox_not_overlap_sigma,
                                 double max_min_distance
                               )
  : bbox_size_sigma_(bbox_size_sigma), direction_sigma_(direction_sigma),
    overlap_sigma_(overlap_sigma), distance_sigma_(distance_sigma),
    overlap_mean_(overlap_mean), bbox_not_overlap_sigma_(bbox_not_overlap_sigma),
    max_min_distance_(max_min_distance)
{
}

double
overlap_track_linking_functor
::operator()( TrackTypePointer track_i, TrackTypePointer track_j )
{
  if(track_i->id() == track_j->id())
  {
    return impossible_match_cost_;
  }

  TrackStateList track_i_states = track_i->history();
  TrackStateList track_j_states = track_j->history();

  TrackState track_i_last_state = track_i_states.back();
  TrackState track_j_first_state = track_j_states.front();

  //Assumes there is some overlap
  if( track_j_first_state->time_ > track_i_last_state->time_ )
  {
    return impossible_match_cost_;
  }

  timestamp begin = track_j_first_state->time_;
  timestamp end = track_i_last_state->time_;
  TrackStateList sub_i = track_i->get_subtrack(begin, end)->history();
  TrackStateList sub_j = track_j->get_subtrack(begin, end)->history();
  if(sub_i.size()==0 || sub_j.size()==0)
  {
    return impossible_match_cost_;
  }

  unsigned int at_i = 0, at_j = 0;
  double count = 0;
  double sum_distance = 0;
  double sum_bbox = 0;
  double sum_dir = 0;
  double bbox_intersection_count = 0;
  double min_distance = max_min_distance_;

  while(at_i < sub_i.size() && at_j < sub_j.size())
  {
    TrackState track_i_state = sub_i[at_i];
    TrackState track_j_state = sub_j[at_j];
    if(track_i_state->time_ != track_j_state->time_)
    {
      if(track_i_state->time_<track_j_state->time_)
      {
        at_i++;
      }
      else //track_i_state->time_>track_j_state->time_
      {
        at_j++;
      }
      continue;
    }
    assert(track_i_state->time_ == track_j_state->time_);
    count++;
    double dpos = (track_i_state->loc_-track_j_state->loc_).magnitude();
    if(dpos<min_distance)
    {
      min_distance = dpos;
    }
    sum_distance += dpos;
    vgl_box_2d<unsigned> box_i, box_j;
    track_i_state->bbox(box_i);
    double size_i = (track_i_state->vel_[0] > track_i_state->vel_[1])?box_i.width():box_i.height();
    track_j_state->bbox(box_j);
    double size_j = (track_j_state->vel_[0] > track_j_state->vel_[1])?box_j.width():box_j.height();
    sum_bbox += vcl_abs(size_i - size_j);
    double dir_dot = dot_product( track_i_state->vel_.normalize(), track_j_state->vel_.normalize() );
    sum_dir += (1 - dir_dot);
    if(!vgl_intersection(box_i,box_j).is_empty())
    {
      bbox_intersection_count++;
    }
    at_i++;
    at_j++;
  }

  if(min_distance == max_min_distance_)
  {
    return impossible_match_cost_;
  }

  double mult = 1/count;


  double tmp = mult*sum_distance/distance_sigma_;
  double neg_log_distance = tmp*tmp;

  tmp = mult*sum_bbox / bbox_size_sigma_;
  double neg_log_size_weight = tmp*tmp;

  tmp = mult * sum_dir / direction_sigma_;
  double neg_log_direction_weight = tmp*tmp;

  tmp = (end.diff_in_secs(begin)-overlap_mean_)/overlap_sigma_;
  double neg_log_overlap = tmp*tmp;

  tmp = (1-bbox_intersection_count/count)/bbox_not_overlap_sigma_;
  double neg_log_not_overlap = tmp*tmp;

  //note: image weight unimplemented
  double neg_log_image_weight = 0.0;

  return ( neg_log_distance + neg_log_size_weight + neg_log_direction_weight +
           neg_log_image_weight + neg_log_overlap + neg_log_not_overlap )/6;
}

}
