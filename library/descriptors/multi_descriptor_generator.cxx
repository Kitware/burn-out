/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/multi_descriptor_generator.h>

namespace vidtk
{

// Configure the internal descriptor operating mode
bool
multi_descriptor_generator
::configure( const external_settings& settings )
{
  multi_descriptor_settings const* desc_settings =
    dynamic_cast<multi_descriptor_settings const*>(&settings);

  if( !desc_settings )
  {
    return false;
  }

  // Create internal copy of inputted external settings
  this->settings_ = *desc_settings;

  // Set internal descriptor running mode options
  generator_settings running_options;
  running_options.thread_count = settings_.thread_count;
  running_options.run_in_safe_mode = settings_.run_in_safe_mode;
  running_options.buffer_smart_ptrs_only = settings_.buffer_smart_ptrs_only;
  running_options.process_tracks = true;
  running_options.sampling_rate = 1;
  this->set_operating_mode( running_options );

  return true;
}


external_settings* multi_descriptor_generator
::create_settings()
{
  return new multi_descriptor_settings;
}


// Add a new external generator
bool
multi_descriptor_generator
::add_generator( generator_sptr& descriptor_module )
{
  // Retrieve info about this descriptor
  generator_data info;
  info.run_time_options = descriptor_module->operating_mode();
  info.counter = 0;
  info.sample_rate = info.run_time_options.sampling_rate;
  info.process_tracks = info.run_time_options.process_tracks;

  // Reconfigure the operating mode settings of the descriptor module
  info.run_time_options.thread_count = 1;
  info.run_time_options.run_in_safe_mode = false;
  info.run_time_options.process_tracks = false;
  info.run_time_options.buffer_smart_ptrs_only = true;
  descriptor_module->set_operating_mode( info.run_time_options );

  // Add the descriptor to our internal list
  descriptors_.push_back( descriptor_module );
  descriptor_info_.push_back( info );
  return true;
}


// Attempts to restart the descriptor
bool
multi_descriptor_generator
::reset_all()
{
  bool success = true;

  // Reset all added descriptor generators
  for( unsigned i = 0; i < descriptors_.size(); i++ )
  {
    if( !descriptors_[i]->reset() )
    {
      success = false;
    }
  }

  // Reset the multi-descriptor generator
  if( !this->reset() )
  {
    success = false;
  }

  return success;
}

// Flush any partially complete descriptors and return to defaults
bool
multi_descriptor_generator
::clear()
{
  // Flush out incompletes
  bool success = this->terminate_all_tracks();

  // Remove descriptors
  descriptor_info_.clear();
  descriptors_.clear();
  return success;
}

// Handle the frame step portion for all descriptors, currently this is unthreaded.
bool
multi_descriptor_generator
::frame_update_routine()
{
  bool success = true;

  // Step each descriptor, this will only call the frame update function
  // of each descriptor, and reset their descriptor output buffers
  for( unsigned i = 0; i < descriptors_.size(); i++ )
  {
    descriptors_[i]->set_input_frame( this->current_frame() );

    if( !descriptors_[i]->step() )
    {
      success = false;
    }
  }

  return success;
}

// Call terminated track routine for all descriptors
bool
multi_descriptor_generator
::terminated_track_routine( track_sptr const& finished_track,
                            track_data_sptr track_data )
{
  bool success = true;

  mdg_track_data* data = dynamic_cast<mdg_track_data*>(track_data.get());

  std::vector< track_data_sptr >& descriptor_data = data->descriptor_track_data;

  for( unsigned i = 0; i < descriptors_.size(); i++ )
  {
    if( !descriptors_[i]->terminated_track_routine( finished_track, descriptor_data[i] ) )
    {
      success = false;
    }
  }

  return success;
}

// Call new track for all descriptors
track_data_sptr
multi_descriptor_generator
::new_track_routine( track_sptr const& new_track )
{
  mdg_track_data* track_data = new mdg_track_data;
  std::vector< track_data_sptr >& descriptor_data = track_data->descriptor_track_data;

  for( unsigned i = 0; i < descriptors_.size(); i++ )
  {
    descriptor_data.push_back( descriptors_[i]->new_track_routine( new_track ) );
  }

  return track_data_sptr( track_data );
}

// Create update tasks for all tracks and all descriptors
void
multi_descriptor_generator
::formulate_tasks( const track_vector_t& active_tracks,
                   const track_vector_t& terminated_tracks,
                   task_vector_t& task_list )
{
  std::vector<bool> enable_descriptor( descriptors_.size(), false );

  // First, decide which descriptors we should call update for
  for( unsigned i = 0; i < descriptors_.size(); i++ )
  {
    // Determine to call update for the current frame on all tracks...
    descriptor_info_[i].counter++;
    if( descriptor_info_[i].counter == descriptor_info_[i].sample_rate )
    {
      descriptor_info_[i].counter = 0;
      enable_descriptor[i] = true;
    }
  }

  // For every active track, create a task to feed to our threads
  for( unsigned i = 0; i < active_tracks.size(); i++ )
  {
    track_data_sptr joint_data = this->track_data_[active_tracks[i]->id()];
    mdg_track_data* joint_data_ptr = dynamic_cast<mdg_track_data*>(joint_data.get());

    for( unsigned j = 0; j < descriptors_.size(); j++ )
    {
      if( enable_descriptor[j] )
      {
        task_list.push_back(
          task_t(
            task_t::UPDATE_ACTIVE_TRACK,
            active_tracks[i],
            *descriptors_[j],
            joint_data_ptr->descriptor_track_data[j]
          )
        );
      }
    }
  }

  for( unsigned i = 0; i < terminated_tracks.size(); i++ )
  {
    track_data_sptr joint_data = this->track_data_[terminated_tracks[i]->id()];
    mdg_track_data* joint_data_ptr = dynamic_cast<mdg_track_data*>(joint_data.get());

    for( unsigned j = 0; j < descriptors_.size(); j++ )
    {
      if( enable_descriptor[j] )
      {
        task_list.push_back(
          task_t(
            task_t::TERMINATE_TRACK,
            terminated_tracks[i],
            *descriptors_[j],
            joint_data_ptr->descriptor_track_data[j]
          )
        );
      }
    }
  }
}

// All operations are finished, lets collect all of the descriptors
bool
multi_descriptor_generator
::final_update_routine()
{
  for( unsigned i = 0; i < descriptors_.size(); i++ )
  {
    const raw_descriptor::vector_t& genout = descriptors_[i]->get_descriptors();

    for( unsigned j = 0; j < genout.size(); j++ )
    {
      this->add_outgoing_descriptor( genout[j] );
    }
  }

  return true;
}

} // end namespace vidtk
