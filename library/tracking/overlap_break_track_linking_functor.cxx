/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//overlap_break_track_linking_functor.cxx

#include "overlap_break_track_linking_functor.h"

#include <vgl/vgl_intersection.h>

#include <vcl_algorithm.h>

namespace vidtk
{

double
overlap_break_track_linking_functor
::operator()( TrackTypePointer track_i, TrackTypePointer track_j )
{
  if(track_i->id() == track_j->id())
  {
    return impossible_match_cost_;
  }

  TrackStateList track_i_states = track_i->history();
  TrackStateList track_j_states = track_j->history();
  if(track_i_states.size() <= 1 || track_j_states.size() <= 1)
  {
    return impossible_match_cost_;
  }

  TrackState track_i_last_state = track_i_states.back();
  TrackState track_j_first_state = track_j_states.front();
  TrackState track_j_last_state = track_j_states.back();
  TrackState track_i_first_state = track_i_states.front();

  //Assumes there is some overlap
  if( track_j_first_state->time_ > track_i_last_state->time_ )
  {
    return impossible_match_cost_;
  }

  if(track_j_last_state->time_ < track_i_first_state->time_)
  {
    return impossible_match_cost_;
  }

  timestamp begin = track_j_first_state->time_;
  timestamp end = track_i_last_state->time_;
  double score_1, score_2, score_3;
  {
    track_sptr sub_i = track_i->get_subtrack_shallow(track_i_states.front()->time_, begin);
    if(sub_i->history().size()<2)
    {
      score_1 = impossible_match_cost_;
    }
    else
    {
      sub_i = sub_i->get_subtrack_shallow(track_i_states.front()->time_, sub_i->history()[sub_i->history().size()-2]->time_);
      track_sptr sub_j = track_j->get_subtrack_shallow(end, track_j_states.back()->time_);
      if(sub_i->history().empty() || sub_j->history().empty())
      {
        score_1 = impossible_match_cost_;
      }
      else
      {
        sub_i->set_id(track_i->id());
        sub_j->set_id(track_j->id());
        score_1 = (*functor_)(sub_i,sub_j);
      }
    }
  }
  {
    track_sptr sub_i = track_i->get_subtrack_shallow(track_i_states.front()->time_, track_i_states[track_i_states.size()-2]->time_);
    track_sptr sub_j = track_j->get_subtrack_shallow(end, track_j_states.back()->time_);
    if(sub_i->history().empty() || sub_j->history().empty())
    {
      score_2 = impossible_match_cost_;
    }
    else
    {
      sub_i->set_id(track_i->id());
      sub_j->set_id(track_j->id());
      score_2 = (*functor_)(sub_i,sub_j);
    }
  }
  {
    track_sptr sub_i = track_i->get_subtrack_shallow(track_i_states.front()->time_, begin);
    track_sptr sub_j = track_j->get_subtrack_shallow(track_j_states[1]->time_, track_j_states.back()->time_);
    if(sub_i->history().empty() || sub_j->history().empty())
    {
      score_3 = impossible_match_cost_;
    }
    else
    {
      sub_i->set_id(track_i->id());
      sub_j->set_id(track_j->id());
      score_3 = (*functor_)(sub_i,sub_j);
    }
  }
  double result;
  //vcl_cout << "three scores: " << score_1<<" "<<score_2<<" "<<score_3 << vcl_endl;
  switch( merge_stratigy_ )
  {
    case MERGE_MIN:
      result = vcl_min(vcl_min(score_1,score_2), score_3);
      break;
    case MERGE_MAX:
      result = vcl_max( vcl_max(score_1,score_2), score_3);
      break;
    case MERGE_MEAN:
      result = (score_1+score_2+score_3)/3.0;
      break;
  }
  if(overlap_time_sigma_set_)
  {
    double tmp = end.diff_in_secs(begin)/overlap_time_sigma_;
    return ( result + tmp*tmp )*0.5;
  }
  return result;
}

void
overlap_break_track_linking_functor
::merge_type(overlap_score_merge_type osmt)
{
  merge_stratigy_ = osmt;
}

void
overlap_break_track_linking_functor
::overlap_time_sigma( double s )
{
  overlap_time_sigma_set_ = true;
  overlap_time_sigma_ = s;
}

overlap_break_track_linking_functor
::overlap_break_track_linking_functor( track_linking_functor_sptr sub_functor )
  : functor_(sub_functor), merge_stratigy_(MERGE_MIN), overlap_time_sigma_set_(false)
{
}

}
