/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_linking_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vcl_utility.h> // for vcl_pair<.,.>
#include <vcl_algorithm.h>
#include <vcl_sstream.h>
#include <vcl_string.h>

#include <vcl_map.h>
#include <vcl_set.h>

#include "kinematic_track_linking_functor.h"
#include "visual_track_linking_functor.h"
#include "overlap_track_linking_functor.h"
#include "following_track_linking_functor.h"

namespace vidtk
{

track_linking_process
::track_linking_process( vcl_string const& name )
   : process( name, "track_linking_process" ),
     time_window_(static_cast<unsigned int>(-1)),
     timestamp_(NULL), flush_(false), terminate_(false), disabled_(true)
{
  //track_linking_process parameters
  config_.add_parameter( "track_termination_cost", "50", 
                         "The cost to terminate a track" );
  config_.add_parameter( "impossible_match_cost", "10000", 
                         "Cost used for impossible matches" );

  //kinematic_track_linking_functor parameters
  config_.add_parameter( "kinematic_track_linking_functor:weight", "0.7",
                         "The weight aplied to the result of the cost for the kinematic match."
                         "  If one want to turn it off set the value to 0.  Note: the weights"
                         " for all the functors will be made to sum to one." );
  config_.add_parameter( "kinematic_track_linking_functor:sigma", "1", 
                         "SD for a single-dimensional Gaussian distribution" );
  config_.add_parameter( "kinematic_track_linking_functor:direction_penalty", "40.0",
                         "The penalty multiplied to the angle angles of the velosity" );
  config_.add_parameter( "kinematic_track_linking_functor:exp_transition_point_distance", "30.0",
                         "The distance between to endpoints before the cost becomes strongly exponential" );
  config_.add_parameter( "assignment_algorithm", "HUNGARIAN",
                         "Implated assignement algorithms are HUNGARIAN and GREEDY");
  config_.add_parameter( "disabled", "true", "Allows the user to turn on or off track linking");

  //visual_track_linking_functor parameters
  config_.add_parameter( "visual_track_linking_functor:weight", "0.3",
                         "The weight aplied to the result of the cost for the visual match."
                         "  If one want to turn it off set the value to 0.  Note: the weights"
                         " for all the functors will be made to sum to one." );
  config_.add_parameter( "visual_track_linking_functor:area_penalty", "50",
                         "This is the penalty multiplied to the area ratio" );
  config_.add_parameter( "visual_track_linking_functor:mean_dist_penalty", "10",
                         "This is the penalty multiplied to the average mehelinobis mean distance" );
  config_.add_parameter( "visual_track_linking_functor:chi_penalty", "10",
                         "This the penalty multiplied to the chi squared distance of variances" );

  //following_track_linking_functor parameters
  config_.add_parameter( "following_track_linking_functor:weight", "0.0","");
  config_.add_parameter( "following_track_linking_functor:bbox_size_sigma", "2.0","");
  config_.add_parameter( "following_track_linking_functor:speed_sigma", "2.5","");
  config_.add_parameter( "following_track_linking_functor:time_sigma", "5.0","");
  config_.add_parameter( "following_track_linking_functor:direction_sigma", "0.25","");
  config_.add_parameter( "following_track_linking_functor:pred_dist_sigma", "10","");

  //overlap_track_linking_functor parameters
  config_.add_parameter( "overlap_track_linking_functor:weight", "0.0","");
  config_.add_parameter( "overlap_track_linking_functor:bbox_size_sigma", "2.0","");
  config_.add_parameter( "overlap_track_linking_functor:direction_sigma", "0.25","");
  config_.add_parameter( "overlap_track_linking_functor:sigma", "0.0","");
  config_.add_parameter( "overlap_track_linking_functor:mean", "2.5","");
  config_.add_parameter( "overlap_track_linking_functor:distance_sigma", "5.0","");
  config_.add_parameter( "overlap_track_linking_functor:bbox_not_overlap_sigma", "0.6","");
  config_.add_parameter( "overlap_track_linking_functor:max_min_distance", "7.0","");

  vcl_stringstream ss;
  ss << time_window_;
  vcl_string tw;
  ss >> tw;
  config_.add_parameter( "time_window", tw, "The number of frames a tracklet is consider before the link is finalized" );
}

config_block 
track_linking_process
::params() const
{
  return config_;
}

bool
track_linking_process
::set_params( config_block const& blk )
{

  try
  {
    //Get parameters for functors
    disabled_ = blk.get<bool>("disabled");
    time_window_ = blk.get<unsigned int>("time_window");
    double trac_term_cost = blk.get<double>( "track_termination_cost" );
    double impossible_cost = blk.get<double>( "impossible_match_cost" );
    vcl_string assignment_algorithm = blk.get<vcl_string>( "assignment_algorithm" );
    //kinematic parameters
    double kin_sigma = blk.get<double>( "kinematic_track_linking_functor:sigma" );
    double kin_weight = blk.get<double>( "kinematic_track_linking_functor:weight" );
    double kin_direction_penalty = blk.get<double>("kinematic_track_linking_functor:direction_penalty");
    double kin_exp_transition_point_distance =
      blk.get<double>("kinematic_track_linking_functor:exp_transition_point_distance");
    //visual parameters
    double vis_weight = blk.get<double>("visual_track_linking_functor:weight");
    double vis_area_penalty = blk.get<double>("visual_track_linking_functor:area_penalty");
    double vis_mean_dist_penalty = blk.get<double>("visual_track_linking_functor:mean_dist_penalty");
    double vis_chi_penalty = blk.get<double>("visual_track_linking_functor:chi_penalty");
    //following parameters
    double follow_weight = blk.get<double>("following_track_linking_functor:weight");
    double follow_bbox_size_sigma = blk.get<double>("following_track_linking_functor:bbox_size_sigma");
    double follow_speed_sigma = blk.get<double>("following_track_linking_functor:speed_sigma");
    double follow_time_sigma = blk.get<double>("following_track_linking_functor:time_sigma");
    double follow_direction_sigma = blk.get<double>("following_track_linking_functor:direction_sigma");
    double follow_pred_dist_sigma = blk.get<double>("following_track_linking_functor:pred_dist_sigma");
    //overlap_track_linking_functor
    double overlap_weight = blk.get<double>("overlap_track_linking_functor:weight");
    double overlap_bbox_size_sigma = blk.get<double>("overlap_track_linking_functor:bbox_size_sigma");
    double overlap_direction_sigma = blk.get<double>("overlap_track_linking_functor:direction_sigma");
    double overlap_sigma = blk.get<double>("overlap_track_linking_functor:sigma");
    double overlap_mean = blk.get<double>("overlap_track_linking_functor:mean");
    double overlap_distance_sigma = blk.get<double>("overlap_track_linking_functor:distance_sigma");
    double overlap_bbox_not_overlap_sigma = blk.get<double>("overlap_track_linking_functor:bbox_not_overlap_sigma");
    double overlap_max_min_distance = blk.get<double>("overlap_track_linking_functor:max_min_distance");

    //Create functors
    vcl_vector<track_linking_functor_sptr> funs;
    vcl_vector< double > wgts;
    //kinematic
    track_linking_functor_sptr kin = new kinematic_track_linking_functor( kin_sigma,
                                                                          kin_direction_penalty,
                                                                          kin_exp_transition_point_distance );
    kin->set_impossible_match_cost(impossible_cost);
    wgts.push_back(kin_weight);
    funs.push_back(kin);
    //visual
    track_linking_functor_sptr
      visual = new visual_track_linking_functor( vis_area_penalty,
                                                 vis_mean_dist_penalty,
                                                 vis_chi_penalty );
    visual->set_impossible_match_cost(impossible_cost);
    funs.push_back(visual);
    wgts.push_back(vis_weight);
    //following 
    track_linking_functor_sptr follow = new following_track_linking_functor(follow_bbox_size_sigma,
                                                                             follow_speed_sigma,
                                                                             follow_time_sigma,
                                                                             follow_direction_sigma,
                                                                             follow_pred_dist_sigma);
    follow->set_impossible_match_cost(impossible_cost);
    wgts.push_back(follow_weight);
    funs.push_back(follow);
    //overlap
    track_linking_functor_sptr overlap = new overlap_track_linking_functor(overlap_bbox_size_sigma,
                                                                       overlap_direction_sigma,
                                                                       overlap_sigma,
                                                                       overlap_mean,
                                                                       overlap_distance_sigma,
                                                                       overlap_bbox_not_overlap_sigma,
                                                                       overlap_max_min_distance);
    overlap->set_impossible_match_cost(impossible_cost);
    wgts.push_back(overlap_weight);
    funs.push_back(overlap);

    //Push functors into one mulifunctor
    multi_track_linking_functors_functor fun(funs, wgts,impossible_cost);
    track_linker_.set_track_match_functor(fun);
    track_linker_.set_track_termination_cost(trac_term_cost);
    if(!track_linker_.set_assignment_algorithm_type( assignment_algorithm ))
    {
      log_error( name() << ": invalid assignment algorithm\n" );
      return false;
    }
  }
  catch( vidtk::unchecked_return_value& )
  {
    log_error( name() << ": couldn't set parameters\n" );
    return false;
  }
  config_.update( blk );

  return true;
}

void 
track_linking_process
::set_timestamp( timestamp const& ts )
{
  timestamp_ = &ts;
}

bool
track_linking_process
::outside_time(track_sptr track)
{
  return track != NULL && timestamp_ != NULL &&
         timestamp_->frame_number() - track->last_state()->time_.frame_number() >= time_window_;
}

bool
track_linking_process
::initialize()
{
  return true;
}

bool
operator<( track_linking_process::current_link const & l,
           track_linking_process::current_link const & r )
{
  return l.self_->last_state()->time_.frame_number() < r.self_->last_state()->time_.frame_number();
}

bool
track_linking_process
::step()
{
  if(disabled_)
  {
    bool temp = !flush_;
    flush_ = true;
    return temp;
  }
  output_tracks_.clear();
  if(terminate_)
  {
    return false;
  }
  //TODO:  Save some information between linkings
  vcl_vector< track_sptr > linkable_rows;
  vcl_vector< track_sptr > linkable_cols;
  //link all current potential.  We will chech for outside time from afterward.  This allows
  //track linking to be called intermittedly, not every iteration.
  if(tracks_were_added_since_last_time_)
  {
    for( vcl_map< unsigned int, current_link >::iterator iter = input_tracklets_to_link_.begin();
         iter != input_tracklets_to_link_.end();
         ++iter )
    {
      if(iter->second.is_finalized_track_)
      {
        linkable_rows.push_back(iter->second.self_);
        log_assert(iter->first == iter->second.self_->id(), "ID needs to equal KEY\n");
      }
      if(!(iter->second.is_fixed_))
      {
        linkable_cols.push_back(iter->second.self_);
      }
    }

    if(linkable_rows.size() != 0)
    {
      vcl_vector< track_linking_result > link_results = track_linker_.link( linkable_rows, linkable_cols );
      for( unsigned int i = 0; i < link_results.size(); ++i )
      {
        unsigned int from = link_results[i].src_track_id;
        unsigned int to   = link_results[i].dst_track_id;
        vcl_map< unsigned int, current_link >::iterator from_iter = input_tracklets_to_link_.find(from);
        log_assert(from_iter != input_tracklets_to_link_.end(), "The linking tracks should be here\n");
        log_assert(from_iter->second.is_finalized_track_, "The track here should be a finalized track\n");
        //Set the link information
        if(link_results[i].is_assigned())
        {
          unsigned int old_next = from_iter->second.next_id_;
          from_iter->second.next_id_ = to;
          vcl_map< unsigned int, current_link >::iterator to_iter = input_tracklets_to_link_.find(to);
          log_assert(to_iter != input_tracklets_to_link_.end(), "The linking tracks should be here\n");
          log_assert(!(to_iter->second.is_fixed_), "ERROR: No new previous link sould be made\n");
          unsigned int old_prev = to_iter->second.prev_id_;
          to_iter->second.prev_id_ = from;
          if( old_next != static_cast<unsigned int>(-1) && old_next != to )
          {
            vcl_map< unsigned int, current_link >::iterator oni = input_tracklets_to_link_.find(old_next);
            log_assert(oni != input_tracklets_to_link_.end(), "The linking tracks should be here\n");
            oni->second.prev_id_ = static_cast<unsigned int>(-1);
          }
          if( old_prev != static_cast<unsigned int>(-1) && from != old_prev )
          {
            vcl_map< unsigned int, current_link >::iterator opi = input_tracklets_to_link_.find(old_prev);
            log_assert(opi != input_tracklets_to_link_.end(), "The linking tracks should be here\n");
            opi->second.next_id_ = static_cast<unsigned int>(-1);
          }
        }
        else  //These is no link, set next to no link
        {
          unsigned int old_next = from_iter->second.next_id_;
          if(old_next != static_cast<unsigned int>(-1))
          {
            from_iter->second.next_id_ = static_cast<unsigned int>(-1);
            vcl_map< unsigned int, current_link >::iterator oni = input_tracklets_to_link_.find(old_next);
            log_assert(oni != input_tracklets_to_link_.end(), "The linking tracks should be here\n");
            oni->second.prev_id_ = static_cast<unsigned int>(-1);
          }
        }
      }
    }
  }
  if(flush_)
  {
    time_window_ = 0;
    this->update_output();
    terminate_ = true;
    return true;
  }
  else
  {
    this->update_output();
  }
  flush_ = true;
  tracks_were_added_since_last_time_ = false;
  return true;
}

vcl_vector< track_sptr > const &
track_linking_process
::get_output_tracks() const
{
  return output_tracks_;
}

vcl_vector< track_linking_process::current_link > const &
track_linking_process
::get_output_link_info() const
{
  return output_link_info_;
}

void
track_linking_process
::set_input_terminated_tracks( vcl_vector< track_sptr > const& in )
{
  flush_ = false;
  if(disabled_)
  {
    output_tracks_ = in;
    return;
  }
  for( unsigned int i = 0; i < in.size(); ++i )
  {
    vcl_map< unsigned int, current_link >::iterator ittli = input_tracklets_to_link_.find(in[i]->id());
    if(ittli == input_tracklets_to_link_.end())
    {
      assert(false && "It should have already been added.\n" );
      vcl_pair< unsigned int, current_link> pair(in[i]->id(),current_link(in[i]));
      pair.second.is_finalized_track_ = true;
      input_tracklets_to_link_.insert(pair);
      tracks_were_added_since_last_time_ = true;
    }
    else
    {
      log_assert(ittli->second.self_ == in[i], "The track should be same between these to\n");
      ittli->second.is_finalized_track_ = true;
      tracks_were_added_since_last_time_ = true;
    }
  }
}

void
track_linking_process
::set_input_active_tracks( vcl_vector< track_sptr > const& in )
{
  flush_ = false;
  if(disabled_)
  {
    output_tracks_ = in;
    return;
  }
  for( unsigned int i = 0; i < in.size(); ++i )
  {
    vcl_map< unsigned int, current_link >::iterator ittli = input_tracklets_to_link_.find(in[i]->id());
    if(ittli == input_tracklets_to_link_.end())
    {
      vcl_pair< unsigned int, current_link> pair(in[i]->id(),current_link(in[i]));
      pair.second.is_finalized_track_ = false;
      input_tracklets_to_link_.insert(pair);
      tracks_were_added_since_last_time_ = true;
    }
    else
    {
      log_assert(ittli->second.self_ == in[i], "The track should be same between these to\n");
      log_assert(!(ittli->second.is_finalized_track_), "The track should not be marked as finalized track\n");
    }
  }
}

void
track_linking_process
::update_output()
{
  // look to see if what links should be solidified
  vcl_vector< current_link > to_remove;
  //Identify the tracklets to be removed;
  for( vcl_map< unsigned int, current_link >::iterator iter = input_tracklets_to_link_.begin();
       iter != input_tracklets_to_link_.end();
       ++iter )
  {
    if( this->outside_time(iter->second.self_) )
    {
      to_remove.push_back(iter->second);
      this->output_link_info_.push_back(iter->second);
    }
  }
  // sort by time
  vcl_sort( to_remove.begin(), to_remove.end() );
  for( vcl_vector< current_link >::iterator iter = to_remove.begin();
       iter != to_remove.end(); ++iter )
  {
    input_tracklets_to_link_.erase(iter->self_->id()); // Clean up
    track_sptr built_track;
    if(iter->is_start())
    {
      built_track = iter->self_;
    }
    else
    {
      vcl_map< unsigned int, track_sptr >::iterator cti =
        constructed_tracks_.find(iter->prev_id_);
      log_assert( cti != constructed_tracks_.end(),
                  "I am assuming that the previous track will be"
                  " put in here before the next one." );
      built_track = cti->second;
      constructed_tracks_.erase(cti);
      built_track->append_track(*(iter->self_));
    }
    // Test to see if it is a termination
    if(iter->is_terminated())
    {
      output_tracks_.push_back(built_track);
    }
    else
    {
      vcl_map< unsigned int, current_link >::iterator ittli = 
        input_tracklets_to_link_.find(iter->next_id_);
      log_assert( ittli != input_tracklets_to_link_.end(), 
                  "The next id should alway be there" );
      ittli->second.is_fixed_ = true;
      constructed_tracks_[iter->self_->id()] = built_track;
    }
  }
}

vcl_vector< track_sptr >
track_linking_process
::get_current_linking()
{
  vcl_vector< current_link > need_to_add_tracks;
  vcl_vector< track_sptr > result;
  for( vcl_map< unsigned int, current_link >::iterator iter = input_tracklets_to_link_.begin();
       iter != input_tracklets_to_link_.end();
       ++iter )
  {
    need_to_add_tracks.push_back(iter->second);
  }
  vcl_map< unsigned int, track_sptr > constr_trks;
  for( vcl_map< unsigned int, track_sptr >::iterator iter = constructed_tracks_.begin();
       iter != constructed_tracks_.end();
       ++iter )
  {
    constr_trks[iter->first] = iter->second->clone();
  }
  // sort by time
  vcl_sort( need_to_add_tracks.begin(), need_to_add_tracks.end() );
  for( vcl_vector< current_link >::iterator iter = need_to_add_tracks.begin();
       iter != need_to_add_tracks.end(); ++iter )
  {
    track_sptr built_track;
    if(iter->is_start())
    {
      built_track = iter->self_;
    }
    else
    {
      vcl_map< unsigned int, track_sptr >::iterator cti =
        constr_trks.find(iter->prev_id_);
      log_assert( cti != constr_trks.end(),
                  "I am assuming that the previous track will be"
                  " put in here before the next one." );
      built_track = cti->second;
      constr_trks.erase(cti);
      built_track->append_track(*(iter->self_));
    }
    // Test to see if it is a termination
    if(iter->is_terminated())
    {
      result.push_back(built_track);
    }
    else
    {
      vcl_map< unsigned int, current_link >::iterator ittli =
        input_tracklets_to_link_.find(iter->next_id_);
      log_assert( ittli != input_tracklets_to_link_.end(),
                  "The next id should alway be there" );
      ittli->second.is_fixed_ = true;
      constr_trks[iter->self_->id()] = built_track;
    }
  }
  return result;
}

} // namespace vidtk

