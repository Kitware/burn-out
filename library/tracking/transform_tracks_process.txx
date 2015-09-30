/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "transform_tracks_process.h"

#include <tracking/tracking_keys.h>

#include <vcl_algorithm.h>

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <vcl_limits.h>
#include <vgl/vgl_intersection.h>

//
// This counter is used to control how many additional cycles are
// needed after we see a missing input (i.e. homography) until we can
// finally fail.  The value here depends on the process that is
// sending us tracks.
//
#define NEEDED_FLUSH_COUNT 2

namespace vidtk
{

template < class PixType >
transform_tracks_process< PixType >
::transform_tracks_process( vcl_string const& name )
  : process( name, "transform_tracks_process" ),
    config_(),
    in_tracks_( NULL ),
    image_buffer_( NULL ),
    ts_image_buffer_( NULL ),
    image_( NULL ),
    curr_ts_( NULL ),
    out_tracks_(),
    split_tracks_(),
    reverse_tracks_disabled_( true ),
    add_image_info_disabled_( true ),
    add_image_info_at_start_( true ),
    add_image_info_buffer_amount_( 0 ),
    add_image_info_ndetections_( 0 ),
    truncate_tracks_disabled_( true ),
    add_lat_lon_disabled_( true ),
    add_lat_lon_full_track_( true ),
    transform_image_location_disabled_( true ),
    transform_world_loc_( true ),
    crop_track_frame_range_disabled_( true ),
    start_frame_crop_( 0 ),
    end_frame_crop_( 0 ),
    crop_track_img_space_disabled_( true ),
    split_cropped_tracks_( false ),
    max_id_seen_( 0 ),
    img_crop_min_x_( 0 ),
    img_crop_min_y_( 0 ),
    img_crop_max_x_( 0 ),
    img_crop_max_y_( 0 ),
    update_track_time_disabled_( true ),
    loc_type_(centroid),
    time_offset_( 0.0 ),
    frame_rate_( 0.0 ),
    reassign_ids_disabled_( true ),
    next_track_id_( 1 ),
    base_track_id_( 1 ),
    state_to_image_disabled_( true ),
    mark_used_mods_disabled_( true ),
    regen_uuid_for_finalized_tracks_disable_( true ),
    create_fg_model_disabled_( true ),
    update_fg_model_disabled_( true ),
    cleanup_fg_model_disabled_( true )
{
  config_.add_parameter( "reverse_tracks:disabled",
    "true",
    "Temporally reverse the order of detections and creates a deep copy. "
    "Also changes corresponding feilds (location, tracking_keys::used_in_track, "
    "etc.) in vidtk::track to match the history. Note that this reversal "
    "is done before any other operation, so you can do multiple operations "
    "led by reversal." );

  config_.add_parameter( "add_image_info:disabled",
    "true",
    "Add a deep copy of image chips for a set of frames at the beginning of "
    "each track. Currently being used for track linking." );

  config_.add_parameter( "add_image_info:location",
    "start",
    "start or end. Whether to add the image info to the start or end of "
    "the track." );

  config_.add_parameter( "add_image_info:buffer_amount",
    "5",
    "Number of extra pixels you want around the original detection." );

  config_.add_parameter( "add_image_info:number_of_detections",
    "4",
    "Number of detections you want to add image information at." );

  config_.add_parameter( "truncate_tracks:disabled",
    "true",
    "Truncate track history to the current timestamp. Creates a deep copy "
    "of the tracks." );

  config_.add_parameter( "add_lat_lon:disabled",
    "true",
    "Populates latitude and longitude fields of the tracks." );

  config_.add_parameter( "add_lat_lon:full_track",
    "true",
    "Whether to populate the latitude and longitude fields of the complete "
    "track or only the latest track state." );

  config_.add_parameter( "transform_image_location:disabled",
    "true",
    "Translates the image origin of track to new aoi" );

  config_.add_parameter( "transform_image_location:transform_world_loc",
    "true",
    "Apply image origin translation to the world_loc column" );

  config_.add_parameter( "crop_track_frame_range:disabled",
    "true",
    "Crops tracks based on a temporal region" );

  config_.add_parameter( "crop_track_frame_range:start_frame_crop",
    "0",
    "The start frame of the cropping range" );

  config_.add_parameter( "crop_track_frame_range:end_frame_crop",
    "0",
    "The end frame of the cropping range" );

  config_.add_parameter( "crop_track_img_space:disabled",
    "true",
    "Crops tracks based on an img space region" );

  config_.add_parameter( "crop_track_img_space:split_cropped_tracks",
     "false",
     "Splits a track into multiple tracks when states are removed from the middle" );

  config_.add_parameter( "crop_track_img_space:img_crop_min_x",
    "0",
    "Min x for img space cropping" );

  config_.add_parameter( "crop_track_img_space:img_crop_min_y",
    "0",
    "Min y for img space cropping" );

  config_.add_parameter( "crop_track_img_space:img_crop_max_x",
    "0",
    "Max x for img space cropping" );

  config_.add_parameter( "crop_track_img_space:img_crop_max_y",
    "0",
    "Max y for img space cropping" );

  config_.add_parameter( "update_track_time:disabled",
    "true",
    "Updates timestamps of tracks" );

  config_.add_parameter( "update_track_time:time_offset",
    "0",
    "time_offset is time for frame 0 only when frame_rate is non-zero, otherwise it is an 'offset' blindly added to the current time." );

  config_.add_parameter( "update_track_time:frame_rate",
    "0",
    "Frame rate in frames per second" );

  config_.add_parameter( "reassign_track_ids:disabled",
    "true",
    "Updates the track ids with a unique number across a run." );

  config_.add_parameter( "reassign_track_ids:base_track_id",
    "1",
    "The base track_id to use for id reassignment");

  config_.add_parameter( "mark_used_mods:disabled",
    "true",
    "Set tracking_keys::used_in_track property of MODs. This is done after "
    "track initialization." );

  config_.add_parameter( "regen_uuid_for_finalized_tracks:disabled",
    "true",
    "Ensure that closed tracks that are being modifed are actually different tracks "
    " which is identified by its uuid");

  config_.add_parameter( "create_fg_model:disabled",
    "true",
    "Compute the initial foreground model of each track." );

  config_.add_subblock( fg_matcher< PixType >::params(), "create_fg_model" );

  config_.add_parameter( "update_fg_model:disabled",
    "true",
    "Update the foreground model for each track." );

  config_.add_parameter( "cleanup_fg_model:disabled",
    "true",
    "Perform cleanup operation as needed when the track has been terminated." );
  config_.add_parameter( "smooth_img_loc:disabled",
    "true",
    "Disable the smoothing of the target location");

  config_.add_parameter( "smooth_img_loc:location_type",
    "centroid",
    "Location of the target for tracking: bottom or centroid. "
    "This parameter is used in conn_comp_sp:conn_comp, tracking_sp:tracker, and tracking_sp:state_to_image");

}


template < class PixType >
config_block
transform_tracks_process< PixType >
::params() const
{
  return config_;
}


template < class PixType >
bool
transform_tracks_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    reverse_tracks_disabled_ = blk.get<bool>( "reverse_tracks:disabled" );

