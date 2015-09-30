/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_tracker_process.h"

#include <vcl_cassert.h>
#include <vcl_cmath.h>
#include <vcl_limits.h>
#include <vcl_functional.h>
#include <vcl_algorithm.h>

#include <vnl/vnl_math.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_2x2.h>
#include <vnl/vnl_det.h>
#include <vnl/vnl_inverse.h>
#include <vnl/vnl_hungarian_algorithm.h>

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <tracking/track_initializer_process.h>
#include <tracking/tracking_keys.h>

#include <klt/klt.h>
#include <kwklt/klt_mutex.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

namespace vidtk
{


struct klt_tracker_process::klt_state
{
  KLT_TrackingContext tc_;
  KLT_FeatureList fl_;

  ~klt_state()
  {
    if( fl_ )
    {
      KLTFreeFeatureList( fl_ );
      fl_ = NULL;
    }
    if( tc_ )
    {
      KLTFreeTrackingContext( tc_ );
      tc_ = NULL;
    }
  }
};


//NOTE: Use initialize() for all variable initializations.
klt_tracker_process
::klt_tracker_process( vcl_string const& name )
  : process( name, "klt_tracker_process" ),
    cur_ts_( NULL ),
    cur_img_( NULL ),
    state_( NULL ),
    initialized_( false ),
	  min_distance_( 0 ),
	  skipped_pixels_( 0 )
{
  config_.add( "verbosity_level", "0" );
  config_.add( "num_features", "100" );
  config_.add( "replace_lost", "true" );
  config_.add( "window_width", "7" );
  config_.add( "window_height", "7" );
  config_.add( "search_range", "15" );
  config_.add( "disabled", "false" );
  config_.add_parameter( "min_distance",
	  "10",
	  "Minimum gap between two tracked feature points." );

  config_.add_parameter( "skipped_pixels",
	  "0",
	  "Number of pixels to skip after every pixel read in the source image." );
}


klt_tracker_process
::~klt_tracker_process()
{
  delete state_;
}


config_block
klt_tracker_process
::params() const
{
  return config_;
}


bool
klt_tracker_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "verbosity_level", verbosity_ );
    blk.get( "num_features", num_feat_ );
    blk.get( "replace_lost", replace_lost_ );
    blk.get( "window_height", window_height_ );
    blk.get( "window_width", window_width_ );
    blk.get( "search_range", search_range_ );
    blk.get( "disabled", disabled_ );
	  blk.get( "min_distance", min_distance_ );
	  blk.get( "skipped_pixels", skipped_pixels_ );
  }
  catch( unchecked_return_value& )
  {
    // revert to previous values
    set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
klt_tracker_process
::initialize()
{
  if( disabled_ )
  {
    return true;
  }

  if( this->initialized_ == false )
  { // critical region
    boost::lock_guard<boost::mutex> lock(vidtk::klt_mutex::instance()->get_lock());

    next_track_id_ = 0;

    delete state_;
    state_ = new klt_state;
    state_->fl_ = NULL;

    state_->tc_ = KLTCreateTrackingContext();
    state_->tc_->sequentialMode = TRUE;
    state_->tc_->window_width = window_width_;
    state_->tc_->window_height = window_height_;
    state_->tc_->mindist = min_distance_;
    state_->tc_->nSkippedPixels = skipped_pixels_;

    KLTChangeTCPyramid( state_->tc_, search_range_ );
    KLTUpdateTCBorder( state_->tc_ );

    KLTSetVerbosity( verbosity_ );

    if( verbosity_ > 0 )
    {
      KLTPrintTrackingContext( state_->tc_ );
    }

    active_tracks_.clear();
    terminated_tracks_.clear();
    created_tracks_.clear();

    track_map_.clear();
    track_map_.resize( num_feat_, active_tracks_.end() );

    this->initialized_ = true;
  } // end critical region

  return true;
}

bool
klt_tracker_process
::reinitialize()
{
  this->initialized_ = false;
  return this->initialize();
}

bool
klt_tracker_process
::reset()
{
  return this->reinitialize();
}


bool
klt_tracker_process
::step()
{
  created_tracks_.clear();

  if( disabled_ )
  {
    return true;
  }

  log_assert( cur_ts_ != NULL, "Timestamp was not supplied" );
  log_assert( cur_img_ != NULL, "Image was not supplied" );
  log_assert( cur_img_->is_contiguous(), "Image is not contiguous" );
  log_assert( cur_img_->nplanes() == 1, "Image is not greyscale" );

  //  critical region
  boost::lock_guard<boost::mutex> lock(vidtk::klt_mutex::instance()->get_lock());

  if( state_->fl_ == NULL )
  {
    initialize_features();
  }
  else
  {
    update_tracks();
    terminate_tracks();
    if( replace_lost_ )
    {
      replace_lost_tracks();
    }
  }

  // Mark that they have been used.
  cur_img_ = NULL;
  cur_ts_ = NULL;

  return true;
}


void
klt_tracker_process
::set_image( vil_image_view< vxl_byte > const& img )
{
  cur_img_ = &img;
}


void
klt_tracker_process
::set_timestamp( timestamp const& ts )
{
  cur_ts_ = &ts;
}


vcl_list< track_sptr > const&
klt_tracker_process
::active_tracks() const
{
  return active_tracks_;
}


vcl_vector< track_sptr > const&
klt_tracker_process
::terminated_tracks() const
{
  return terminated_tracks_;
}


vcl_vector< track_sptr > const&
klt_tracker_process
::created_tracks() const
{
  return created_tracks_;
}


void
klt_tracker_process
::initialize_features()
{
  // Create storage for the features.
  state_->fl_ = KLTCreateFeatureList( num_feat_ );

  KLT_PixelType* img_ptr = const_cast<KLT_PixelType*>( cur_img_->top_left_ptr() );

  // Create the features.
  KLTSelectGoodFeatures( state_->tc_,
                         img_ptr,
                         cur_img_->ni(),
                         cur_img_->nj(),
                         state_->fl_ );

//#ifndef NDEBUG
//  int good_features;
//  for( good_features = 0; good_features < state_->fl_->nFeatures &&
//                          state_->fl_->feature[good_features]->val > 0; ++good_features )
//  {
//  }
//#endif

  // Create track structures for the newly created features.
  create_new_tracks();

  // This initial track will force an image pyramid to be created for
  // this image.  Then, we no longer need to store the pixel data for
  // this image, because the KLT code will just rely on the stored
  // pyramid.  This trick causes one unnecessary tracking operation
  // after every initialization (which most often will happen just
  // once), but has the advantage that the previous image does not
  // been to be deep copied and maintained.

  KLT_FeatureList fl = KLTCreateFeatureList( num_feat_ );

  KLTTrackFeatures( state_->tc_,
                    img_ptr,
                    img_ptr,
                    cur_img_->ni(),
                    cur_img_->nj(),
                    fl );

  KLTFreeFeatureList( fl );

  // Since the images were identical, every feature must have been
  // tracked or not created.
  // NOTE: This code does not work if the frame is bad (example:
  //       when it is nearly all one color.
//#ifndef NDEBUG
//  for( int i = 0; i < good_features; ++i )
//  {
//    log_assert( state_->fl_->feature[i]->val > -1,
//                "Feature " << i << " was not tracked" );
//  }
//#endif
}


/// \internal
///
/// \brief Create new tracks for newly created features.
///
/// This will search the KLT feature list for newly created features,
/// and for each one found, will create a new track in active_tracks_
/// and put a pointer to that in the corresponding track_map_.
void
klt_tracker_process
::create_new_tracks()
{
  int const num_feat = state_->fl_->nFeatures;
  KLT_Feature const* feature = state_->fl_->feature;

  for( int i = 0; i < num_feat; ++i )
  {
    if( feature[i]->val > 0 ) // for new features
    {
      assert( track_map_[i] == active_tracks_.end() );

      track_state_sptr st = new track_state;
      st->loc_[0] = feature[i]->x;
      st->loc_[1] = feature[i]->y;
      st->loc_[2] = 0;
      st->vel_[0] = 0;
      st->vel_[1] = 0;
      st->vel_[2] = 0;
      st->time_ = *cur_ts_;

      track_sptr trk = new track;
      trk->set_id( next_track_id_ );
      trk->add_state( st );

      track_map_[i] = active_tracks_.insert( active_tracks_.end(), trk );
      created_tracks_.push_back( trk );

      ++next_track_id_;
    }
  }
}


/// \internal
///
/// \brief Track features to the new image.
///
/// This will track the KLT features to the new image, and for the
/// features that were successfully tracked, append states to the
/// tracks in active_tracks_.
void
klt_tracker_process
::update_tracks()
{
  KLT_PixelType* img_ptr = const_cast<KLT_PixelType*>( cur_img_->top_left_ptr() );

  // Track the features onto the current frame.
  KLTTrackFeatures( state_->tc_,
                    NULL,
                    img_ptr,
                    cur_img_->ni(),
                    cur_img_->nj(),
                    state_->fl_ );

  int const num_feat = state_->fl_->nFeatures;
  KLT_Feature const* feature = state_->fl_->feature;

  // Update the state of each feature that was successfully tracked.
  for( int i = 0; i < num_feat; ++i )
  {
    // > 0 is only for new features.  There shouldn't be any new
    // > features at this point.
    assert( feature[i]->val <= 0 );

    if( feature[i]->val == 0 )
    {
      assert( track_map_[i] != active_tracks_.end() );

      track& trk = **track_map_[i];
      track_state const& lst = *trk.last_state();

      track_state_sptr st = new track_state;
      st->loc_[0] = feature[i]->x;
      st->loc_[1] = feature[i]->y;
      st->loc_[2] = 0;
      st->vel_ = ( st->loc_ - lst.loc_ ) / cur_ts_->diff_in_secs( lst.time_ );
      st->time_ = *cur_ts_;

      trk.add_state( st );
    }
  }
}


/// \internal
///
/// \brief Clean up tracks for the features that weren't tracked.
///
/// This will move the untracked tracks from active_tracks_ to
/// terminated_tracks_, and will remove the corresponding entry from
/// track_map_.
///
/// It will also clear terminated_tracks_, so that its contents when
/// this function returns is the list of all tracks that terminated
/// *this time*.
void
klt_tracker_process
::terminate_tracks()
{
  terminated_tracks_.clear();

  int const num_feat = state_->fl_->nFeatures;
  KLT_Feature const* feature = state_->fl_->feature;

  for( int i = 0; i < num_feat; ++i )
  {
    // -1 means the feature wasn't created to being with. < -1 means
    // -it couldn't be tracked.
    if( feature[i]->val < -1 )
    {
      typedef vcl_list< track_sptr >::iterator iter_type;

      iter_type it = track_map_[i];

      // we must have been tracking this previously
      assert( it != active_tracks_.end() );

      terminated_tracks_.push_back( *it );
      active_tracks_.erase( it );

      track_map_[i] = active_tracks_.end();

      // mark the feature as no longer existing.
      feature[i]->val = -1;
    }
  }
}


/// \internal
///
/// \brief Replace any features that were not tracked.
///
/// Try to replace features to get the feature count back up to
/// num_feat_ that the user requested.  A "side effect" is that this
/// process will create new tracks to replace those that weren't
/// tracked.
void
klt_tracker_process
::replace_lost_tracks()
{
  KLT_PixelType* img_ptr = const_cast<KLT_PixelType*>( cur_img_->top_left_ptr() );

  // Attempt to add more features
  KLTReplaceLostFeatures( state_->tc_,
                          img_ptr,
                          cur_img_->ni(),
                          cur_img_->nj(),
                          state_->fl_ );

  // Create tracks for any new features that were added.
  create_new_tracks();
}


} // end namespace vidtk
