/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "da_tracker_process.h"

#include <vcl_cassert.h>
#include <vcl_cmath.h>
#include <vcl_limits.h>
#include <vcl_iostream.h>
#include <vcl_algorithm.h>

#include <vnl/vnl_math.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_2x2.h>
#include <vnl/vnl_det.h>
#include <vnl/vnl_inverse.h>
#include <vnl/vnl_hungarian_algorithm.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_intersection.h>
#include <vil/vil_convert.h>

#include <utilities/log.h>
#include <utilities/greedy_assignment.h>
#include <utilities/unchecked_return_value.h>
#include <tracking/track_initializer_process.h>
#include <tracking/tracking_keys.h>
#include <tracking/tracker_cost_func_kin.h>
#include <tracking/tracker_cost_func_color_size_kin_amhi.h>
#include <tracking/fg_matcher.h>

#ifdef PRINT_DEBUG_INFO
#include <tracking/debug_tracker.h>
#endif

namespace vidtk
{

da_tracker_process
::da_tracker_process( vcl_string const& name )
  : process( name, "da_tracker_process" ),
    INF_(vcl_numeric_limits<double>::infinity()),
    gate_sigma_sqr_( 1.0 ),
    term_missed_count_( 0 ),
    term_missed_secs_( 0.0 ),
    tracker_cost_function_(NULL),
    assignment_alg_( HUNGARIAN ),
    loc_type_(centroid),
    amhi_enabled_( false ),
    amhi_update_objects_( false ),
    fg_tracking_enabled_( false ),
    fg_min_misses_( 0 ),
    fg_max_mod_lag_( 0.0 ),
    fg_predict_forward_( true ),
    amhi_tighter_bbox_frac_(-1.0),
    area_window_size_(5),
    area_weight_decay_rate_(0.2),
    min_speed_sqr_for_da_( 0.0 ),
    min_area_similarity_for_da_(0.0),
    min_color_similarity_for_da_(0.0),
    reassign_track_ids_( true ),
    min_bbox_overlap_percent_( 0.25 ),
    mods_( NULL ),
    cur_ts_( NULL ),
    image_( NULL ),
    mf_params_( NULL ),
    wld2img_H_( NULL ),
    img2wld_H_( NULL ),
    fg_objs_( NULL ),
    updated_active_tracks_( NULL ),
    new_trackers_( NULL ),
    next_track_id_( 1 ),
    track_id_offset_( 0 ),
    amhi_( NULL ),
    train_file_self_(),
    world_units_per_pixel_( 0.0 ),
    search_both_locations_( true )
{
  mod_cov_.set_identity();

  //The measurement noise covariance of reference point in meter square.
  //But it should be changed to pixel squre, with source/image to world plane mapping."
  config_.add_parameter( "measurement_noise_covariance", 
                         "1 0   0 1",
                         "The measurement noise covariance of reference point in meter square.");
  config_.add( "gate_sigma", "3" );
  config_.add( "terminate:missed_count", "1000000" );
  config_.add( "terminate:missed_seconds", "1000000" );
  config_.add( "assignment_algorithm", "hungarian" );
  config_.add_parameter( "location_type",
                         "centroid",
                         "Location of the target for tracking: bottom or centroid. "
                         "This parameter is used in conn_comp_sp:conn_comp, tracking_sp:tracker, and tracking_sp:state_to_image");
  config_.add_parameter( "starting_track_id", "1",
                         "Track will be numbered consecutively starting from this number.");
  config_.add_parameter( "min_speed_for_da", "0",
                         "We only use fg tracking under this speed." );
  config_.add_parameter( "reassign_track_ids", "true",
                         "This process will over-write the track ids of the new incoming tracks." );
  config_.add_parameter( "min_bbox_overlap_percent", "0.25",
                         "Minimum percentage of intersection bounding box area "
                         "used to determine overlap between a track and a MOD." );
  config_.add_parameter( "area_window_size", "10",
                         "Up to this num of frames in history is used to compute area feature");
  config_.add_parameter( "area_weight_decay_rate", "0.2",
                         "The weight of each frame is this much less than the next frame "
                         "in history for computing area feature." );
  config_.add_parameter( "min_area_similarity_for_da", "0.0",
                         "Min area similarity between an object and the tracked target"
                         "in history for computing area feature." );
  config_.add_parameter( "min_color_similarity_for_da", "0.0",
                         "Min color similarity between an object and the tracked target");

  // Aligned Motion History Image (AMHI) related parameters.
  config_.add( "amhi:enabled", "false" );
  config_.add( "amhi:update_objects", "false" );
  config_.add( "amhi:alpha", "0.1" );
  config_.add( "amhi:padding_factor", "0.5" );
  config_.add( "amhi:w_bbox_online", "0.1" );
  config_.add( "amhi:remove_zero_thr", "0.001" );
  config_.add( "amhi:max_valley_width", "0.4" );
  config_.add( "amhi:min_valley_depth", "0.2" );
  config_.add( "amhi:use_weights", "true" );
  config_.add( "amhi:max_bbox_side", "200" );
  config_.add( "amhi:tighter_bbox_frac", "-1.0" );

  // Fore-ground tracking related parameters.
  config_.add_parameter( "fg_tracking:enabled", "false",
                         "Whether to perform foreground tracking when data-association "
                         "tracking fails." );
  config_.add_parameter( "fg_tracking:min_misses", "1",
                         "Min number of misses by the da_tracker after which the foreground"
                         " tracker kicks in. NOTE: min_misses should > 0.");
  config_.add_parameter( "fg_tracking:max_time_since_mod", "2",
                         "Units seconds and fall back to frames when seconds not available.");
  config_.add_parameter( "fg_tracking:predict_forward", "true",
                         "True, use the Kalman predicted location.  "
                         "False, use the previous location" );
  config_.add_parameter( "fg_tracking:search_both_locations", "true",
                         "Causes the algorithm to look for the forground between the "
                         "previous location and the predicted location." );
  config_.add_parameter( "fg_tracking:type", "ssd",
                         "ssd, csurf, or loft. Specific algorithm used for "
                         "foreground matching." );
}

da_tracker_process
::~da_tracker_process()
{
}

config_block
da_tracker_process
::params() const
{
  return config_;
}


bool
da_tracker_process
::set_params( config_block const& blk )
{

  try
  {
    blk.get( "area_window_size", area_window_size_ );
    blk.get( "area_weight_decay_rate", area_weight_decay_rate_ );
    blk.get( "min_area_similarity_for_da", min_area_similarity_for_da_ );
    blk.get( "min_color_similarity_for_da", min_color_similarity_for_da_ );

    blk.get( "measurement_noise_covariance", mod_cov_ );
    double sigma;
    blk.get<double>( "gate_sigma", sigma );
    gate_sigma_sqr_ = sigma * sigma;
    blk.get( "terminate:missed_count", term_missed_count_ );
    blk.get( "terminate:missed_seconds", term_missed_secs_ );
    vcl_string assn_alg;
    blk.get( "assignment_algorithm", assn_alg );
    if( assn_alg == "greedy" )
    {
      assignment_alg_ = GREEDY;
    }
    else if( assn_alg == "hungarian" )
    {
      assignment_alg_ = HUNGARIAN;
    }
    else
    {
      log_error( "Unknown assignment algorithm " << assn_alg << "\n" );
      throw unchecked_return_value( "unknown assignment algorithm" );
    }

    vcl_string loc = blk.get<vcl_string>( "location_type" );
    if( loc == "centroid" )
    {
      loc_type_ = centroid;
    }
    else if( loc == "bottom" )
    {
      loc_type_ = bottom;
    }
    else
    {
      log_error( "Invalid location type \"" << loc << "\". Expect \"centroid\" or \"bottom\".\n" );
      throw unchecked_return_value( "unknown tracking location type" );
    }

    blk.get( "starting_track_id", track_id_offset_ );
    double min_speed_for_da = blk.get<double>( "min_speed_for_da" );
    min_speed_sqr_for_da_ = min_speed_for_da * min_speed_for_da;
    blk.get( "reassign_track_ids", reassign_track_ids_ );
    blk.get( "min_bbox_overlap_percent", min_bbox_overlap_percent_ );

    // Aligned Motion History Image (AMHI) related parameters.
    blk.get( "amhi:enabled", amhi_enabled_ );
    blk.get( "amhi:update_objects", amhi_update_objects_);

    double alpha = blk.get<double>( "amhi:alpha" );
    double padding = blk.get<double>( "amhi:padding_factor" );
    double weight_bbox = blk.get<double>( "amhi:w_bbox_online" );
    double almost_zero = blk.get<double>( "amhi:remove_zero_thr" );
    double max_frac_valley_width = blk.get<double>( "amhi:max_valley_width" );
    double min_frac_valley_depth = blk.get<double>( "amhi:min_valley_depth" );
    bool use_weights = blk.get<bool>( "amhi:use_weights" );
    unsigned max_bbox_side = blk.get<unsigned>( "amhi:max_bbox_side" );

    amhi_ = new amhi<vxl_byte>( alpha,
                                padding,
                                weight_bbox,
                                almost_zero,
                                max_frac_valley_width,
                                min_frac_valley_depth,
                                use_weights,
                                max_bbox_side );

    blk.get( "amhi:tighter_bbox_frac", amhi_tighter_bbox_frac_ );

    blk.get( "fg_tracking:enabled", fg_tracking_enabled_ );
    if( fg_tracking_enabled_ )
    {
      blk.get( "fg_tracking:min_misses", fg_min_misses_ );
      blk.get( "fg_tracking:max_time_since_mod", fg_max_mod_lag_ );
      blk.get( "fg_tracking:predict_forward", fg_predict_forward_ );
      blk.get( "fg_tracking:search_both_locations", search_both_locations_ );
    } // if fg_tracking_enabled_
  }
  catch( unchecked_return_value& e )
  {
    log_error( name() << ": set_params() failed because, "<< e.what() << "\n" );
    return false;
  }

  if(mf_params_ && mf_params_->train_enabled )
  {
    tracker_cost_function_ =
      new tracker_cost_func_color_size_kin_amhi_training( mod_cov_,
                                                          gate_sigma_sqr_,
                                                          mf_params_ );
  }
  else if(mf_params_ && mf_params_->test_enabled )
  {
    tracker_cost_function_ =
      new tracker_cost_func_color_size_kin_amhi( mod_cov_,
                                                 gate_sigma_sqr_,
                                                 mf_params_,
                                                 area_window_size_,
                                                 area_weight_decay_rate_,
                                                 min_area_similarity_for_da_,
                                                 min_color_similarity_for_da_);
  }
  else
  {
    tracker_cost_function_ = new tracker_cost_func_kin( mod_cov_, gate_sigma_sqr_);
  }

  config_.update( blk );
  return true;
}


bool
da_tracker_process
::initialize()
{
  mods_ = NULL;
  cur_ts_ = NULL;

  return true;
}


// ----------------------------------------------------------------
/** Main processing loop
 *
 *
 */
bool
da_tracker_process
::step()
{
  // Verify that we have our pre-requisites.
  vcl_string error_msg;
  if( mods_ == NULL )
  {
    error_msg += "MODs were not supplied. ";
  }
  if( cur_ts_ == NULL )
  {
    error_msg += "Timestamp was not supplied. ";
  }
  if( ( amhi_enabled_ || fg_tracking_enabled_ )
      && (image_ == NULL) )
  {
    error_msg += "Current image was not supplied.";
  }
  if( updated_active_tracks_ == NULL )
  {
    error_msg += "Updated active tracks not supplied.";
  }
  if( new_trackers_ == NULL )
  {
    error_msg += "New trackers not supplied.";
  }

  // If we don't have the data to track, but we do have active tracks,
  // terminate them and return that state. Otherwise, we can just end.
  //
  if( ! error_msg.empty() )
  {
    log_info( this->name() << ": " << error_msg << "\n" );
    if( (! this->tracks_.empty()) || (! this->new_tracks_.empty()) )
    {
      log_info( this->name() << ": terminating "<< this->tracks_.size()
                <<" active tracks ("<< this->new_tracks_.size()
                << " new tracks on next cycle)\n" );
      this->terminate_all_tracks();

      // Move new tracks into active tracks for next cycle.  This is
      // needed to handle the case where new tracks are introduced at
      // the step where the tracker is terminating. The new tracks
      // must be presented to the active tracks port before they are
      // went to the terminated tracks list.
      tracks_ = new_tracks_;
      if( new_trackers_ )
      {
        trackers_ = *new_trackers_;
      }
      new_trackers_ = NULL;
      wh_trackers_ = new_wh_trackers_;

      new_tracks_.clear();
      new_wh_trackers_.clear();
      updated_active_tracks_  = NULL;

      return true;
    }
    else
    {
      // Clear the terminated tracks, for backward compatibility with older code that
      // may try to flush some stuff at the end.
      terminated_tracks_.clear();
      terminated_trackers_.clear();
      terminated_wh_trackers_.clear();
      log_info( this->name() << ": no active tracks. Cannot process any more\n" );
      return false;
    }
  }

#ifdef PRINT_DEBUG_INFO
  log_debug( this->name() << ": Processing frame " << *(this->cur_ts_) << "\n" );
#endif

  tracks_ = *updated_active_tracks_;
  //
  // All prerequisites have been verified. Append new entires to our
  // current ones.
  //
  tracks_.insert ( tracks_.end(), new_tracks_.begin(), new_tracks_.end() );
  trackers_.insert ( trackers_.end(), new_trackers_->begin(), new_trackers_->end() );
  wh_trackers_.insert( wh_trackers_.end(), new_wh_trackers_.begin(), new_wh_trackers_.end() );
  new_tracks_.clear();
  new_trackers_ = NULL;
  new_wh_trackers_.clear();

  if( fg_objs_ )
  {
    updated_fg_objs_ = *fg_objs_;
  }
  else
  {
    updated_fg_objs_.clear();
  }

  //TODO: This could not be done in the initialize() because the
  //filename is coming from the MF process node. Is there a way
  //to ensure receipt of data before starting off this node?

  //Open files for writing training data.
  if( mf_params_
      && mf_params_->train_enabled
      && !train_file_self_.is_open() )
  {
    vcl_string fname( mf_params_->train_filename.c_str() );
    fname.append( "_self.txt" );
    train_file_self_.open( fname.c_str() );

    if( ! train_file_self_ )
    {
      log_debug( this->name() <<":Couldn't open file '"<< fname
        << "' for writing.\n" );
      return false;
    }
  }


  // Update all the trackers that we have.
  if( trackers_.empty() )
  {
    unassigned_mods_ = *mods_;
  }
  else
  {
    update_trackers();
  }

  // Terminate tracks that seem lost.
  terminate_trackers();

  // Mark that they have been used.
  mods_ = NULL;
  cur_ts_ = NULL;
  fg_objs_ = NULL;
  updated_active_tracks_ = NULL;

  return true;
}


vcl_vector< image_object_sptr > const&
da_tracker_process
::unassigned_objects() const
{
  return unassigned_mods_;
}


vcl_vector< track_sptr > const&
da_tracker_process
::active_tracks() const
{
  return tracks_;
}


vcl_vector< da_so_tracker_sptr > const&
da_tracker_process
::active_trackers() const
{
  return trackers_;
}

void
da_tracker_process
::set_mf_params( multiple_features const& mf )
{
  mf_params_ = &mf;
}

vcl_vector< track_sptr > const&
da_tracker_process
::terminated_tracks() const
{
  return terminated_tracks_;
}

vcl_vector< da_so_tracker_sptr > const&
da_tracker_process
::terminated_trackers() const
{
  return terminated_trackers_;
}


void
da_tracker_process
::set_source_image( vil_image_view<vxl_byte> const& im )
{
  image_ = &im;
}


void
da_tracker_process
::update_trackers()
{
  assert(tracker_cost_function_!=NULL);
  // First, all the trackers predict forward to this frame.  Then we
  // do a simple data association to give a single MOD to each
  // tracker. Finally, we update each tracker with the assigned MOD.

  unsigned const M = trackers_.size();
  unsigned const N = mods_->size();

  vcl_vector<bool> assigned_mods( N, false );
  for( unsigned mod_idx = 0; mod_idx < N; ++mod_idx )
  {
    assigned_mods[mod_idx] = (*mods_)[mod_idx]->data_.is_set(
      tracking_keys::used_in_track );
  }
  // The assignment cost is the -log likelihood of the measurement
  // (actual location) given the predicted position.
  vnl_matrix<double> cost( M, N );

  tracker_cost_function_->set_size(M,N);
  double max_cost = -tracker_cost_function_->inf();

  for( unsigned trk_idx = 0; trk_idx < M; ++trk_idx )
  {
    bool disable_da_tracking = false;
    if( fg_tracking_enabled_ &&
      trackers_[trk_idx]->get_current_velocity().squared_magnitude()
      < min_speed_sqr_for_da_ )
    {
#ifdef PRINT_DEBUG_INFO
      log_debug( this->name() <<": Disabling DA tracking for track: "
        << tracks_[trk_idx]->id()
        <<" with speed "<< trackers_[trk_idx]->get_current_velocity().magnitude()
        << vcl_endl );
#endif
      disable_da_tracking = true;
    }

    tracker_cost_function_->set(trackers_[trk_idx], tracks_[trk_idx], cur_ts_);

    for( unsigned mod_idx = 0; mod_idx < N; ++mod_idx )
    {
      tracker_cost_function_->set_location(trk_idx, mod_idx);
      double final_distance;

      if( disable_da_tracking || assigned_mods[mod_idx] )
        final_distance = tracker_cost_function_->inf();
      else
        final_distance = tracker_cost_function_->cost((*mods_)[mod_idx]);

      cost( trk_idx, mod_idx) = final_distance;

      if( final_distance != tracker_cost_function_->inf()
          && final_distance > max_cost )
      {
        max_cost = final_distance;
      }
    }// for mod_idx
  }// for trk_idx

  if( ! vnl_math::isfinite( max_cost ) )
  {
    // no point falls in a gate.  Set the max_cost to some arbitrary
    // value. Then, every assignment derived from the hungarian
    // algorithm will be rejected because the individual cost will
    // exceed max_cost.
    max_cost = 0;
  }

  // The vnl_hungarian_algorithm implementation cannot handle Inf.
  for( unsigned i = 0; i < M; ++i )
  {
    for( unsigned j = 0; j < N; ++j )
    {
      if( cost(i,j) > max_cost )  // is Inf
      {
        cost(i,j) = max_cost + 1;
      }
    }
  }


  log_debug( name() << ": computing assignment (" << cost.rows()
             << "x" << cost.cols() << ")\n" );

  vcl_vector<unsigned> assn;
  if( assignment_alg_ == HUNGARIAN )
  {
    assn = vnl_hungarian_algorithm<double>( cost );
  }
  else
  {
    assn = greedy_assignment( cost );
  }

  log_debug( name() << ": assignment computed" << vcl_endl );

  vcl_vector<unsigned> fg_matched;
  vcl_vector<image_object_sptr> fg_new_objs;

  for( unsigned i = 0; i < M; ++i )
  {
    unsigned const j = assn[i];

    if( j != unsigned(-1) && cost(i,j) <= max_cost ) //found a match
    {

#ifdef PRINT_DEBUG_INFO
      //if( DT_is_track_of_interest( tracks_[i]->id() ) )
      //{
      //  log_debug( this->name() << ":\n" );
      //}
      if( DT_is_track_of_interest( tracks_[i]->id() ) )
      {
        log_debug( this->name() << ": found DA match; MOD # "<< j <<"/"
          << N <<" (sptr: "<< (*mods_)[j] <<") for "<< *tracks_[i] );
      }
#endif

      image_object & obj = *(*mods_)[j];
      //vnl_double_2 loc( obj.world_loc_[0], obj.world_loc_[1] );
      //trackers_[i]->update( cur_ts_->time_in_secs(), loc, mod_cov_ );
      if( amhi_enabled_ && image_ )
      {
        //Added for online update of amhi
        amhi_->update_track_model( obj, *image_, *tracks_[i]);

        //In order to update the track & tracker positions with the
        //amhi bboxes.
        if( amhi_update_objects_  && wh_trackers_.empty() )
        { // Run amhi if width-height Kalman filter is not turned on
          vcl_vector< vgl_box_2d< unsigned> > boxes( 1, tracks_[i]->amhi_datum().bbox );
          bool res = update_obj( boxes,
                                 obj,
                                 *trackers_[i],
                                 tracks_[i]->amhi_datum().weight );
          if(!res)
          {
#ifdef PRINT_DEBUG_INFO
            log_debug( this->name() <<":update_obj() failed for bbox track: "
              << tracks_[i]->id() <<"\n" );
#endif
          }
        }
      }

      trackers_[i]->update( *cur_ts_, obj, mod_cov_ );
      if (!wh_trackers_.empty())
      {
        wh_trackers_[i]->update_wh( *cur_ts_, obj, mod_cov_);

        // get current width and height and modify obj
        update_bbox_wh(wh_trackers_[i]->get_current_width(),
                       wh_trackers_[i]->get_current_height(),obj );
      }
      assigned_mods[j] = true;

      append_state( *trackers_[i], *tracks_[i], (*mods_)[j] );

      tracks_[i]->set_last_mod_match( cur_ts_->time_in_secs() );

      tracker_cost_function_->output_training(i,j,train_file_self_);
    }
    else //missed
    {
#ifdef PRINT_DEBUG_INFO
      log_debug( this->name() <<": DA tracking failed " <<", track: "<< tracks_[i]->id()
        <<vcl_endl );
#endif
      if( fg_tracking_enabled_
          && ( (trackers_[i]->missed_count()+1) % fg_min_misses_ == 0 )
          && ( vcl_abs( cur_ts_->time_in_secs() - tracks_[i]->last_mod_match() ) < fg_max_mod_lag_ )  )
      {
#ifdef PRINT_DEBUG_INFO
        log_debug( this->name() <<": checking fg"
                 <<", track: "<< tracks_[i]->id()
                 <<", frame: "<<cur_ts_->frame_number()
                 <<", trkrs.mc: "<< (trackers_[i]->missed_count())
                 <<", min_val: "<<vcl_endl );
#endif
        image_object_sptr new_obj;
        //Perform foreground tracking.
        if( ! update_fg_tracks(tracks_[i], trackers_[i], new_obj) )
        {
#ifdef PRINT_DEBUG_INFO
          log_debug( this->name() <<":fg tracking failed for track: "
            << tracks_[i]->id() <<vcl_endl );
#endif
          trackers_[i]->update();
          if (!wh_trackers_.empty())
          {
            wh_trackers_[i]->update();
          }
        }
        else
        {
          fg_matched.push_back( i );
          if( new_obj )
          {
            fg_new_objs.push_back( new_obj );
          }
        }
      }
      else
      {
        // inform the tracker that they were considered for, but not
        // given, a MOD.
        trackers_[i]->update();
        if ( !wh_trackers_.empty() )
        {
          wh_trackers_[i]->update();
        }
      }
    }
  }

  //Try catchcing overlap b/w bboxes and mods
  //1. Update fg matched track only if doesn't 'double' overlap.
  //2. Flag the unused mods if it overlaps an amhi bbox.
  flag_mod_overlaps( assigned_mods, fg_matched, fg_new_objs );

  // Record the unassigned MODs for other processes, like the new
  // track initializer, to use.
  unassigned_mods_.clear();
  for( unsigned j = 0; j < N; ++j )
  {
    if( ! assigned_mods[j] )
    {
      unassigned_mods_.push_back( (*mods_)[j] );
    }
    else
    {
      (*mods_)[j]->data_.set( tracking_keys::used_in_track, true );
    }
  }
}


void
da_tracker_process
::flag_mod_overlaps( vcl_vector<bool> & assigned_mods,
                     vcl_vector<unsigned> const & fg_matched,
                     vcl_vector<image_object_sptr> const & fg_new_objs )
{

  //1 of 2
  //
  //For fg_tracking. Invalidate fg match if it is overlapping another
  //assigned mod.
  if( fg_tracking_enabled_ )
  {
    for( unsigned i = 0; i < fg_matched.size(); ++i )
    {
      bool overlaps = false;
      vgl_box_2d<unsigned> const & new_bbox = fg_new_objs[i]->bbox_;

      for( unsigned j = 0; j < updated_fg_objs_.size() && !overlaps; ++j )
      {
        if( this->boxes_intersect( updated_fg_objs_[j]->bbox_, new_bbox ) )
        {
          overlaps = true;
        }
      }

      for( unsigned j = 0; j < assigned_mods.size() && !overlaps; ++j )
      {
        image_object_sptr const & obj = (*mods_)[j];

        if( this->boxes_intersect( new_bbox, obj->bbox_ ) )
        {
          if( assigned_mods[j] )
          {
            overlaps = true;
          }
          else
          {
            assigned_mods[j] = true;
          }
        }
      } // for assigned_mods

      if( !overlaps )
      {
        //Passed the test. Did not have a double match any assigned mod.
        //update track/tracker
        unsigned idx = fg_matched[i];
        vnl_double_2 loc( fg_new_objs[i]->world_loc_[0],
                          fg_new_objs[i]->world_loc_[1] );
        vnl_double_2 loc_wh( fg_new_objs[i]->bbox_.width(),
                             fg_new_objs[i]->bbox_.height() );

        trackers_[idx]->update( cur_ts_->time_in_secs(), loc, mod_cov_ );
        image_object_sptr obj = fg_new_objs[i];
        if (!wh_trackers_.empty())
        {
          wh_trackers_[idx]->update( cur_ts_->time_in_secs(), loc_wh, mod_cov_ );
          // get current width and height and modify obj
          update_bbox_wh( wh_trackers_[i]->get_current_width(),
                          wh_trackers_[i]->get_current_height(),
                          *obj );
        }

        append_state( *trackers_[idx], *tracks_[idx], obj );

        updated_fg_objs_.push_back( fg_new_objs[i] );

        //We *do not* update the amhi model of the track because the
        //weight matrix will only be updated once we have the fg mask.
        //
        //We only update the new bounding box in the 'amhi_datum'.
        //Overwrite the bbox in the track_state that was just inserted
        //into the track.
        if(amhi_enabled_)
        {
          // TODO: This needs to change, because...
          // - bbox that needs to be updated is track::amhi_datum::bbox, not
          // track_state::amhi_bbox_ that is obsolete and is not being used
          // anymore.
          // - fg_new_objs[i]->bbox_ is usually a cropped version of the amhi
          // bbox, therefore an appropriate reverse *uncropping* needs to be
          // applied before setting the box
           tracks_[idx]->set_init_amhi_bbox( tracks_[idx]->history().size() - 1,
                                             obj->bbox_ );
        }
      }
      else
      {
        trackers_[fg_matched[i]]->update();
        if (!wh_trackers_.empty())
        {
          wh_trackers_[fg_matched[i]]->update();
        }

#ifdef PRINT_DEBUG_INFO
        log_debug( this->name() <<": fg_match: discarding due to overlap, track: "
          << tracks_[fg_matched[i]]->id() << vcl_endl );
#endif
      }
    }
  } // if( fg_tracking_enabled_ )

  //2 of 2
  //
  //Mark the unassigned mod's as assigned if they 'encapsulated' by an
  //AMHI bbox. Used for handling broken pieces of blob.
  //
  //TODO: Move this back into amhi update where it can be used to update the
  //amhi model.

  if( fg_tracking_enabled_ || amhi_enabled_ )
  {
    for( unsigned i = 0; i < tracks_.size(); ++i )
    {
      if( trackers_[i]->missed_count() == 0 )
      {
        for( unsigned j = 0; j < assigned_mods.size(); ++j )
        {
          if( !assigned_mods[j] )
          {
            vgl_box_2d<unsigned> const & new_trk_bbox = tracks_[i]->get_end_object()->bbox_;
            image_object_sptr const & unassn_obj = (*mods_)[j];

            if( this->boxes_intersect( new_trk_bbox, unassn_obj->bbox_ ) )
            {
              //Make this mod unusable for track initialization because this has
              //been completely 'encapsulated' by an AMHI bbox
              assigned_mods[j] = true;
  #ifdef PRINT_DEBUG_INFO
            log_debug( this->name() <<": flag_mod_overlaps: mod 'assigned' "
              "due to overlap, track: "<< tracks_[i]->id() << vcl_endl );
  #endif
            }
          }
        }
      }
    }
  }
}


void
da_tracker_process
::set_objects( vcl_vector< image_object_sptr > const& mods )
{
  mods_ = &mods;
}


void
da_tracker_process
::set_timestamp( timestamp const& ts )
{
  cur_ts_ = &ts;
}


void
da_tracker_process
::terminate_trackers()
{
  typedef  vcl_vector< da_so_tracker_sptr >::iterator iter0_type;
  typedef  vcl_vector< track_sptr >::iterator iter1_type;

  terminated_tracks_.clear();
  terminated_trackers_.clear();
  terminated_wh_trackers_.clear();
  iter0_type it0 = trackers_.begin();
  iter1_type it1 = tracks_.begin();
  iter0_type it2;
  if ( !wh_trackers_.empty() )
  {
    it2 = wh_trackers_.begin();
  }
  while( it0 != trackers_.end() )
  {
    assert( it1 != tracks_.end() );
    da_so_tracker const& trk = **it0;
    bool terminate = false;

    if( trk.missed_count() > term_missed_count_ )
    {
      (*it1)->data().get_or_create_ref<vcl_string>( "note" ) += "Terminated by missed count\n";
      terminate = true;
    }
    else if ( vcl_abs(cur_ts_->time_in_secs() - trk.last_update_time()) > term_missed_secs_ )
    {
      (*it1)->data().get_or_create_ref<vcl_string>( "note" ) += "Terminated by missed seconds\n";
      terminate = true;
    }

    if( terminate )
    {
#ifdef PRINT_DEBUG_INFO
      log_debug( this->name() << ": Terminating "<< *(*it1) << "\n" );
#endif

      terminated_tracks_.push_back( *it1 );
      terminated_trackers_.push_back( *it0 );
      if ( !wh_trackers_.empty() )
      {
        terminated_wh_trackers_.push_back( *it2 );
      }
      it0 = trackers_.erase( it0 );
      it1 = tracks_.erase( it1 );
      if ( !wh_trackers_.empty() )
      {
        it2 = wh_trackers_.erase( it2 );
      }
    }
    else
    {
      ++it0;
      ++it1;
      if ( !wh_trackers_.empty() )
      {
        ++it2;
      }
    }
  }
}


void
da_tracker_process
::append_state( da_so_tracker& tracker, track& trk, image_object_sptr obj )
{
  track_state_sptr s = new track_state;
  tracker.get_current_location(s->loc_);
  tracker.get_current_velocity(s->vel_);

  // Add image velocity.
  if(!s->add_img_vel(*wld2img_H_))
  {
    log_error(this->name()<<"Fail to compute image velocity\n");
  }

  s->time_ = *cur_ts_;

  if( amhi_enabled_ )
  {
    s->amhi_bbox_ = trk.amhi_datum().bbox;
  }

  s->data_.set( tracking_keys::img_objs,
                vcl_vector< image_object_sptr >( 1, obj ) );
  trk.add_state( s );
}

bool
da_tracker_process
::update_fg_tracks( track_sptr track,
                    da_so_tracker_sptr tracker,
                    image_object_sptr & new_obj )
{
  new_obj = NULL;
  vnl_double_2 pred_pos_img;
  vnl_double_2 current_loc;

#ifdef PRINT_DEBUG_INFO
   g_DT_frame = cur_ts_->frame_number();
   g_DT_id = track->id();
#endif

  if( fg_predict_forward_  || search_both_locations_ )
  {
    //1. Predict the tracker position in the current frame.
    vnl_double_2 pred_pos_wld;
    vnl_double_2x2 pred_cov_wld;
    tracker->predict( cur_ts_->time_in_secs(), pred_pos_wld, pred_cov_wld );
    //2. Project the predicted 'world' position into the current frame.
    //pred_pos: world --> image
    vnl_double_3 wp( pred_pos_wld[0], pred_pos_wld[1], 1.0 );
    vnl_double_3 ip = wld2img_H_->get_matrix() * wp;
    pred_pos_img[0] = ip[0] / ip[2];
    pred_pos_img[1] = ip[1] / ip[2];
    current_loc = track->get_end_object()->img_loc_;
  }
  else
  {
    pred_pos_img = track->get_end_object()->img_loc_;
  }

  //predicted position lies outside image space
  if(  pred_pos_img[0] <= 0
    || pred_pos_img[1] <= 0
    || pred_pos_img[0] >= image_->ni()
    || pred_pos_img[1] >= image_->nj() )
  {
    return false;
  }

  //3. Currently performing min ssd based matching around img_pos.

  // We prepare the 'predicted bounding box' for ssd by taking
  // width and height from the last bounding in the track and
  // position from the predicted tracker position.
  //
  vgl_box_2d<unsigned> wh_bbox;

  if( amhi_enabled_ )
    wh_bbox = track->amhi_datum().bbox;
  else
    wh_bbox = track->get_end_object()->bbox_;

  vgl_box_2d<unsigned> pred_box;
  vgl_box_2d<unsigned> curr_box;

  if( loc_type_ == bottom ) //tracking bottom
  {
    pred_box.set_min_x( static_cast<unsigned int>(vcl_max( 0, int(pred_pos_img[0] - wh_bbox.width() / 2 ) ) ) );
    pred_box.set_min_y( static_cast<unsigned int>(vcl_max( 0, int(pred_pos_img[1] - wh_bbox.height() ) ) ) );

    pred_box.set_max_x( static_cast<unsigned int>(vcl_min( int(image_->ni()), int(pred_pos_img[0] + wh_bbox.width() / 2 ) ) ) );
    pred_box.set_max_y( static_cast<unsigned int>(vcl_min( int(image_->nj()), int(pred_pos_img[1] ) ) ) );

    if( search_both_locations_ )
    {
      curr_box.set_min_x( static_cast<unsigned int>(vcl_max( 0, int(current_loc[0] - wh_bbox.width() / 2 ) ) ) );
      curr_box.set_min_y( static_cast<unsigned int>(vcl_max( 0, int(current_loc[1] - wh_bbox.height() ) ) ) );

      curr_box.set_max_x( static_cast<unsigned int>(vcl_min( int(image_->ni()), int(current_loc[0] + wh_bbox.width() / 2 ) ) ) );
      curr_box.set_max_y( static_cast<unsigned int>(vcl_min( int(image_->nj()), int(current_loc[1] ) ) ) );
    }
  }
  else //tracking centroid
  {
    pred_box.set_min_x( static_cast<unsigned int>(vcl_max( 0, int(pred_pos_img[0] - wh_bbox.width() / 2 ) )) );
    pred_box.set_min_y( static_cast<unsigned int>(vcl_max( 0, int(pred_pos_img[1] - wh_bbox.height() / 2 ) )) );

    pred_box.set_max_x( static_cast<unsigned int>(vcl_min( int(image_->ni()), int(pred_pos_img[0] + wh_bbox.width() / 2 ) )) );
    pred_box.set_max_y( static_cast<unsigned int>(vcl_min( int(image_->nj()), int(pred_pos_img[1] + wh_bbox.height() / 2 ) )) );

    if( search_both_locations_ )
    {
      curr_box.set_min_x( static_cast<unsigned int>(vcl_max( 0, int(current_loc[0] - wh_bbox.width() / 2 ) )) );
      curr_box.set_min_y( static_cast<unsigned int>(vcl_max( 0, int(current_loc[1] - wh_bbox.height() / 2 ) )) );

      curr_box.set_max_x( static_cast<unsigned int>(vcl_min( int(image_->ni()), int(current_loc[0] + wh_bbox.width() / 2 ) )) );
      curr_box.set_max_y( static_cast<unsigned int>(vcl_min( int(image_->nj()), int(current_loc[1] + wh_bbox.height() / 2 ) )) );
    }
  }

  if( search_both_locations_ )
  {
    pred_box.add( curr_box );
  }

  vcl_vector< vgl_box_2d<unsigned> > out_bboxes;

  // Run the foreground matching function and retrieve one or more boxes.
  bool match_found = false;
  fg_matcher< vxl_byte >::sptr_t fgm;
  if( track->data().has( vidtk::tracking_keys::foreground_model ) )
  {
    track->data().get( vidtk::tracking_keys::foreground_model, fgm );
    match_found = fgm->find_matches( track, pred_box, *image_, out_bboxes );
  }

#ifdef PRINT_DEBUG_INFO
  //if(DT_is_track_of_interest(track->id()))
  {
    log_debug( this->name() <<":For Track: " << track->id() << " there were "
             << out_bboxes.size() << " minima." << vcl_endl );
  }
#endif

  //Return if we did not find a foreground match.
  if( !match_found )
  {
#ifdef PRINT_DEBUG_INFO
    log_debug( this->name() <<": fg match not found, track: "<< track->id()<<vcl_endl );
#endif
    return false;
  }

  // Prepare image_object for the current frame.
  new_obj = new image_object();

  //4. Back project the location of the best match (if any) to the
  //   world_pos.
  bool res;
  if(amhi_enabled_ && wh_trackers_.empty())
  {
    res = update_obj( out_bboxes, *new_obj, *tracker, track->amhi_datum().weight );
  }
  else
  {
    res = update_obj( out_bboxes, *new_obj, *tracker );
  }

#ifdef PRINT_DEBUG_INFO
  if(!res)
  {
    log_debug( this->name() <<":update_obj() failed for fg track: "<< track->id() <<"\n" );
  }
#endif

  return res;

	//5. Update the tracker and track objects.
  //Moving out into update_trackers() because of checking 'wrong' matches.
}


void
da_tracker_process
::set_image2world_homography( vgl_h_matrix_2d<double> const& H )
{
  img2wld_H_ = &H;
}


void
da_tracker_process
::set_world2image_homography( vgl_h_matrix_2d<double> const& H )
{
  wld2img_H_ = &H;
}


//  Back project the location of the best match (if any) to the
//   world_pos. This position will be used as the new location in
//   'tracker' and 'track'.
bool
da_tracker_process
::update_obj( vcl_vector< vgl_box_2d<unsigned> > const & bboxes,
              image_object & obj,
              da_so_tracker const & tracker,
              vil_image_view<float> const & amhi_weight )
{
  double best_dist = gate_sigma_sqr_;
  for( unsigned int i = 0; i < bboxes.size(); ++i )
  {
    vgl_box_2d<unsigned> const & bbox = bboxes[i];
    vgl_box_2d<unsigned> tighter_bbox;

    // If point at a valid weight matrix and want to get a tighter box.
    if( !!amhi_weight && amhi_tighter_bbox_frac_ >= 0 )
    {
      amhi<vxl_byte>::get_tighter_bbox( amhi_weight,
                                        amhi_tighter_bbox_frac_,
                                        tighter_bbox,
                                        bbox );
    }
    else
    {
      tighter_bbox = bbox;
    }

    vnl_double_3 ip;
    vnl_double_3 wp;

    if( loc_type_ == bottom ) //tracking bottom
    {
      ip[0] = tighter_bbox.centroid_x();
      ip[1] = tighter_bbox.max_y();
      ip[2] = 1;
    }
    else //tracking centroid
    {
      ip[0] = tighter_bbox.centroid_x();
      ip[1] = tighter_bbox.centroid_y();
      ip[2] = 1;
    }

    wp = img2wld_H_->get_matrix() * ip;
    vnl_double_2 new_pos_wld;
    new_pos_wld[0] = wp[0] / wp[2];
    new_pos_wld[1] = wp[1] / wp[2];

    //Apply gate on the position of the fg match
    vnl_double_2 pred_pos;
    vnl_double_2x2 pred_cov;
    tracker.predict( cur_ts_->time_in_secs(), pred_pos, pred_cov );

    // Add the uncertainty of the measurement.
    vnl_double_2x2 cov = pred_cov;
    cov += mod_cov_;

    // Assume z=0 in the measurements.
    vnl_double_2 delta( new_pos_wld );
    delta -= pred_pos;
    double mahan_dist_sqr;
    if(vnl_det(cov) != 0)
    {
      mahan_dist_sqr = dot_product( delta, vnl_inverse( cov ) * delta );
    }
    else
    {
      mahan_dist_sqr = INF_;
    }
    if( mahan_dist_sqr < best_dist )
    {
      best_dist = mahan_dist_sqr;
      obj.bbox_ = tighter_bbox;
      obj.img_loc_[0] = ip[0];
      obj.img_loc_[1] = ip[1];
      obj.world_loc_[0] = new_pos_wld[0];
      obj.world_loc_[1] = new_pos_wld[1];
      obj.world_loc_[2] = 0;
      obj.area_ = tighter_bbox.area() * world_units_per_pixel_ * world_units_per_pixel_;
    }
  }
  return best_dist < gate_sigma_sqr_;
}

void
da_tracker_process
::reset_trackers_and_tracks()
{
  terminated_tracks_.clear();
  terminated_trackers_.clear();
  trackers_.clear();
  tracks_.clear();
  wh_trackers_.clear();
  terminated_trackers_.clear();

}


void
da_tracker_process
::update_bbox_wh( double width, double height, image_object &obj )
{
  double p1x, p1y, p2x, p2y;
  p1x = obj.bbox_.min_x();
  p1y = obj.bbox_.min_y();
  p2x = obj.bbox_.max_x();
  p2y = obj.bbox_.max_y();

  double cx, cy;

  // expand around center
  if ( 1 )
  {
    cx = (p1x + p2x)/2;
    cy = (p1y + p2y)/2;
    p1x = floor( cx - width/2 + .5 ); //.5 for rounding
    p2x = floor( cx + width/2 + .5 );
    p1y = floor( cy - height/2 + .5 );
    p2y = floor( cy + height/2 + .5 );
  }
  else //try expanding about bottom pt
  {
    cx = (p1x + p2x)/2;
    cy = p2y;
    p1x = floor( cx - width/2 +.5 );
    p1y = floor( cy - height + .5 );
    p2x = floor( cx + width/2 + .5 ); //p2y does not change
  }
  p1x = (p1x < 0) ? 0 : p1x;
  p1y = ( p1y < 0 ) ? 0 : p1y;

  if(image_ && image_->ni()>0 && image_->nj()>0) // in case image_ is NULL
  {
    p2x = ( p2x > image_->ni() ) ? image_->ni() : p2x;
    p2y = ( p2y > image_->nj() ) ? image_->nj() : p2y;
  }
  obj.bbox_.set_min_x( p1x );
  obj.bbox_.set_min_y( p1y );
  obj.bbox_.set_max_x( p2x );
  obj.bbox_.set_max_y( p2y );
}


void
da_tracker_process
::terminate_all_tracks()
{
  // Make all the active tracks terminated tracks, and clear out the active
  // track list.

#ifdef PRINT_DEBUG_INFO
  typedef  vcl_vector< da_so_tracker_sptr >::iterator iter0_type;
  typedef  vcl_vector< track_sptr >::iterator iter1_type;
  iter0_type it0 = trackers_.begin();
  iter1_type it1 = tracks_.begin();
  while( it0 != trackers_.end() )
  {
    assert( it1 != tracks_.end() );
    log_debug( this->name() << ": Terminating "<< *(*it1) << "\n" );
    ++it0;
    ++it1;
  }
#endif
  terminated_tracks_.clear();
  terminated_tracks_.swap( tracks_ );

  terminated_trackers_.clear();
  terminated_trackers_.swap( trackers_ );

  terminated_wh_trackers_.clear();
  terminated_wh_trackers_.swap( wh_trackers_ );
}


void
da_tracker_process
::set_world_units_per_pixel( double val )
{
  world_units_per_pixel_ = val;
}


// ----------------------------------------------------------------
/** Accept tracks and trackers as input
 *
 *
 */
void
da_tracker_process
::set_new_trackers( vcl_vector< da_so_tracker_sptr > const& trackers )
{
  // clear the new element lists
  new_tracks_.clear();
  new_trackers_ = &trackers;

  for( unsigned int i = 0; i < trackers.size(); ++i )
  {
    track_sptr t = trackers[i]->get_track();

    if( reassign_track_ids_ )
    {
      t->set_id(this->next_track_id_);
      this->next_track_id_++;
    }

#ifdef PRINT_DEBUG_INFO
    log_debug( this->name() << ": Received new "<< *t << " \n" );
#endif

    new_tracks_.push_back(t);
  }
}

void
da_tracker_process
::set_new_wh_trackers( vcl_vector< da_so_tracker_sptr > const& wh_trackers )
{
  new_wh_trackers_ = wh_trackers;
}


double
da_tracker_process
::gate_sigma() const
{
  return vcl_sqrt( gate_sigma_sqr_ );
}

vcl_vector< image_object_sptr>
da_tracker_process
::updated_fg_objects() const
{
  return updated_fg_objs_;
}

void
da_tracker_process
::set_fg_objects( vcl_vector< image_object_sptr > const& fg_objs )
{
  fg_objs_ = &fg_objs;
}

inline
bool
da_tracker_process
::boxes_intersect( vgl_box_2d<unsigned> const& a,
                   vgl_box_2d<unsigned> const& b ) const
{
  vgl_box_2d<unsigned> I = vgl_intersection( a, b);
  return (I.area() >  min_bbox_overlap_percent_ * vcl_min( a.area(), b.area() ) );
}


void
da_tracker_process
::set_active_tracks( vcl_vector< track_sptr > const& trks )
{
  updated_active_tracks_ = &trks;
}

} // end namespace vidtk