    add_image_info_disabled_ = blk.get<bool>( "add_image_info:disabled" );
    add_image_info_buffer_amount_ = blk.get<unsigned int>( "add_image_info:buffer_amount" );
    add_image_info_ndetections_ = blk.get<unsigned int>( "add_image_info:number_of_detections" );
    vcl_string str = blk.get<vcl_string>( "add_image_info:location" );
    if( str == "start" )
    {
      add_image_info_at_start_ = true;
    }
    else
    {
      add_image_info_at_start_ = false;
    }

    vcl_string loc = blk.get<vcl_string>( "smooth_img_loc:location_type" );
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
      return false;
    }

    state_to_image_disabled_ = blk.get<bool>( "smooth_img_loc:disabled" );

    truncate_tracks_disabled_ = blk.get<bool>( "truncate_tracks:disabled" );

    add_lat_lon_disabled_ = blk.get<bool>( "add_lat_lon:disabled" );

    add_lat_lon_full_track_ = blk.get<bool>( "add_lat_lon:full_track" );

    transform_image_location_disabled_ = blk.get<bool>( "transform_image_location:disabled" );
    transform_world_loc_ = blk.get<bool>( "transform_image_location:transform_world_loc" );

    crop_track_frame_range_disabled_ = blk.get<bool>( "crop_track_frame_range:disabled" );
    start_frame_crop_ = blk.get<unsigned>( "crop_track_frame_range:start_frame_crop" );
    end_frame_crop_ = blk.get<unsigned>( "crop_track_frame_range:end_frame_crop" );

    crop_track_img_space_disabled_ = blk.get<bool>( "crop_track_img_space:disabled" );
    split_cropped_tracks_ = blk.get<bool>( "crop_track_img_space:split_cropped_tracks" );
    img_crop_min_x_ = blk.get<int>( "crop_track_img_space:img_crop_min_x" );
    img_crop_min_y_ = blk.get<int>( "crop_track_img_space:img_crop_min_y" );
    img_crop_max_x_ = blk.get<int>( "crop_track_img_space:img_crop_max_x" );
    img_crop_max_y_ = blk.get<int>( "crop_track_img_space:img_crop_max_y" );

    update_track_time_disabled_ = blk.get<bool>( "update_track_time:disabled" );
    time_offset_ = blk.get<double>( "update_track_time:time_offset");
    frame_rate_ = blk.get<double>( "update_track_time:frame_rate");

    reassign_ids_disabled_ = blk.get<bool>( "reassign_track_ids:disabled" );

    base_track_id_ = blk.get<unsigned>( "reassign_track_ids:base_track_id" );

    mark_used_mods_disabled_ = blk.get<bool>( "mark_used_mods:disabled" );

    regen_uuid_for_finalized_tracks_disable_ = blk.get<bool>( "regen_uuid_for_finalized_tracks:disabled" );

    create_fg_model_disabled_ = blk.get<bool>( "create_fg_model:disabled" );

    update_fg_model_disabled_ = blk.get<bool>( "update_fg_model:disabled" );

    cleanup_fg_model_disabled_ = blk.get<bool>( "cleanup_fg_model:disabled" );
    
    config_.update( blk );
  }
  catch( unchecked_return_value & e )
  {
    log_error( e.what() );
    return false;
  }

  if( !create_fg_model_disabled_ )
  {
    fg_model_ = fg_matcher< PixType >::create_with_params( blk.subblock( "create_fg_model" ) );
    if( !fg_model_ )
    {
      log_error( this->name() << ": Could not initailize foreground model\n" );
      return false;
    }
  }

  return true;
}


