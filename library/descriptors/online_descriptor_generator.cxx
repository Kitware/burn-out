/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/online_descriptor_generator.h>

#include <boost/thread/mutex.hpp>

#include <queue>
#include <set>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cmath>

#include <boost/math/special_functions/fpclassify.hpp>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_online_descriptor_generator_cxx__
VIDTK_LOGGER("online_descriptor_generator_cxx");


namespace vidtk
{

class online_descriptor_generator::io_data
{
public:

  io_data() : frames_observed_(0) {}
  ~io_data() {}

  // Pointer to the latest buffer object in use by this generator
  frame_sptr latest_input_;

  // List of outgoing descriptors
  raw_descriptor::vector_t outgoing_descriptors_;

  // Internal counters
  unsigned frames_observed_;

  // Mutex objects used for attaching output descriptors & events
  boost::mutex attach_descriptor_mutex_;

  // Track inputs for the last and current frame, if process tracks enabled
  std::vector<track_sptr> current_tracks_;
  std::vector<track_sptr> prior_tracks_;

  // Returns a vector of indices for both new and terminated tracks
  void modified_tracks( std::vector< unsigned >& new_track_indices,
                        std::vector< unsigned >& terminated_track_indices );
};

// Returns any new tracks and terminated tracks. Note: returns indices
// in the current and prior track vectors, not track ids. Could be made
// more efficient.
void
online_descriptor_generator::io_data
::modified_tracks( std::vector< unsigned >& new_track_indices,
                   std::vector< unsigned >& terminated_track_indices )
{
  new_track_indices.clear();
  terminated_track_indices.clear();

  std::vector< unsigned > current_ids;
  std::vector< unsigned > prior_ids;

  for( unsigned i = 0; i < prior_tracks_.size(); ++i )
  {
    prior_ids.push_back( prior_tracks_[i]->id() );
  }

  for( unsigned i = 0; i < current_tracks_.size(); ++i )
  {
    current_ids.push_back( current_tracks_[i]->id() );
  }

  std::set< unsigned > sorted_current( current_ids.begin(), current_ids.end() );
  std::set< unsigned > sorted_prior( prior_ids.begin(), prior_ids.end() );

  for( unsigned i = 0; i < current_ids.size(); ++i )
  {
    if( sorted_prior.find( current_ids[i] ) == sorted_prior.end() )
    {
      new_track_indices.push_back(i);
    }
  }

  for( unsigned i = 0; i < prior_ids.size(); ++i )
  {
    if( sorted_current.find( prior_ids[i] ) == sorted_current.end() )
    {
      terminated_track_indices.push_back(i);
    }
  }
}

// Default initialize all variables
online_descriptor_generator
::online_descriptor_generator()
{
  this->set_operating_mode( generator_settings() );
}

// Kill all threads if using threading
online_descriptor_generator
::~online_descriptor_generator()
{
}

// Configure the internal descriptor operating mode
bool
online_descriptor_generator
::set_operating_mode( const generator_settings& settings )
{
  // Validate inputs with an arbitrarily large value
  if( settings.frame_buffer_length > 10000 )
  {
    this->report_error( "Frame buffer length too large!" );
    return false;
  }
  if( settings.thread_count == 0 )
  {
    this->report_error( "Thread count must be at least 1 or more!" );
    return false;
  }

  // Set internal settings
  running_options_ = settings;

  // Re-initialize internal variables
  io_data_.reset( new io_data() );

  // Setup multi-threading
  if( settings.thread_count > 1 )
  {
    thread_sys_.reset( new thread_sys_t( settings.thread_count ) );
  }

  // Setup new buffers
  if( running_options_.frame_buffer_length > 0 )
  {
    frame_buffer_.reset( new buffer_t( running_options_.frame_buffer_length ) );
  }

  // Reset track data
  track_data_.clear();

  return true;
}

// Attempts to restart the descriptor
bool
online_descriptor_generator
::reset()
{
  return this->set_operating_mode( running_options_ ) && reset_generator();
}

// Manually terminate *all* active tracks
bool
online_descriptor_generator
::terminate_all_tracks()
{
  bool status = true;

  if( running_options_.process_tracks )
  {
    track_vector_t& prior_tracks = io_data_->prior_tracks_;
    track_vector_t& current_tracks = io_data_->current_tracks_;

    prior_tracks = current_tracks;
    current_tracks.clear();

    for( unsigned i = 0; i < prior_tracks.size(); ++i )
    {
      const unsigned track_id = prior_tracks[i]->id();

      data_storage_t::iterator itr = track_data_.find( track_id );

      /// Note: If any call fails, the failure is delayed until
      /// all tracks are processed.
      if( !this->terminated_track_routine( prior_tracks[i], itr->second ) )
      {
        // Remember that one call failed.
        status = false;
      }
    }

    // Remove local data associates with terminated track
    track_data_.clear();
  }

  return status;
}


// Step the pipeline, keeping track of track instances and data
bool
online_descriptor_generator
::step()
{
  // Reset outgoing list
  io_data_->outgoing_descriptors_.clear();

  // Call frame step function
  bool success = this->frame_update_routine();

  // Handle tracks as necessary
  if( !success || !running_options_.process_tracks )
  {
    return success;
  }

  track_vector_t& prior_tracks = io_data_->prior_tracks_;
  track_vector_t& current_tracks = io_data_->current_tracks_;
  frame_sptr_t& latest_input = io_data_->latest_input_;

  // Cycle the internal list of tracks from the inputted buffer
  prior_tracks = current_tracks;
  current_tracks = latest_input->active_tracks();

  // If there are no supplied tracks, terminate all prior tracks
  if( current_tracks.empty()  )
  {
    for( unsigned ind = 0; ind < prior_tracks.size(); ind++ )
    {
      const unsigned track_id = prior_tracks[ind]->id();

      if( !this->terminated_track_routine( prior_tracks[ind], track_data_[track_id] ) )
      {
        success = false;
      }
    }
    prior_tracks.clear();
    track_data_.clear();
  }
  // Check for terminated and newly initialized tracks
  else
  {
    std::vector<unsigned> new_track_inds, term_track_inds;

    io_data_->modified_tracks( new_track_inds, term_track_inds );

    // Handle new tracks
    for( unsigned i = 0; i < new_track_inds.size(); ++i )
    {
      const unsigned ind = new_track_inds[i];
      const unsigned track_id = current_tracks[ind]->id();
      track_data_[ track_id ] = this->new_track_routine( current_tracks[ind] );
    }

    // Handle terminated tracks
    track_vector_t terminated_tracks;
    for( unsigned i = 0; i < term_track_inds.size(); ++i )
    {
      terminated_tracks.push_back( prior_tracks[ term_track_inds[i] ] );
    }

    // Formulate and run track update and termination tasks
    task_vector_t task_list;
    this->formulate_tasks( current_tracks, terminated_tracks, task_list );

    if( running_options_.thread_count > 1 )
    {
      success = thread_sys_->process_tasks( task_list );
    }
    else
    {
      for( unsigned i = 0; i < task_list.size(); ++i )
      {
        if( !task_list[i].execute() )
        {
          success = false;
        }
      }
    }

    // Delete data for terminated tracks
    for( unsigned i = 0; i < term_track_inds.size(); ++i )
    {
      const unsigned track_id = prior_tracks[ term_track_inds[i] ]->id();
      track_data_.erase( track_data_.find( track_id ) );
    }
  }

  // Call final post-track update routine
  if( !this->final_update_routine() )
  {
    success = false;
  }

  return success;
}


// Retrieve a list of any descriptors generated since the last step
raw_descriptor::vector_t
online_descriptor_generator
::get_descriptors()
{
  return io_data_->outgoing_descriptors_;
}

// Add outgoing descriptor
bool
online_descriptor_generator
::add_outgoing_descriptor( descriptor_sptr const& descriptor )
{
  if( !descriptor )
  {
    LOG_WARN( "Attempted to output an invalid descriptor" );
    return false;
  }

  if( running_options_.run_in_safe_mode )
  {
    // Scan data contents for NaN
    const descriptor_data_t& data = descriptor->get_features();
    for( unsigned i = 0; i < data.size(); ++i )
    {
      if( boost::math::isnan( data[i] ) )
      {
        return false;
      }
    }
  }

  if( running_options_.append_modality_to_id &&
      current_frame()->was_modality_set() )
  {
    std::string extension;

    if( is_video_modality_eo( current_frame()->modality() ) )
    {
      extension = ".eo";
    }
    else if( is_video_modality_ir( current_frame()->modality() ) )
    {
      extension = ".ir";
    }

    // Dis-qualify const only for this special case
    const_cast< raw_descriptor* >( descriptor.ptr() )->set_type(
      descriptor->get_type() + extension );
  }

  boost::unique_lock<boost::mutex> lock( io_data_->attach_descriptor_mutex_ );
  io_data_->outgoing_descriptors_.push_back( descriptor );
  return true;
}

// Add tasks to update active thread data
void
online_descriptor_generator
::formulate_tasks( const track_vector_t& active_tracks,
                   const track_vector_t& terminated_tracks,
                   task_vector_t& task_list )
{
  for( unsigned i = 0; i < active_tracks.size(); ++i )
  {
    task_list.push_back(
      task_t(
        task_t::UPDATE_ACTIVE_TRACK,
        active_tracks[i],
        *this,
        track_data_[ active_tracks[i]->id() ]
      )
    );
  }

  for( unsigned i = 0; i < terminated_tracks.size(); ++i )
  {
    task_list.push_back(
      task_t(
        task_t::TERMINATE_TRACK,
        terminated_tracks[i],
        *this,
        track_data_[ terminated_tracks[i]->id() ]
      )
    );
  }
}

// Report a critical error
void
online_descriptor_generator
::report_error( const std::string& error )
{
  LOG_FATAL( error );
  throw std::runtime_error( error );
}

unsigned
online_descriptor_generator
::frame_buffer_length()
{
  return frame_buffer_->size();
}

boost::shared_ptr< frame_data > const&
online_descriptor_generator
::current_frame()
{
  return io_data_->latest_input_;
}

boost::shared_ptr< frame_data > const&
online_descriptor_generator
::get_frame( unsigned i )
{
  return frame_buffer_->get_frame( i );
}

generator_settings const&
online_descriptor_generator
::operating_mode()
{
  return running_options_;
}

// Set inputs
void
online_descriptor_generator
::set_input_frame( const frame_sptr& frame )
{
  io_data_->latest_input_ = frame;

  if( running_options_.frame_buffer_length > 0 )
  {
    if( running_options_.buffer_smart_ptrs_only )
    {
      frame_buffer_->insert_frame( frame );
    }
    else
    {
      frame_sptr copied_frame( new frame_data );
      copied_frame->deep_copy( *frame );
      frame_buffer_->insert_frame( copied_frame );
      io_data_->latest_input_ = copied_frame;
    }
  }
}

} // end namespace vidtk
