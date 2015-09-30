/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "back_tracking_super_process.h"

#include <pipeline/sync_pipeline.h>

#include <tracking/da_so_tracker_generator_process.h>
#include <tracking/da_tracker_process.h>
#include <tracking/transform_tracks_process.h>
#include <utilities/ring_buffer_process.h>
#include <utilities/unchecked_return_value.h>

#include <vcl_algorithm.h>

#include <tracking/gui_process.h>
#include <video/image_list_writer_process.h>

namespace vidtk
{

template< class PixType >
class back_tracking_super_process_impl
{
public:
  // Processes that actually performs back tracking
  process_smart_pointer< da_tracker_process > proc_tracker;
  process_smart_pointer< transform_tracks_process< PixType > > proc_track_reverser1;
  process_smart_pointer< transform_tracks_process< PixType > > proc_create_fg_model;
  process_smart_pointer< transform_tracks_process< PixType > > proc_track_reverser2;
  process_smart_pointer< transform_tracks_process< PixType > > proc_fg_init;
  process_smart_pointer< transform_tracks_process< PixType > > proc_fg_update;
  process_smart_pointer< da_so_tracker_generator_process > proc_tracker_init;
  process_smart_pointer< gui_process > proc_gui;
  process_smart_pointer< image_list_writer_process< PixType > > proc_gui_writer;

  // I/O port data
  vcl_vector< track_sptr > const * new_tracks;
  vcl_vector< track_sptr > terminated_tracks;

  // Model used for area, color, and kinematics based back tracking.
  multiple_features const * mf_params;

  // Buffers used to access the history.
  // NOTE: All have to be synchronized and same length.
  buffer< vil_image_view< PixType > > const * image_buffer;
  paired_buffer< timestamp, vil_image_view< PixType > > const * ts_image_buffer;
  paired_buffer< timestamp, double > const * gsd_buffer;
  buffer< timestamp > const * timestamp_buffer;
  buffer< vcl_vector< image_object_sptr > > const * mod_buffer;
  buffer< vgl_h_matrix_2d<double> > const * img2wld_homog_buffer;
  buffer< vgl_h_matrix_2d<double> > const * wld2img_homog_buffer;
  frame_objs_type fg_objs_buffer;
  frame_objs_type updated_fg_objs_buffer;

  // Configuration parameters
  config_block config;
  config_block default_config;
  config_block forced_config;

  bool disabled;
  double duration_secs;
  unsigned duration_frames;
  unsigned track_init_delta;

  // Temporary variable holding the sptr to the vgui_process passed
  // in from the app.
  process_smart_pointer< gui_process > gui;
  bool gui_disabled;

  back_tracking_super_process_impl()
  : proc_tracker( NULL ),
    proc_track_reverser1( NULL ),
    proc_create_fg_model( NULL ),
    proc_track_reverser2( NULL ),
    proc_fg_init( NULL ),
    proc_fg_update( NULL ),
    proc_tracker_init( NULL ),
    proc_gui( NULL ),
    proc_gui_writer( NULL ),
    new_tracks( NULL ),
    terminated_tracks(),
    mf_params( NULL ),
    image_buffer( NULL ),
    ts_image_buffer( NULL ),
    timestamp_buffer( NULL ),
    mod_buffer( NULL ),
    img2wld_homog_buffer( NULL ),
    wld2img_homog_buffer( NULL ),
    fg_objs_buffer(),
    updated_fg_objs_buffer(),
    config(),
    default_config(),
    forced_config(),
    disabled( true ),
    duration_secs( 0.0 ),
    duration_frames( 0 ),
    track_init_delta( 0 ),
    gui( new gui_process( "bt_gui" ) ),
    gui_disabled( true )
  {}