template < class PixType >
bool
transform_tracks_process< PixType >
::initialize()
{
  this->reset();
  next_track_id_ = base_track_id_;
  if( !create_fg_model_disabled_ )
  {
    return fg_model_->initialize();
  }

  return true;
}


template < class PixType >
bool
transform_tracks_process< PixType >
::reset()
{
  H_wld2utm_ = NULL;
  H_wld2utm_last_ = NULL;
  wld2img_H_ = NULL;
  wld2img_H_last_ = NULL;
  img2wld_H_ = NULL;
  img2wld_H_last_ = NULL;
  image_last_ = NULL;
  flush_counter_ = NEEDED_FLUSH_COUNT;
  last_ts_ = timestamp();
  split_tracks_.clear();
  max_id_seen_ = 0;
  out_tracks_.clear();

  // Note that next_track_id_ is not being reset to 1 because it is
  // intended to be unique across pipeline resets within a run.

  return true;
}


// ----------------------------------------------------------------
/** Main processing loop.
 *
 *
 */
template < class PixType >
bool
transform_tracks_process< PixType >
::step()
{
  if( (!add_image_info_disabled_ && !image_buffer_) ||
      (!create_fg_model_disabled_ && !ts_image_buffer_) ||
      (!create_fg_model_disabled_ && !curr_ts_) ||
      (!update_fg_model_disabled_ && !ts_image_buffer_) ||
      (!update_fg_model_disabled_ && !curr_ts_) ||
      (!truncate_tracks_disabled_ && !curr_ts_) ||
      !in_tracks_ )
  {
    log_error( this->name() << ": Required inputs not available.\n" );
    return false;
  }


  // Special handling for handling the terminated tracks flushed from the
  // tracker. We will remember the *required* data from last frame and
  // use it for only one step when that data is not provided.
  if( !add_lat_lon_disabled_ && (!H_wld2utm_ || (flush_counter_ <= 0) ) )
  {
    log_info (this->name() << ": H_wld2utm_last_: " << H_wld2utm_last_
              << "  H_wld2utm_: " << H_wld2utm_
              << "  flush_counter_: " << flush_counter_
              << "\n");

    if( (flush_counter_ > 0) && (H_wld2utm_last_ != NULL ))
    {
      --flush_counter_;
      H_wld2utm_ = H_wld2utm_last_;
      log_debug( this->name() << ": Trying to flush out "<< in_tracks_->size()
                           << " track(s).\n" );
    }
    else
    {
      log_error( this->name() << ": Did not expect to be run after flushing tracks "
                  << "through the last " << NEEDED_FLUSH_COUNT << " steps. Still "
                  << "have " << in_tracks_->size() << " track(s).\n" );
      return false;
    }
  }

  // Special handling for up to NEEDED_FLUSH_COUNT active tracks from tracker
  // when there is no valid homography
  if(!state_to_image_disabled_ && (!wld2img_H_ || !image_ || !curr_ts_
      || (flush_counter_ <= 0) ) )
  {
    if( flush_counter_ == NEEDED_FLUSH_COUNT && ( wld2img_H_ != NULL || image_ != NULL
                                                  || curr_ts_ != NULL ) )
    {
      log_error( name() << ": Inconsistent input data, one of the three inputs is valid.\n" );
      return false;
    }

    log_info (this->name() << ": wld2img_H_last_: " << wld2img_H_last_
              << "  wld2img_H_: " << wld2img_H_
              << " image_last_: " << image_last_
              << " image_last_ size: " << image_last_->ni() << " x " << image_last_->nj()
              << " image_: " << image_
              << "  flush_counter_: " << flush_counter_
              << vcl_endl );

    if( (flush_counter_ > 0) && (wld2img_H_last_ != NULL ) && (image_last_ != NULL )
                             && (curr_ts_last_ != NULL) )
    {
      --flush_counter_;
      wld2img_H_ = wld2img_H_last_;
      image_ = image_last_;
      curr_ts_ = curr_ts_last_;
      log_debug( this->name() << ": Trying to flush out "<< in_tracks_->size()
                           << " track(s)." << vcl_endl );
    }
    else
    {
      log_error( this->name() << ": Did not expect to be run after flushing tracks "
                  << "through the last " << NEEDED_FLUSH_COUNT << " steps. Still "
                  << "have " << in_tracks_->size() << " track(s)." << vcl_endl );
      return false;
    }
  }

  // Special handling for up to NEEDED_FLUSH_COUNT frames, similar to lat_lon mode.
  if( !add_image_info_disabled_ && !curr_ts_ )
  {
    if( flush_counter_ > 0  )
    {
      --flush_counter_;
      if( !last_ts_.is_valid() )
      {
        log_error( this->name() << ":Never received valid input timestamp.\n" );
        return false;
      }

      curr_ts_ = &last_ts_;
      // image_buffer_ not is not reset so no need to remember the last_image_buffer_.
      log_info( this->name() << ": Trying to flush out "<< in_tracks_->size()
                           << " track(s).\n" );
    }
    else
    {
      log_error( this->name() << ": Did not expect to be run after flushing tracks "
                  << "through the last " << NEEDED_FLUSH_COUNT << " steps. Still "
                  << "have " << in_tracks_->size() << " track(s).\n" );
      return false;
    }
  }

  out_tracks_.clear();

  if( in_tracks_->empty() )
  {
    this->reset_inputs();

    return true;
  }

  //preallocate the max(?) we'll need
  out_tracks_.reserve( in_tracks_->size() );
  perform_step( *in_tracks_ );

  //if we finish with in_tracks_ and have some that were split, process them now.
  if ( !split_tracks_.empty() )
  {
    perform_step( split_tracks_ );
  }

  this->reset_inputs();

  return true;
}


