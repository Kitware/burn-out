/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */



#include <pipeline/async_pipeline_node.h>
#include <pipeline/super_process.h>
#include <pipeline/async_pipeline.h>
#include <utilities/log.h>
#include <utilities/thread_util.h>

namespace vidtk
{

async_pipeline_node
::async_pipeline_node()
  : is_running_(false)
{
}


async_pipeline_node
::async_pipeline_node( process* p )
  : pipeline_node( p ), is_running_(false)
{
  p->set_push_output_func( boost::bind( &async_pipeline_node::push_output, this, _1 ) );
}


async_pipeline_node
::async_pipeline_node( process::pointer p )
  : pipeline_node( p ), is_running_(false)
{
  p->set_push_output_func( boost::bind( &async_pipeline_node::push_output, this, _1 ) );
}

async_pipeline_node
::async_pipeline_node(const async_pipeline_node& other)
  : pipeline_node(other), is_running_(false)
{
}


/// Reset the pipeline node after a failure so that it can be restarted
bool
async_pipeline_node
::reset()
{
  if( this->is_running() )
  {
    log_error("Failed to reset running node "<<this->name()<<vcl_endl);
    return false;
  }
  // call base class reset
  return pipeline_node::reset();
}


void
async_pipeline_node
::thread_job()
{
  // Can't always rely on the process pointer being available.
  if (this->process_)
  {
    name_thread( this->process_->name() );
  }

  // sets and unsets the is_running_ flag automatically
  async_is_running_marker run_mark(this->is_running_, this->mut_,
                                   &this->cond_is_running_);

  if (super_process * s = dynamic_cast<super_process*>(this->process_.ptr()))
  {
    s->run(this);
  }
  else
  {
    this->run();
  }
}


void
async_pipeline_node
::run()
{
  while( this->last_execute_state() != process::FAILURE )
  {
    process::step_status edge_status = this->execute_incoming_edges();
    process::step_status node_status = process::FAILURE;
    if( edge_status == process::FAILURE )
    {
      this->set_last_execute_to_failed();
    }
    else if( edge_status == process::SKIP )
    {
      this->set_last_execute_to_skip();
    }
    else
    {
      node_status = this->execute();
    }
    this->log_status(edge_status, node_status);

    this->execute_outgoing_edges();
  }
}


void
async_pipeline_node
::push_output( process::step_status status )
{
  this->last_execute_state_ = status;
  this->execute_outgoing_edges();
}


process::step_status
async_pipeline_node
::execute_incoming_edges()
{
  bool found_skip = false;
  bool found_failure = false;
  typedef vcl_vector<pipeline_edge*>::iterator itr;
  for( itr it = incoming_edges_.begin();
        it != incoming_edges_.end(); ++it )
  {
    // push data from the edge to the node
    // always pops from the edge status queue
    // pops from the edge data queue if status is SUCCESS
    process::step_status status = (*it)->push_data();
#ifdef VIDTK_SKIP_IS_PROPOGATED_OVER_OPTIONAL_CONNECTIONS
    bool skip_optional = true;
#else
    bool skip_optional = !(*it)->optional_connection_;
#endif
    if( skip_optional &&
        status == process::SKIP )
    {
      found_skip = true;
    }
    else if( !(*it)->optional_connection_ &&
             status == process::FAILURE )
    {
      found_failure = true;
    }
  }


  if( found_failure )
  {
    return process::FAILURE;
  }
  if( found_skip )
  {
    return process::SKIP;
  }

  // If we get to here, we executed all the edges, and therefore set
  // all the required inputs.
  //
  return process::SUCCESS;
}


process::step_status
async_pipeline_node
::execute_outgoing_edges()
{
  typedef vcl_vector<pipeline_edge*>::iterator itr;
  for( itr it = outgoing_edges_.begin();
        it != outgoing_edges_.end(); ++it )
  {
    // pull the data and last executed state from the node into the outgoing edge
    (*it)->pull_data();
  }
  return process::SUCCESS;
}


bool
async_pipeline_node
::is_running()
{
  boost::unique_lock<boost::mutex> lock(this->mut_);
  if( this->is_running_ )
  {
    // if the process just failed, give it a short time to wrap up
    boost::posix_time::time_duration td = boost::posix_time::milliseconds(100);
    if( !this->cond_is_running_.timed_wait(lock, td) )
    {
      return true;
    }
    return this->is_running_;
  }
  return false;
}

} // end namespace vidtk