  void create_process_configs()
  {
    proc_track_reverser1 = new transform_tracks_process< PixType >( "track_reverser1" );
    config.add_subblock( proc_track_reverser1->params(),
                         proc_track_reverser1->name() );

    proc_fg_init = new transform_tracks_process< PixType >( "fg_init" );
    config.add_subblock( proc_fg_init->params(),
                         proc_fg_init->name() );

    proc_tracker = new da_tracker_process( "back_tracker" );
    config.add_subblock( proc_tracker->params(), proc_tracker->name() );

    proc_fg_update = new transform_tracks_process< PixType >( "fg_update" );
    config.add_subblock( proc_fg_update->params(),
                         proc_fg_update->name() );

    proc_track_reverser2 = new transform_tracks_process< PixType >( "track_reverser2" );
    config.add_subblock( proc_track_reverser2->params(),
                         proc_track_reverser2->name() );

    proc_tracker_init = new da_so_tracker_generator_process( "tracker_initializer" );
    config.add_subblock( proc_tracker_init->params(),
                         proc_tracker_init->name() );

    // Creating the *do nothing* gui by default.
    proc_gui = new gui_process( "bt_gui" );
    config.add_subblock( proc_gui->params(), proc_gui->name() );

    proc_gui_writer = new image_list_writer_process< PixType >( "gui_writer" );
    config.add_subblock( proc_gui_writer->params(), proc_gui_writer->name() );

    default_config.add( proc_gui_writer->name() + ":disabled", "true" );
    config.update( default_config );

    // These are the chosen default value for the pipeline to work as planned.
    // User is not allowed to over-ride these values.
    forced_config.add( "track_reverser1:reverse_tracks:disabled", "false" );
    forced_config.add( "track_reverser2:reverse_tracks:disabled", "false" );
    forced_config.add( "back_tracker:reassign_track_ids", "false" );
    // Over-riding the defualt/file config parameters
    config.update( forced_config );
  }

  void setup_pipeline( pipeline_sptr & pp )
  {
    sync_pipeline* p = new sync_pipeline;
    pp = p;

    p->add( proc_track_reverser1 );
    p->add( proc_fg_init );
    p->add( proc_tracker_init );
    p->add( proc_tracker );
    p->add( proc_fg_update );
    p->add( proc_track_reverser2 );
    //incoming connections of fg_init
    p->connect( proc_track_reverser1->out_tracks_port(),
                proc_fg_init->in_tracks_port() );

    // incoming connections of tracker_init
    p->connect( proc_fg_init->out_tracks_port(),
                proc_tracker_init->set_new_tracks_port() );

    // incoming connections of tracker
    p->connect( proc_tracker_init->new_trackers_port(),
                proc_tracker->set_new_trackers_port() );

    // incoming connections of track_reverser2
    p->connect( proc_tracker->terminated_tracks_port(),
                proc_track_reverser2->in_tracks_port() );
    p->connect_without_dependency( proc_fg_update->out_tracks_port(),
                                   proc_tracker->set_active_tracks_port());
    
    // incoming connections for fg_update
    p->connect( proc_tracker->active_tracks_port(),
                proc_fg_update->in_tracks_port() );

    // Replace the *do nothing* gui with the supplied gui object.
    if( gui_disabled && gui->class_name() != "gui_process" )
    {
      proc_gui = new gui_process( "bt_gui" );
    }
    else
    {
      if( gui->class_name() == "gui_process" )
      {
        log_warning( "vgui_process not provided for "
                     "back_tracking_super_process:bt_gui.\n" );
      }

      proc_gui = gui;
    }

    p->add( proc_gui );
    p->add( proc_gui_writer );
    // incoming connections of proc_gui
    p->connect( proc_tracker->active_tracks_port(),
                proc_gui->set_active_tracks_port() );
    p->connect( proc_tracker->active_trackers_port(),
                proc_gui->set_active_trkers_port() );
    // See step() for the rest

    // incoming connections of proc_gui_writer
    p->connect( proc_gui->gui_image_out_port(),
                proc_gui_writer->set_image_port() );
    // See step() for the rest
  }

  bool compute_duration_frames()
  {
    // derive duration_frames_ from duration_secs_
    timestamp const& last_ts = timestamp_buffer->datum_at( 0 );
    timestamp const& second_last_ts = timestamp_buffer->datum_at( 1 );
    if( last_ts.has_time() && second_last_ts.has_time() )
    {
      //treat the user-specified duration_secs_ in secs
      double dt = last_ts.time_in_secs() - second_last_ts.time_in_secs();
      if( dt > 0 )
      {
        duration_frames = static_cast<unsigned int>( vcl_floor( duration_secs / dt ) );
      }
      return true;
    }
    else
    {
      return false;
    }
  }