template < class PixType >
bool
transform_tracks_process< PixType >
::perform_step( const vcl_vector< track_sptr > & tracks )
{
  for( unsigned i = 0; i < tracks.size(); i++ )
  {
    track_sptr trk;
    // (shallow or deep) copy of tracks
    if( !truncate_tracks_disabled_ || !transform_image_location_disabled_ )
    {
      // TODO: Even for truncation you don't need a deep copy!
      //       This should be removed after making sure the user of
      //       truncation is aware of this.
      trk = (tracks)[i]->clone();
    }
    else
    {
      trk = (tracks)[i];
    }

    if( !crop_track_frame_range_disabled_ )
    {
      crop_track_by_frame_range( trk );

      //if we've removed the entire track, move along to the next one.
      if ( !trk )
      {
        continue;
      }
    }
    if( !reverse_tracks_disabled_ && trk )
    {
      trk->reverse_history();
    }

    if( !add_image_info_disabled_ && trk )
    {
      this->add_image_info( trk );
    }

    if( !truncate_tracks_disabled_ && trk )
    {
      trk->truncate_history( *curr_ts_ );
    }

    if( !add_lat_lon_disabled_ && trk )
    {
      if( !H_wld2utm_->is_valid() )
      {
        log_error( this->name() << ": world-to-utm homography transformation is invalid.\n" );
      }
      else
      {
        if( add_lat_lon_full_track_ )
        {
          trk->update_latitudes_longitudes( *H_wld2utm_ );
        }
        else
        {
          // TODO: For optimization, do not update the latest track_state
          // multiple times in case of missed detection or termination lag.
          // i.e. use the current timestamp and only  update lat/long if the
          // track_state::time_ is the same.
          trk->update_latest_latitude_longitude( *H_wld2utm_ );
        }
      }
    }  // if( !add_lat_lon_disabled_ )

    if( !reassign_ids_disabled_ && trk )
    {
      trk->set_id( next_track_id_++ );
#ifdef PRINT_DEBUG_INFO
      log_debug( this->name() << ": Assigning new id: "<< *trk << " \n" );
#endif
    }
    if( !crop_track_img_space_disabled_ && trk )
    {
      crop_tracks_in_img_space( trk );
    }
    if( !transform_image_location_disabled_ && trk )
    {
      if ( H_wld_.is_valid() && H_img_.is_valid() )
      {
        transform_image_location( trk );
      }
      else
      {
        log_error( this->name() <<
                   ": either world-to-world or loc_to_loc homography transformation is invalid.\n" );
      }
    }
    if( !update_track_time_disabled_ && trk )
    {
      update_track_time( trk );
    }
    if( !mark_used_mods_disabled_ && trk )
    {
        mark_used_mods( trk );
    }
    if( !create_fg_model_disabled_ && trk )
    {
      if( !create_fg_model( trk ) )
      {
        log_error( name() << ": Foreground model creation failed for "<< *trk << "\n" );
        return false;
      }
    }
    if( !update_fg_model_disabled_ && trk )
    {
      if(! update_fg_model( trk ) )
      {
        log_error( name() << ": Foreground model update failed for "<< *trk << "\n" );
        return false;
      }
    }
    if( !cleanup_fg_model_disabled_ && trk )
    {
      if( !cleanup_fg_model( trk ) )
      {
        log_error( name() << ": Foreground model cleanup failed for "<< *trk << "\n" );
        return false;
      }
    }
    //project state to image
    //
    if(!state_to_image_disabled_ && trk)
    {
      if( !smooth_track_and_bbox(trk) )
      {
        return false;
      }
    }

    if ( trk )
    {
      //since all of the operations that follow change the track
      //we need to generate a new UUID,
      //but ONLY PERFORM THIS OPERATION WHEN POSTPROCESSING TRACK
      //NEVER IN LIVE MODE OPERATIONS BECAUSE IT IS SO SLOW.
      if ( !regen_uuid_for_finalized_tracks_disable_ )
      {
        trk->regenerate_uuid();
      }
      //if we haven't filtered the entire track, add it to out_tracks
      //NOTE*  This block should be the last if stmt in step!!
      out_tracks_.push_back( trk );
    }
  }  // for i --> tracks.size()

  return true;
}

