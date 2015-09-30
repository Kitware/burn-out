/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "async_pipeline.h"

#include <vbl/vbl_smart_ptr.txx>
#include <vul/vul_timer.h>

#include <pipeline/super_process.h>
#include <utilities/log.h>

#include <boost/bind.hpp>


namespace vidtk
{

/**
 * @todo Remove duplicate typedef from the following code and centralize them here.
 */
typedef vcl_vector<pipeline_node*>::const_iterator citr;
typedef vcl_vector<pipeline_node*>::iterator itr;
typedef vcl_vector<async_pipeline_node*>::iterator aitr;


async_pipeline
::async_pipeline( status_forward_t sf )
  : status_forwarding_( sf == ENABLE_STATUS_FOWARDING ),
    feeder_is_running_( false ),
    feeder_failed_( false )
{
}


void
async_pipeline
::add( async_pipeline_node const& n )
{
  pipeline::add(new async_pipeline_node( n ));
}


void
async_pipeline
::add( process* p )
{
  pipeline::add( new async_pipeline_node( p ) );
}

void
async_pipeline
::add( process::pointer p )
{
  pipeline::add( new async_pipeline_node( p ) );
}

void
async_pipeline
::add_without_execute( async_pipeline_node const& n )
{
  pipeline::add_without_execute( new async_pipeline_node( n ) );
}


void
async_pipeline
::add_without_execute( process* p )
{
  pipeline::add_without_execute( new async_pipeline_node( p ) );
}

void
async_pipeline
::add_without_execute( process::pointer p )
{
  pipeline::add_without_execute( new async_pipeline_node( p ) );
}


/// Reset the pipeline after a failure so that it can be restarted
bool
async_pipeline
::reset()
{
  this->feeder_failed_ = false;
  return pipeline::reset();
}


/// Execute the pipeline in a loop until all processes have failed.
bool
async_pipeline
::run()
{
  vul_timer timer;
  this->run_async();
  this->wait();
  double t = timer.real();
  this->total_elapsed_ms_ += t;
  typedef vcl_vector<pipeline_node*>::const_iterator itr;
  this->total_step_count_ = 0;
  for( itr it = execution_order().begin(); it != execution_order().end(); ++it )
  {
    async_pipeline_node* node = dynamic_cast<async_pipeline_node*>(*it);
    if( node->step_count_ > this->total_step_count_ )
    {
      this->total_step_count_ = node->step_count_;
    }
  }

  if( this->print_detailed_report_ )
  {
    this->output_detailed_report();
  }

  return true;
}


/// Start asynchronous processing.
void
async_pipeline
::run_async()
{
  typedef vcl_vector<pipeline_node*>::const_iterator itr;
  for( itr it = execution_order().begin(); it != execution_order().end(); ++it )
  {
    async_pipeline_node* node = dynamic_cast<async_pipeline_node*>(*it);
    if( node->is_output_node() )
    {
      critical_threads_.create_thread(boost::bind(&async_pipeline_node::thread_job, node));
    }
    else
    {
      threads_.create_thread(boost::bind(&async_pipeline_node::thread_job, node));
    }
  }
}


/// Shut down the running threads cleanly to cancel the pipeline run
/// does not wait for all processing to finish
bool
async_pipeline
::cancel()
{
  log_info ("async_pipeline::cancel() called\n"); // TEMP
  // interrupt all running threads
  critical_threads_.interrupt_all();
  threads_.interrupt_all();
  // wait for all the threads to cleanup
  critical_threads_.join_all();
  threads_.join_all();

  //
  // traverse the graph and send cancel to all processes
  //
  for( citr it = execution_order().begin(); it != execution_order().end(); ++it )
  {
    if ((*it)->get_process())
    {
      (*it)->get_process()->cancel();
    }
  } // end for


  return true;
}


/// Return true if the threads are still running
bool
async_pipeline
::is_running()
{
  typedef vcl_vector<pipeline_node*>::const_iterator itr;
  for( itr it = execution_order().begin(); it != execution_order().end(); ++it )
  {
    async_pipeline_node* node = dynamic_cast<async_pipeline_node*>(*it);
    if( node->is_running() )
    {
      return true;
    }
  }
  return false;
}


/// Block until the threads are no longer running
void
async_pipeline
::wait()
{
  // wait for all critical threads to fail
  critical_threads_.join_all();
  // kill any non-critical threads that may still be running
  threads_.interrupt_all();
  threads_.join_all();
}


bool
async_pipeline
::run_with_pads(async_pipeline_node* parent)
{
  vul_timer timer;

  // spawn off threads for each non-pad process
  vcl_vector<async_pipeline_node*> input_pad_nodes;
  vcl_vector<async_pipeline_node*> output_pad_nodes;
  typedef vcl_vector<pipeline_node*>::const_iterator itr;
  for( itr it = execution_order().begin(); it != execution_order().end(); ++it )
  {
    async_pipeline_node* node = dynamic_cast<async_pipeline_node*>(*it);
    if ( dynamic_cast<super_process_pad*>(node->get_process().ptr()) )
    {
      if( node->is_output_node() )
      {
        output_pad_nodes.push_back(node);
      }
      else
      {
        input_pad_nodes.push_back(node);
      }
    }
    else
    {
      // only restart a job if it is not running and if it has been reset
      // last_execute_state() == SUCCESS after a reset()
      if( ! node->is_running() &&
          node->last_execute_state() == process::SUCCESS )
      {
        threads_.create_thread(boost::bind(&async_pipeline_node::thread_job,
                                           node));
      }
    }
  }

  // create a thread to handle passing data into the super process
  if( ! this->feeder_is_running() )
  {
    boost::lock_guard<boost::mutex> lock(this->mut_);
    if( ! this->feeder_failed_ )
    {
      threads_.create_thread(boost::bind(&async_pipeline::feed_pads_job, this,
                                         parent, input_pad_nodes));
    }
  }


  // handle passing data out of the super process
  typedef vcl_vector<async_pipeline_node*>::iterator aitr;

  bool thread_has_been_interrupted = false;

  process::step_status parent_edge_status = process::FAILURE;

  while( parent->last_execute_state() != process::FAILURE )
  {
    if( !this->status_forwarding_ )
    {
      // get the status from the feeder thread
      {
        boost::unique_lock<boost::mutex> lock(this->mut_);
        while(this->edge_status_queue_.empty())
        {
          this->cond_status_available_.wait(lock);
        }
        parent_edge_status = this->edge_status_queue_.back();
        this->edge_status_queue_.pop_back();
      }

      if( parent_edge_status == process::FAILURE )
      {
        parent->set_last_execute_to_failed();
      }
      else if( parent_edge_status == process::SKIP )
      {
        parent->set_last_execute_to_skip();
      }
    }
    else
    {
      parent_edge_status = process::SUCCESS;
    }

    if( this->status_forwarding_ || parent_edge_status == process::SUCCESS )
    {
      bool all_outputs_failed = true;
      bool any_outputs_skipped = false;
      // pull the data from the output queues of the inner pipeline
      for( aitr it = output_pad_nodes.begin(); it != output_pad_nodes.end(); ++it )
      {
        process::step_status edge_status = (*it)->execute_incoming_edges();
        (*it)->execute();
        if( edge_status != process::FAILURE )
        {
          all_outputs_failed = false;
        }
        if( edge_status == process::SKIP )
        {
          any_outputs_skipped = true;
        }
      }
      if( all_outputs_failed )
      {
        // attempt to recover from failure
        if( parent->recover_ && parent->recover_() )
        {
          parent->set_last_execute_to_skip();
          ++this->total_step_count_;
          ++parent->step_count_;
          double t = timer.real();
          parent->elapsed_ms_ += t;
          this->total_elapsed_ms_ += t;
          return true;
        }
        parent->set_last_execute_to_failed();
      }
      else if( any_outputs_skipped )
      {
        parent->set_last_execute_to_skip();
      }
      else
      {
        parent->set_last_execute_to_success();
      }
    }
    if( this->status_forwarding_ )
    {
      parent_edge_status = parent->last_execute_state();
    }
    super_process* sp = dynamic_cast<super_process*>(parent->get_process().ptr());
    if( sp )
    {
      sp->post_step();
    }
    parent->log_status(parent_edge_status, parent->last_execute_state());

    // if the process has failed, wait to execute outgoing edges until
    // all threads have been joined
    if( parent->last_execute_state() != process::FAILURE )
    {
      try
      {
        parent->execute_outgoing_edges();
      }
      catch( boost::thread_interrupted& e )
      {
        log_info(parent->name()<<" thread interrupted"<<vcl_endl);
        // thread interrupted, fail the process to terminate
	thread_has_been_interrupted = true;
        parent->set_last_execute_to_failed();
      }
    }
    ++this->total_step_count_;
    ++parent->step_count_;
  }

  // interrupt all running threads
  log_info("async_pipeline: interrupting all threads\n"); // TEMP
  critical_threads_.interrupt_all();
  threads_.interrupt_all();

  try
  {
    critical_threads_.join_all();
    threads_.join_all();
  }
  catch( boost::thread_interrupted& e )
  {
    // if interrupted while waiting for other interrupted threads to exit
    // then continue to wait
    thread_has_been_interrupted = true;
    critical_threads_.join_all();
    threads_.join_all();
  }

  double t = timer.real();
  parent->elapsed_ms_ += t;
  this->total_elapsed_ms_ += t;

  // send out the final FAILURE message to the downstream nodes
  parent->log_status(parent_edge_status, parent->last_execute_state());

  // do not flush if we're processing an interrupt
  if ( ! thread_has_been_interrupted )
  {
    parent->execute_outgoing_edges();
  }


  return false;
}


/// This function handles the thread for feeding data to the input pads
void
async_pipeline
::feed_pads_job(async_pipeline_node* parent,
                vcl_vector<async_pipeline_node*> input_pad_nodes)
{
  async_is_running_marker run_mark(this->feeder_is_running_, this->mut_,
                                   &this->cond_feeder_is_running_);
  {
    boost::lock_guard<boost::mutex> lock(this->mut_);
    this->feeder_failed_ = false;
  }
  process::step_status edge_status = process::SUCCESS;
  // handle passing data into the super process
  typedef vcl_vector<async_pipeline_node*>::iterator aitr;
  while( edge_status != process::FAILURE )
  {
    // get the data from the outer pipeline
    edge_status = parent->execute_incoming_edges();
    if( edge_status == process::SUCCESS || this->status_forwarding_ )
    {
      // push the data into all the input queues of the inner pipeline
      for( aitr it = input_pad_nodes.begin(); it != input_pad_nodes.end(); ++it )
      {
        (*it)->execute();
        (*it)->last_execute_state_ = edge_status;
        (*it)->execute_outgoing_edges();
      }
    }
    if( ! this->status_forwarding_ )
    {
      // use a status queue to synchronize input and output states and simulate
      // synchronous pipeline behaviour
      {
        boost::lock_guard<boost::mutex> lock(this->mut_);
        this->edge_status_queue_.push_front(edge_status);
      }
      this->cond_status_available_.notify_one();
    }
  }
  {
    boost::lock_guard<boost::mutex> lock(this->mut_);
    // this flag can be false when this function exits (with feeder_is_running_
    // also false) in the case where this thread is interrupted.
    this->feeder_failed_ = true;
  }
}


bool
async_pipeline
::feeder_is_running()
{
  boost::unique_lock<boost::mutex> lock(this->mut_);
  if( this->feeder_is_running_ )
  {
    // if the process just failed, give it a short time to wrap up
    boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);
    if( !this->cond_feeder_is_running_.timed_wait(lock, td) )
    {
      return true;
    }
    return this->feeder_is_running_;
  }
  return false;
}

} // end namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::async_pipeline );