  process::step_status run_pipeline( pipeline_sptr & p )
  {
    // Run pipeline
    process::step_status p_status = p->execute();

    if( p_status != process::FAILURE )
    {
      // Add teriminated tracks on the current frame to the pool
      vcl_vector< track_sptr > const t_trks = proc_track_reverser2->out_tracks();
      terminated_tracks.insert( terminated_tracks.end(), t_trks.begin(), t_trks.end() );
    }

    // Keeping this after the copy of terminated_tracks, because we know that
    // isOK can be false in the *normal* case of the last push of active_tracks
    // from the tracker.
    return p_status;
  }

}; //class back_tracking_super_process


template<class PixType>
back_tracking_super_process<PixType>
::back_tracking_super_process( vcl_string const& name )
  : super_process( name, "back_tracking_super_process" ),
    impl_( new back_tracking_super_process_impl<PixType> )
{
  impl_->create_process_configs();

  impl_->config.add_parameter( "disabled",
    "true",
    "Deactivite this super-process. The data should pass through "
    "unchanged." );

  impl_->config.add_parameter( "duration_secs",
    "0.0",
    "The duration (in secs) of the temporal window through which "
    "back-tracking is performed. duration_frames chosen over "
    "duration_secs when both supplied." );

  impl_->config.add_parameter( "duration_frames",
    "0",
    "The duration (in frames) of the temporal window through which "
    "back-tracking is performed. duration_frames chosen over "
    "duration_secs when both supplied." );

  impl_->config.add_parameter( "track_init_duration_frames",
    "0",
    "The size of track initialization window (in frames), before which "
    "back tracking will be applied for the specified duration. Note that "
    "this parameter *has* to be the same ast the track_init:delta parameter." );
}

template<class PixType>
back_tracking_super_process<PixType>
::~back_tracking_super_process()
{
  delete impl_;
}

template<class PixType>
config_block
back_tracking_super_process<PixType>
::params() const
{
  return impl_->config;
}

template<class PixType>
bool
back_tracking_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    impl_->config.update( blk );

    impl_->config.get( "disabled", impl_->disabled );
    impl_->config.get( "duration_secs", impl_->duration_secs );
    impl_->config.get( "duration_frames", impl_->duration_frames );
    impl_->config.get( "track_init_duration_frames", impl_->track_init_delta );
    impl_->config.get( impl_->proc_gui->name() + ":disabled",
                       impl_->gui_disabled );

    // Over-riding the defualt/file config parameters
    impl_->config.update( impl_->forced_config );

    impl_->setup_pipeline( this->pipeline_ );

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw unchecked_return_value( " unable to set pipeline parameters." );
    }
  }
  catch( unchecked_return_value& e)
  {
    log_error( this->name() << ": couldn't set parameters: "
                            << e.what() <<"\n" );
    return false;
  }

  return true;
}

template<class PixType>
bool
back_tracking_super_process<PixType>
::initialize()
{
  return this->pipeline_->initialize();
}

template<class PixType>
bool
back_tracking_super_process<PixType>
::reset()
{
  impl_->updated_fg_objs_buffer.clear();
  impl_->terminated_tracks.clear();
  return this->pipeline_->reset();
}

// The outcome of back tracking is updated vidtk::track.history_ only. The latest
// state of the trackers and the time order of vidtk::track.history_ should remain
// unchanged.