template < class PixType >
void
transform_tracks_process< PixType >
::reset_inputs()
{
  H_wld2utm_last_ = H_wld2utm_;
  img2wld_H_last_ = img2wld_H_;
  wld2img_H_last_ = wld2img_H_;
  image_last_ = image_;
  curr_ts_last_ = curr_ts_;
  if( curr_ts_ )
  {
    last_ts_ = *curr_ts_;
  }

  H_wld2utm_ = NULL;
  img2wld_H_ = NULL;
  wld2img_H_ = NULL;
  curr_ts_ = NULL;
  image_ = NULL;
  in_tracks_ = NULL;
  split_tracks_.clear();
}

template < class PixType >
void
transform_tracks_process< PixType >
::in_tracks( vcl_vector< track_sptr > const& trks )
{
  in_tracks_ = &trks;
}

template < class PixType >
vcl_vector< track_sptr > const&
transform_tracks_process< PixType >
::out_tracks() const
{
  return out_tracks_;
}

template < class PixType >
void
transform_tracks_process<PixType>
::set_timestamp( timestamp const& ts )
{
  curr_ts_ = &ts;
}

template < class PixType >
void
transform_tracks_process<PixType>
::set_source_image_buffer( buffer< vil_image_view<PixType> > const& buffer )
{
  image_buffer_ = &buffer;
}

template < class PixType >
void
transform_tracks_process<PixType>
::set_ts_source_image_buffer( img_buff_t const& buffer )
{
  ts_image_buffer_ = &buffer;
}

template < class PixType >
void
transform_tracks_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  image_ = &img;
}

template <class PixType >
void
transform_tracks_process<PixType>
::add_image_info( track_sptr trk )
{
  // Code moved from tracking/add_image_info_to_track_state.txx

  track_state_sptr at;
  vcl_vector< track_state_sptr > const & hist = trk->history();
  int dir = (add_image_info_at_start_)?1:-1;
  int start = (add_image_info_at_start_)?0:hist.size()-1;
  unsigned int index = static_cast<unsigned>(start);
  for( int j = 0;
       j < static_cast<int>(add_image_info_ndetections_) && index < hist.size();
       ++j, index=static_cast<unsigned>(start+dir*j))
  {
    assert(start+dir*j >= 0);
    at = hist[index];
    timestamp at_time = at->time_;

    log_assert( at_time.frame_number() <= curr_ts_->frame_number(), name()
      << ": BAD Frame number" );

    unsigned offset = curr_ts_->frame_number() - at_time.frame_number();

    if( !image_buffer_->has_datum_at( offset ) )
    {
      log_warning( name() << ": image buffer overflow due to skipped states"
        " in the track. Skipping add_image_info operation at "<< at_time
        << " for " << *trk );
      continue;
    }

    vil_image_view<PixType> const & datum_at = image_buffer_->datum_at( offset );
    vcl_vector<vidtk::image_object_sptr> objs;
    if( at->data_.get(vidtk::tracking_keys::img_objs, objs))
    {
      for(unsigned int k = 0; k < objs.size(); ++k)
      {
        vil_image_view<PixType> data;
        data.deep_copy( objs[k]->templ(datum_at, add_image_info_buffer_amount_) );
        objs[k]->data_.set(vidtk::tracking_keys::pixel_data,data);
        objs[k]->data_.set(vidtk::tracking_keys::pixel_data_buffer,
                           add_image_info_buffer_amount_);
      }
    }
  }
}

template <class PixType >
void
transform_tracks_process<PixType>
::set_wld_to_utm_homography( plane_to_utm_homography const& H )
{
  H_wld2utm_ = &H;
}

template < class PixType >
void
transform_tracks_process<PixType>
::set_wld_homography( image_to_image_homography const& H )
{
  H_wld_ = H;
}

template < class PixType >
void
transform_tracks_process<PixType>
::set_img_homography( image_to_image_homography const& H )
{
  H_img_ = H;
}

//input port function for state project to image
template < class PixType >
void
transform_tracks_process<PixType>
::set_src2wld_homography( vgl_h_matrix_2d<double> const& H )
{
  img2wld_H_ = &H;
}

//input port function for state project to image
template < class PixType >
void
transform_tracks_process<PixType>
::set_wld2src_homography( vgl_h_matrix_2d<double> const& H )
{
  wld2img_H_ = &H;
}


template <class PixType >
void
transform_tracks_process<PixType>
::crop_track_by_frame_range( track_sptr & trk )
{
  //TODO:
  //Should we sanity check the start and end frame to ensure they represent a range? ie. end-start > 0
  const track_state_sptr & st = trk->first_state();
  const track_state_sptr & end = trk->last_state();

  //optimization: If the start frame and end frame are out of range so is the track
  if ( st->time_.frame_number() > end_frame_crop_ ||
       end->time_.frame_number() < start_frame_crop_ )
  {
    trk = NULL;
    return;
  }

  const vcl_vector<track_state_sptr> & track_states = trk->history();
  vcl_vector< track_state_sptr >revised_history;
  revised_history.reserve( track_states.size() );

  vcl_vector<track_state_sptr>::const_iterator state = track_states.begin();
  for(; state != track_states.end(); ++state)
  {
    unsigned frame_num = (*state)->time_.frame_number();
    if( frame_num >= start_frame_crop_ && frame_num <= end_frame_crop_ )
    {
      revised_history.push_back( (*state) );
    }
  }

  //optimization: If we didn't actually prune, then don't change anything in history
  if( revised_history.size() != track_states.size() )
  {
    //in case we removed all states, delete track.  Otherwise, replace history
    if( revised_history.size() > 0 )
    {
      trk->reset_history( revised_history );
    }
    else
    {
      trk = NULL;
      return;
    }
  }
}


