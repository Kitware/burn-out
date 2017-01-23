/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "online_descriptor_thread_system.h"
#include "online_descriptor_generator.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "online_descriptor_thread_system_cxx" );

online_descriptor_thread_task
::online_descriptor_thread_task(
  const task_activity act,
  const track_sptr& trk,
  online_descriptor_generator& gen,
  track_data_sptr& trk_data )
{
  action = act;
  track_ref = &trk;
  generator_ref = &gen;
  data_ref = &trk_data;
}

bool
online_descriptor_thread_task
::execute()
{
  LOG_ASSERT( generator_ref, "Invalid descriptor generator pointer" );

  if( action == UPDATE_ACTIVE_TRACK )
  {
    return generator_ref->track_update_routine( *(track_ref), *(data_ref) );
  }
  else if( action == TERMINATE_TRACK )
  {
    return generator_ref->terminated_track_routine( *(track_ref), *(data_ref) );
  }

  LOG_FATAL( "Invalid task action received" );
  return false;
}

online_descriptor_thread_system
::online_descriptor_thread_system( unsigned thread_count )
{
  LOG_ASSERT( thread_count > 0, "Thread count must exceed 0" );

  thread_count_ = thread_count;
  thread_worker_queues_= new std::queue< task_t >[ thread_count_ ];
  thread_cond_var_ = new boost::condition_variable[ thread_count_ ];
  thread_muti_ = new boost::mutex[ thread_count_ ];
  thread_status_ = new thread_status[ thread_count_ ];

  // Launch worker threads
  for( unsigned id = 0; id < thread_count_; id++ )
  {
    thread_status_[ id ] = THREAD_WAITING;

    threads_.create_thread(
      boost::bind(
        &online_descriptor_thread_system::worker_thread,
        this,
        id
      )
    );
  }
}

online_descriptor_thread_system
::~online_descriptor_thread_system()
{
  this->shutdown();
}

void
online_descriptor_thread_system
::shutdown()
{
  if( thread_worker_queues_ != NULL )
  {
    threads_.interrupt_all();
    threads_.join_all();

    delete[] thread_worker_queues_;
    delete[] thread_cond_var_;
    delete[] thread_muti_;
    delete[] thread_status_;
  }

  thread_worker_queues_ = NULL;
  thread_cond_var_ = NULL;
  thread_muti_ = NULL;
  thread_status_ = NULL;
}

void
online_descriptor_thread_system
::worker_thread( unsigned thread_id )
{
  try
  {
    // Get pointers to required objects
    boost::mutex *thread_mutex = &thread_muti_[thread_id];
    std::queue< task_t > *thread_queue = &thread_worker_queues_[thread_id];
    boost::condition_variable *thread_cond_var = &thread_cond_var_[thread_id];
    thread_status *current_status = &thread_status_[thread_id];

    // Initiate main loop which will execute until interrupted when we are shutting down
    while( *current_status != THREAD_TERMINATED )
    {
      // Wait until we have new data to process
      {
        boost::unique_lock<boost::mutex> lock( *thread_mutex );
        while( *current_status != THREAD_TASKED )
        {
          thread_cond_var->timed_wait( lock, boost::posix_time::milliseconds(1000) );
        }
      }

      // Update thread status
      *current_status = THREAD_RUNNING;

      // While there is still data left in our queue
      bool success = true;

      while( !thread_queue->empty() )
      {
        // Pop an entry
        task_t to_do = thread_queue->front();
        thread_queue->pop();

        if( !to_do.execute() )
        {
          success = false;
        }
      }

      // Signal that processing is complete for this thread
      *current_status = ( success ? THREAD_FINISHED_W_SUCCESS : THREAD_FINISHED_W_FAILURE );
      thread_cond_var->notify_one();
    }
  }
  catch( boost::thread_interrupted const& )
  {
    LOG_DEBUG( "Descriptor worker thread " << thread_id << " shutting down." );
  }
  catch(...)
  {
    LOG_ERROR( "Descriptor worker thread " << thread_id << " has crashed!" );
  }
}

bool
online_descriptor_thread_system
::process_tasks( const std::vector< task_t >& tasks )
{
  bool ret_flag = true;

  // Assign tasks to threads
  for( unsigned i = 0; i < tasks.size(); i++ )
  {
    thread_worker_queues_[ i % thread_count_ ].push( tasks[i] );
  }

  // Start threads
  for( unsigned thread_id = 0; thread_id < thread_count_; thread_id++ )
  {
    if( thread_worker_queues_[ thread_id ].size() != 0 )
    {
      thread_status_[ thread_id ] = THREAD_TASKED;
      thread_cond_var_[ thread_id ].notify_one();
    }
    else
    {
      thread_status_[ thread_id ] = THREAD_FINISHED_W_SUCCESS;
    }
  }

  // Wait for all threads to finish their jobs
  for( unsigned thread_id = 0; thread_id < thread_count_; thread_id++ )
  {
    boost::unique_lock<boost::mutex> lock( this->thread_muti_[ thread_id ] );

    while( thread_status_[ thread_id ] != THREAD_FINISHED_W_SUCCESS &&
           thread_status_[ thread_id ] != THREAD_FINISHED_W_FAILURE )
    {
      thread_cond_var_[ thread_id ].timed_wait( lock, boost::posix_time::milliseconds(1000) );
    }

    if( thread_status_[ thread_id ] == THREAD_FINISHED_W_FAILURE )
    {
      ret_flag = false;
    }

    thread_status_[ thread_id ] = THREAD_WAITING;
  }

  return ret_flag;
}

} // end namespace vidtk