template<class PixType>
process::step_status
back_tracking_super_process<PixType>
::step2()
{
  if( ! impl_->disabled && ! impl_->new_tracks )
  {
    log_error( this->name() << ": Input tracks not available." );
    return FAILURE;
  }

  // Converting the back-tracking window duration parameter from secs to frames
  // default, only do this once.
  if( !impl_->disabled && impl_->duration_frames == 0 )
  {
    if( ! impl_->compute_duration_frames() )
    {
      log_error( this->name() << ": Could not use duration due to invalid data in "
        "timestamp buffer.\n" );
      return FAILURE;
    }
  }

  impl_->updated_fg_objs_buffer.clear();
  impl_->terminated_tracks.clear();

  if( impl_->disabled || impl_->duration_frames == 0 ||
      impl_->new_tracks->empty() )
  {
    // Pass the received data unchanged.
    impl_->terminated_tracks = *(impl_->new_tracks);
    impl_->new_tracks = NULL;
    impl_->fg_objs_buffer.clear();
    return SUCCESS;
  }

  impl_->terminated_tracks.reserve( impl_->new_tracks->size() );

  vcl_cout << this->name() << ": Back tracking "<< impl_->new_tracks->size()
           << " new tracks.\n";

#ifdef DEBUG
  vcl_map<unsigned, unsigned> track_length_LUT;
  for(unsigned i = 0; i < impl_->new_tracks->size(); i++ )
  {
    track_length_LUT[ (*new_tracks_)[i]->id() ] = (*new_tracks_)[i]->history().size();
  }
#endif

  log_assert( impl_->ts_image_buffer &&
              impl_->image_buffer->size() == impl_->ts_image_buffer->buffer_.size() &&
              impl_->image_buffer->size() == impl_->timestamp_buffer->size() &&
              impl_->image_buffer->size() == impl_->img2wld_homog_buffer->size() &&
              impl_->image_buffer->size() == impl_->wld2img_homog_buffer->size() &&
              impl_->image_buffer->size() == impl_->mod_buffer->size() &&
              impl_->image_buffer->size() == impl_->fg_objs_buffer.size() &&
              impl_->image_buffer->size() == impl_->gsd_buffer->buffer_.size(),
              this->name() << ": The input buffers are of inconsistent sizes." );

  // window indices [back_track_win_begin, back_track_win_end), 0 being the current frame.
  unsigned back_track_win_begin = impl_->track_init_delta + 1;

  // To not to go beyond the beginning of the current sequence
  unsigned back_track_win_end = vcl_min( impl_->image_buffer->size(),
    back_track_win_begin + impl_->duration_frames );

  // Copy data to input port(s)
  impl_->proc_fg_init->set_ts_source_image_buffer( *impl_->ts_image_buffer );
  impl_->proc_fg_update->set_ts_source_image_buffer( *impl_->ts_image_buffer );

  process::step_status p_status = FAILURE;
  vcl_vector< track_sptr > const empty_tracks(0);
  frame_objs_type::const_iterator fg_objs_iter;
  paired_buffer< timestamp, double >::buffer_t::const_iterator gsd_buff_iter;

  // For each frame in the back tracking window
  for(unsigned i = back_track_win_begin; i < back_track_win_end;  i++ )
  {
    timestamp const& ts = impl_->timestamp_buffer->datum_at(i);

    log_assert( impl_->timestamp_buffer->has_datum_at(i),
                this->name() << ": Indexing wrong frame while back tracking." );

    if( i == back_track_win_begin )
    {
      // Find is called only on the first iteration, because the entries in
      // frame_objs_type are sorted by timestamp.

      fg_objs_iter = impl_->fg_objs_buffer.find( ts );
      log_assert( fg_objs_iter != impl_->fg_objs_buffer.end(),
        name() << ": Didn't find time in fg objs buffer.\n" );

      bool found = impl_->gsd_buffer->find_datum( ts, gsd_buff_iter, true );
      log_assert( found, name() << ": Didn't find time in gsd buffer.\n" );
    }
    else
    {
      // Hack to make the first node in the pipeline work
      impl_->proc_track_reverser1->in_tracks( empty_tracks );

      // Note that we are not testing for buffer overflow condition on every
      // iteration mainly for effeciency. The boundary conditions of all
      // buffers has already been tested before this for loop.
      fg_objs_iter--;
      gsd_buff_iter--;
    }

    // Copy data to input port(s)
    impl_->proc_fg_init->set_timestamp( ts );
    impl_->proc_fg_update->set_timestamp( ts );
    impl_->proc_tracker->set_timestamp( ts );
    impl_->proc_tracker->set_source_image( impl_->image_buffer->datum_at(i) );
    impl_->proc_tracker->set_image2world_homography( impl_->img2wld_homog_buffer->datum_at(i));
    impl_->proc_tracker->set_world2image_homography( impl_->wld2img_homog_buffer->datum_at(i));
    impl_->proc_tracker->set_objects( impl_->mod_buffer->datum_at(i) );
    impl_->proc_tracker->set_fg_objects( fg_objs_iter->second );
    impl_->proc_tracker->set_world_units_per_pixel( gsd_buff_iter->datum_ );

    // incoming connections of proc_gui
    impl_->proc_gui->set_source_timestamp( ts );
    impl_->proc_gui->set_image( impl_->image_buffer->datum_at(i) );
    impl_->proc_gui->set_world_homog( impl_->wld2img_homog_buffer->datum_at(i) );
    impl_->proc_gui->set_filter1_objects( impl_->mod_buffer->datum_at(i) );

    // incoming connections of proc_gui_writer
    impl_->proc_gui_writer->set_timestamp( ts );

    p_status = impl_->run_pipeline( this->pipeline_ );
    if( p_status == FAILURE ) // A legitimate pipeline failure.
      return p_status;

    if( impl_->proc_tracker->updated_fg_objects().size() >
        fg_objs_iter->second.size() )
    {
      // We only want to push back the updated set of foreground objects when
      // we have at least one new object added to the set on this frame.

      impl_->updated_fg_objs_buffer[ ts ] = impl_->proc_tracker->updated_fg_objects();
    }

    // For optimization
    if( impl_->proc_fg_update->out_tracks().empty() )
    {
      break;
    }
  }

  // In case we were still tracking objects even at the end of the specified
  // *duration*, we want to get those too and pass them along to be
  // tracked normally forward in time.
  if( ! impl_->proc_fg_update->out_tracks().empty() )
  {
    log_debug( this->name() << ": Flushing "<< impl_->proc_fg_update->out_tracks().size()
      << " active track(s) through the back-tracking pipeline.\n" );

    // Calling run_pipeline again because we want to have the tracks pass
    // through the rest of the downstream pipeline before they can be
    // published as *terminated_tracks* of this super_process.
    impl_->run_pipeline( this->pipeline_ );

    // We know that the nodes upstream of proc_tracker node would have failed,
    // therefore we will reset the whole pipeline. Note that we will lose any
    // data in the process state across this reset.
    this->pipeline_->reset();
  }

  log_assert( impl_->new_tracks->size() == impl_->terminated_tracks.size(),
    this->name() << ": There is a  discrepancy in the input ("
    << impl_->new_tracks->size() << ") and output ("
    << impl_->terminated_tracks.size() << ") tracks sizes." );

  impl_->new_tracks = NULL;
  impl_->fg_objs_buffer.clear();

#ifdef DEBUG
  vcl_cout<<"Track length check:"<<vcl_endl;
  for( unsigned i = 0; i < impl_->terminated_tracks.size(); i++ )
  {
    vcl_cout << "Track # " << impl_->terminated_tracks_[i]->id() << " from "
             << track_length_LUT[ impl_->terminated_tracks_[i]->id() ]
             << " to " << impl_->terminated_tracks_[i]->history().size()  << vcl_endl;
  }
  vcl_cout<<name()<<":- done."<<vcl_endl;
#endif

  return p_status;
}