template <class PixType >
void
transform_tracks_process<PixType>
::transform_image_location( track_sptr & trk )
{
  const vcl_vector<track_state_sptr> & track_states = trk->history();

  const vgl_h_matrix_2d< double > & img_transform = H_img_.get_transform();
  const vgl_h_matrix_2d< double > & wld_transform = H_wld_.get_transform();

  vcl_vector<track_state_sptr>::const_iterator state = track_states.begin();
  for(; state != track_states.end(); ++state)
  {
    //transform state_loc
    apply_transformation( wld_transform, (*state)->loc_, (*state)->loc_ );

    //get the image_object from state
    image_object_sptr img_obj;
    (*state)->image_object( img_obj );
    if(!img_obj)
    {
      log_warning( "No img_obj available\n" );
      continue;
    }

    //translate img_loc
    apply_transformation( img_transform, img_obj->img_loc_, img_obj->img_loc_ );

    //the user wants to shift world_loc
    if (transform_world_loc_ )
    {
      apply_transformation( wld_transform, img_obj->world_loc_, img_obj->world_loc_ );
    }

    //apply homography to bbox
    const vgl_box_2d<unsigned> & img_bbox = img_obj->bbox_;

    vnl_vector_fixed<double, 2> bbox_min( img_bbox.min_x(), img_bbox.min_y() );
    apply_transformation( img_transform, bbox_min, bbox_min );

    vnl_vector_fixed<double, 2> bbox_max( img_bbox.max_x(), img_bbox.max_y() );
    apply_transformation( img_transform, bbox_max, bbox_max );

    //detect corner cases involving overflow of unsigned
    //For now we will peg underflow cases to 0 and overflow cases to UINT_MAX
    //****Note, once the image_object bbox becomes a box of double we can remove all this code!!!
    //Underflow case
    bool underflow = false;
    bool overflow = false;
    if ( bbox_min[0] < 0 )
    {
      underflow = true;
      bbox_min[0] = 0;
    }
    if ( bbox_min[1] < 0 )
    {
      underflow = true;
      bbox_min[1] = 0;
    }
    if ( bbox_max[0] < 0 )
    {
      underflow = true;
      bbox_max[0] = 0;
    }
    if ( bbox_max[1] < 0 )
    {
      underflow = true;
      bbox_max[1] = 0;
    }

    //Overflow case
    if ( bbox_min[0] > UINT_MAX )
    {
      overflow = true;
      bbox_min[0] = UINT_MAX;
    }
    if ( bbox_min[1] > UINT_MAX )
    {
      overflow = true;
      bbox_min[1] = UINT_MAX;
    }
    if ( bbox_max[0] > UINT_MAX )
    {
      overflow = true;
      bbox_max[0] = UINT_MAX;
    }
    if ( bbox_max[1] > UINT_MAX )
    {
      overflow = true;
      bbox_max[1] = UINT_MAX;
    }

    if (underflow)
    {
      log_warning( "Bounding box underflow\n");
    }
    if (overflow)
    {
      log_warning( "Bounding box overflow\n");
    }

    vgl_box_2d<unsigned> shifted_bbox(
      vgl_point_2d<unsigned>( (unsigned)bbox_min[0], (unsigned)bbox_min[1] ),
      vgl_point_2d<unsigned>( (unsigned)bbox_max[0], (unsigned)bbox_max[1] )
      );

    img_obj->bbox_ = shifted_bbox;

    //transform image_object::boundary
    vgl_polygon<double> boundary = img_obj->boundary_;
    for (unsigned int s = 0; s < boundary.num_sheets(); ++s)
    {
      for (unsigned int p = 0; p < boundary[s].size(); ++p)
      {
        apply_transformation( img_transform, boundary[s][p], boundary[s][p] );
      }
    }

    //transform image_object::mask_io & mask_jo
    vnl_vector_fixed<double, 2> mask( img_obj->mask_i0_, img_obj->mask_j0_ );
    apply_transformation( img_transform, mask, mask );

    img_obj->mask_i0_ = (unsigned) mask[0];
    img_obj->mask_j0_ = (unsigned) mask[1];
  }
}

