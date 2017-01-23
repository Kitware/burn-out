/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "training_thread.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER("training_thread_cxx_");

training_thread
::training_thread()
{
  current_thread_index_ = 0;
  launch_new_thread( 0 );
}

training_thread
::~training_thread()
{
  if( current_thread_->status == THREAD_RUNNING_UNTASKED ||
      current_thread_->status == THREAD_RUNNING_TASKED )
  {
    current_thread_->status = THREAD_ABANDONED;
    current_thread_->thread->interrupt();
    current_thread_->thread->join();
  }

  while( !old_threads_.empty() )
  {
    if( old_threads_.front()->status != THREAD_TERMINATED )
    {
      old_threads_.front()->thread->join();
    }
    old_threads_.pop();
  }
}

void
training_thread
::launch_new_thread( index_t id )
{
  current_thread_.reset( new thread_info_t() );
  current_thread_->status = THREAD_RUNNING_UNTASKED;
  current_thread_->index = id;

  current_thread_->thread.reset(
    new thread_t(
      boost::bind( &training_thread::thread_job, this, current_thread_ )
    )
  );
}

void
training_thread
::execute_if_able( training_thread_task_sptr& task )
{
  lock_t lock( current_thread_->mutex );
  current_thread_->task_queue.push_back( task );
  current_thread_->in_cond_var.notify_one();
}

void
training_thread
::block_until_finished()
{
  lock_t lock( current_thread_->mutex );

  while( current_thread_->status != THREAD_RUNNING_UNTASKED )
  {
    current_thread_->out_cond_var.timed_wait( lock, boost::posix_time::milliseconds(1000) );
  }
}

void
training_thread
::forced_reset()
{
  if( current_thread_->status != THREAD_RUNNING_UNTASKED )
  {
    lock_t lock( current_thread_->mutex );

    current_thread_index_++;

    task_queue_t& queue = current_thread_->task_queue;

    for( task_queue_t::iterator p = queue.begin(); p != queue.end(); ++p )
    {
      (*p)->mark_as_invalid();
    }

    current_thread_->status = THREAD_ABANDONED;
    current_thread_->thread->interrupt();
    launch_new_thread( current_thread_index_ );
  }
}

void
training_thread
::thread_job( thread_info_sptr_t info )
{
  try
  {
    // Initiate main loop which will execute until interrupted when we are shutting down
    while( info->status != THREAD_ABANDONED ||
           info->status != THREAD_TERMINATED )
    {
      training_thread_task_sptr task_ptr;

      {
        lock_t lock( info->mutex );

        if( info->task_queue.empty() )
        {
          info->status = THREAD_RUNNING_UNTASKED;
        }

        while( info->task_queue.empty() )
        {
          info->in_cond_var.timed_wait( lock, boost::posix_time::milliseconds(1000) );
        }

        while( info->task_queue.size() > 1 )
        {
          info->task_queue.pop_front();
        }

        task_ptr = info->task_queue.front();
        info->task_queue.pop_front();

        if( info->status == THREAD_ABANDONED )
        {
          break;
        }

        info->status = THREAD_RUNNING_TASKED;
      }

      if( !task_ptr->execute() )
      {
        LOG_ERROR( "Error occured while executing training task!" );
      }

      info->out_cond_var.notify_one();
    }
  }
  catch( boost::thread_interrupted const& )
  {
    LOG_DEBUG( "Trainer worker thread shutting down." );
  }
  catch(...)
  {
    LOG_ERROR( "Trainer worker thread has thrown an exception!" );
  }

  info->status = THREAD_TERMINATED;
}

} // end namespace vidtk