/// -------------------------- Input Ports ----------------------------

template<class PixType>
void
back_tracking_super_process<PixType>
::set_mf_params( multiple_features const& mfp )
{
  impl_->proc_tracker->set_mf_params( mfp );
}

template<class PixType>
void
back_tracking_super_process<PixType>
::in_tracks( vcl_vector< track_sptr > const& trks )
{
  impl_->proc_track_reverser1->in_tracks( trks );
  impl_->new_tracks = &trks;
}

template<class PixType>
void
back_tracking_super_process<PixType>
::set_image_buffer( buffer< vil_image_view<PixType> > const& buf )
{
  impl_->image_buffer = &buf;
}

template<class PixType>
void
back_tracking_super_process<PixType>
::set_ts_image_buffer( paired_buffer< timestamp, vil_image_view< PixType > >
                       const& buf )
{
  impl_->ts_image_buffer = &buf;
}

template<class PixType>
void
back_tracking_super_process<PixType>
::set_gsd_buffer( paired_buffer< timestamp, double > const& buf )
{
  impl_->gsd_buffer = &buf;
}

template<class PixType>
void
back_tracking_super_process<PixType>
::set_timestamp_buffer( buffer< timestamp > const& buf )
{
  impl_->timestamp_buffer = &buf;
}

template<class PixType>
void
back_tracking_super_process<PixType>
::set_img2wld_homog_buffer( buffer< vgl_h_matrix_2d<double> > const& buf )
{
  impl_->img2wld_homog_buffer = &buf;
}

template<class PixType>
void
back_tracking_super_process<PixType>
::set_wld2img_homog_buffer( buffer< vgl_h_matrix_2d<double> > const& buf )
{
  impl_->wld2img_homog_buffer = &buf;
}

template<class PixType>
void
back_tracking_super_process<PixType>
::set_mod_buffer( buffer< vcl_vector< image_object_sptr > > const& buf )
{
  impl_->mod_buffer = &buf;
}

template< class PixType >
void
back_tracking_super_process< PixType >
::set_gui( process_smart_pointer< gui_process > gui )
{
  impl_->gui = gui;
}

template< class PixType >
void
back_tracking_super_process< PixType >
::set_fg_objs_buffer( frame_objs_type const& buf )
{
  impl_->fg_objs_buffer.insert( buf.begin(), buf.end() );
}

/// -------------------------- Output Ports ----------------------------

template<class PixType>
vcl_vector< track_sptr > const&
back_tracking_super_process<PixType>
::updated_tracks() const
{
  return impl_->terminated_tracks;
}

template< class PixType >
frame_objs_type const&
back_tracking_super_process< PixType >
::updated_fg_objs_buffer() const
{
  return impl_->updated_fg_objs_buffer;
}


} // vidtk