template <class PixType >
void
transform_tracks_process<PixType>
::crop_tracks_in_img_space( track_sptr & trk )
{
  bool processing_track_center = false;

  //protect from out of order track_ids
  max_id_seen_ = ( trk->id() <= max_id_seen_ ? max_id_seen_:trk->id() );

  vgl_box_2d<unsigned int> testbox(
    vgl_point_2d<unsigned int>(img_crop_min_x_, img_crop_min_y_),
    vgl_point_2d<unsigned int>(img_crop_max_x_, img_crop_max_y_)
    );

  const vcl_vector<track_state_sptr> & track_states = trk->history();
  vcl_vector <track_state_sptr> tmp_history;
  vcl_vector< vcl_vector <track_state_sptr> >inlier_segments;

  vcl_vector<track_state_sptr>::const_iterator state = track_states.begin();
  for(; state != track_states.end(); ++state)
  {
    image_object_sptr img_obj;

    if(!(*state)->image_object( img_obj ))
    {
      log_warning( "No img_obj available\n" );
      continue;
    }

    const vgl_box_2d<unsigned int> & img_bbox = img_obj->bbox_;

    const vgl_box_2d<unsigned int> & intersect_box = vgl_intersection( testbox, img_bbox );

    // now that we have an intersection between bbox and aoi,
    // test that the area of the intersection == area of bbox
    // if so, keep the state
    if ( intersect_box.area() == img_bbox.area() )
    {
      tmp_history.push_back((*state));
      processing_track_center = true;
    }
    //we've kept part of the track already and we want to split tracks
    // rather than remove states from the middle
    else if ( processing_track_center && split_cropped_tracks_ )
    {
      inlier_segments.push_back( tmp_history );
      tmp_history.clear();
      //set reassign track_ids = true so all new tracks will start id numbering
      // from this point on
      reassign_ids_disabled_ = false;

      //reset processing_track_center = false
      processing_track_center = false;
    }
  }


  //if we're reassigning ids then set the base next_id here
  if ( !reassign_ids_disabled_ )
  {
    next_track_id_ = max_id_seen_ + 1;
  }

  //if we have left over states from the final round of processing, add them to the back
  if ( !tmp_history.empty() )
  {
    inlier_segments.push_back( tmp_history );
  }

  //if inlier_segments is 0, then don't want to check its contents!
  if ( inlier_segments.empty() )
  {
    trk = NULL;
    return;
  }

  tmp_history = inlier_segments[0];

  //this shouldn't have happened but if we've added an empty vector to list, then exit
  if ( tmp_history.empty() )
  {
    trk = NULL;
    return;
  }

  trk->reset_history( tmp_history );
  //here we have more than 1 track
  for (int i = 1; i < inlier_segments.size(); i++ )
  {
    tmp_history = inlier_segments[i];
    track_sptr tmp_track = trk->shallow_clone();
    tmp_track->regenerate_uuid();
    tmp_track->reset_history( tmp_history );
    split_tracks_.push_back( tmp_track );
  }
}


// Updates the timestamp
template <class PixType >
void
transform_tracks_process<PixType>
::update_track_time( track_sptr & trk )
{
  const double time_offset_usecs = time_offset_ * 1e6;
  const double time_per_frame = 1e6 / frame_rate_ ;
  double new_time;
  vcl_vector<track_state_sptr> hist = trk->history();
  vcl_vector<track_state_sptr>::iterator i = hist.begin();
  vcl_vector<track_state_sptr>::iterator i_end = hist.end();

  for(; i != i_end; ++i )
  {
    if ( frame_rate_ > 0 )
    {
      new_time =  ( (*i)->time_.frame_number() * time_per_frame ) + time_offset_usecs;
    }
    else
    {
      new_time = (*i)->time_.time() + time_offset_usecs;
    }
    (*i)->time_.set_time( new_time );
  }
}

// Mark the MODs in the track as being used, so that back tracking
// (this frame) or track initializer (next frame) doesn't try to reuse them.
template <class PixType>
void
transform_tracks_process<PixType>
::mark_used_mods( track_sptr trk )
{
  vcl_vector<track_state_sptr> const& hist = trk->history();
  vcl_vector<track_state_sptr>::const_iterator i = hist.begin();
  vcl_vector<track_state_sptr>::const_iterator h_end = hist.end();

  for( ; i != h_end; ++i )
  {
    vcl_vector<image_object_sptr> objs;
    if( (*i)->data_.get( tracking_keys::img_objs, objs ) )
    {
      for( unsigned j = 0; j < objs.size(); ++j )
      {
        objs[j]->data_.set( tracking_keys::used_in_track, true );
      }
    }
  }
}


template <class PixType>
void
transform_tracks_process<PixType>
::apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vnl_vector_fixed<double, 3> & P,
    vnl_vector_fixed<double, 3> & out  )
{
  //detect and assert if z is not 0
  log_assert( P[2] == 0, "Found a non-zero z value");

  vgl_homg_point_2d<double> hP(P[0], P[1]);
  vgl_point_2d<double> tmp_out;
  apply_transformation( T, hP, tmp_out);
  out[0] = tmp_out.x();
  out[1] = tmp_out.y();
  out[2] = 0;
}

template <class PixType>
void
transform_tracks_process<PixType>
::apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vnl_vector_fixed<double, 2> & P,
    vnl_vector_fixed<double, 2> & out  )
{
  vgl_point_2d<double> tmp_out;
  vgl_homg_point_2d<double> hP(P[0], P[1]);
  apply_transformation( T, hP, tmp_out);
  out[0] = tmp_out.x();
  out[1] = tmp_out.y();
}

template <class PixType>
void
transform_tracks_process<PixType>
::apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vgl_point_2d<double> & P,
    vgl_point_2d<double> & out  )
{
  vgl_homg_point_2d<double> hP(P.x(), P.y());
  apply_transformation( T, hP, out);
}

//inner most function that others will call
template <class PixType>
void
transform_tracks_process<PixType>
::apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vgl_homg_point_2d<double> & P,
    vgl_point_2d<double> & out  )
{
  out = T * P;
}

template <class PixType>
bool
transform_tracks_process<PixType>
::create_fg_model( track_sptr trk )
{
  typename fg_matcher< PixType >::sptr_t track_model = fg_model_->deep_copy();
  if( !track_model->create_model( trk, *ts_image_buffer_, *curr_ts_ ) )
    return false;

  // Add the model to the track.
  trk->data().set( tracking_keys::foreground_model, track_model );

  return true;
}

template <class PixType>
bool
transform_tracks_process<PixType>
::update_fg_model( track_sptr trk )
{
  typename fg_matcher< PixType >::sptr_t track_model;
  try
  {
    trk->data().get( tracking_keys::foreground_model, track_model );
  }
  catch( unchecked_return_value )
  {
    log_error( this->name() << ": Did not find fg model in map.\n" );
    return false;
  }

  return track_model->update_model( trk, *ts_image_buffer_, *curr_ts_ );
}

template <class PixType>
bool
transform_tracks_process<PixType>
::cleanup_fg_model( track_sptr trk )
{
  typename fg_matcher< PixType >::sptr_t track_model;
  try
  {
    trk->data().get( tracking_keys::foreground_model, track_model );
  }
  catch( unchecked_return_value )
  {
    log_error( this->name() << ": Did not find fg model in map.\n" );
    return false;
  }

  return track_model->cleanup_model( trk );
}

template <class PixType>
bool
transform_tracks_process<PixType>
::wld2img(vnl_double_3 const& wp_3, vnl_double_2& ip_2)
{
    vnl_double_3 ip_3;

    if(wld2img_H_)
    {
      ip_3 = this->wld2img_H_->get_matrix() * wp_3;
    }
    else
    {
       log_error( this->name() <<
                   ": world to image homography is invalid.\n" );
      return false;
    }

    ip_2[0] = ip_3[0]/ip_3[2];
    ip_2[1] = ip_3[1]/ip_3[2];
    ip_3[2]=1.0;

    return true;
}

template <class PixType>
bool
transform_tracks_process<PixType>
::img2wld(vnl_double_2 const& ip_2, vnl_double_3& wp_3)
{
    vnl_double_3 ip_3;

    ip_3[0] = ip_2[0];
    ip_3[1] = ip_2[1];
    ip_3[2] = 1.0;

    if(img2wld_H_)
    {
      wp_3 = this->img2wld_H_->get_matrix() * ip_3;
    }
    else
    {
      log_error( this->name() <<
                   ": image to world homography is invalid.\n" );
      return false;
    }

    wp_3[0] = wp_3[0] / wp_3[2];
    wp_3[1] = wp_3[1] / wp_3[2];
    wp_3[2] = 1.0;

    return true;

}

template <class PixType>
bool
transform_tracks_process<PixType>
::smooth_track_and_bbox( track_sptr trk )
{
  vnl_double_3 wld_loc;
  vnl_double_2 img_loc;
  image_object_sptr obj;

  double min_x, min_y, max_x, max_y;

  if(trk->last_state()->time_ == (*curr_ts_) )
  {
    if(trk->history().back()->image_object(obj) && image_)
    {
      // for location
      wld_loc[0]=trk->history().back()->loc_[0];
      wld_loc[1]=trk->history().back()->loc_[1];
      wld_loc[2]=1.0;
      if(wld2img(wld_loc,img_loc))
      {
        // check image boundary
        img_loc[0] = (img_loc[0] > 0) ? img_loc[0] : 0;
        img_loc[0] = (img_loc[0] < image_->ni() - 1) ? img_loc[0] : image_->ni() - 1;
        img_loc[1] = (img_loc[1] > 0) ? img_loc[1] : 0;
        img_loc[1] = (img_loc[1] < image_->nj() - 1) ? img_loc[1] : image_->nj() - 1;

        // set smoothed image location
        obj->img_loc_[0]=img_loc[0];
        obj->img_loc_[1]=img_loc[1];

        min_x = img_loc[0] - obj->bbox_.width()/2.0;
        max_x = img_loc[0] + obj->bbox_.width()/2.0;

        if(loc_type_ == bottom)
        {
          min_y = img_loc[1] - obj->bbox_.height();
          max_y = img_loc[1];
        }
        else // centroid
        {
          min_y = img_loc[1] - obj->bbox_.height()/2.0;
          max_y = img_loc[1] + obj->bbox_.height()/2.0;
        }

        // handle out of image case
        min_x = (min_x > 1.) ? min_x : 0.;
        max_x = (max_x > 1.) ? max_x : 0.;
        min_y = (min_y > 1.) ? min_y : 0.;
        max_y = (max_y > 1.) ? max_y : 0.;

        min_x = (min_x < image_->ni() - 1) ? min_x : image_->ni() - 1;
        max_x = (max_x < image_->ni() - 1) ? max_x : image_->ni() - 1;
        min_y = (min_y < image_->nj() - 1) ? min_y : image_->nj() - 1;
        max_y = (max_y < image_->nj() - 1) ? max_y : image_->nj() - 1;


        // set smoothed bbox
        obj->bbox_.set_min_x((unsigned)min_x);
        obj->bbox_.set_max_x((unsigned)max_x);
        obj->bbox_.set_min_y((unsigned)min_y);
        obj->bbox_.set_max_y((unsigned)max_y);

        return true;
      }
    }
  }
  else
  {
    return true;
  }
  return false;
}

} // vidtk
