/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <pipeline_framework/async_pipeline_node.h>
#include <pipeline_framework/super_process.h>
#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/async_pipeline_edge.h>

#include <utilities/thread_util.h>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_async_pipeline_node_cxx__
VIDTK_LOGGER("pipeline.async_node");

// Controls local verbose debugging that you don't want on all the
// time.
#define LOCAL_DEBUG 0

namespace vidtk
{


async_pipeline_node
::async_pipeline_node( node_id_t p )
  : pipeline_node( p ), is_running_(false), is_critical_(false)
{
  p->set_push_output_func( boost::bind( &async_pipeline_node::push_output, this, _1 ) );
}

async_pipeline_node
::async_pipeline_node( process::pointer p )
  : pipeline_node( p ), is_running_(false), is_critical_(false)
{
  p->set_push_output_func( boost::bind( &async_pipeline_node::push_output, this, _1 ) );
}

async_pipeline_node
::async_pipeline_node(const async_pipeline_node& other)
  : pipeline_node(other), is_running_(false), is_critical_(false)
{
}

async_pipeline_node
::~async_pipeline_node()
{
  boost::unique_lock<boost::mutex> lock(this->thread_mut_);
  if(thread_)
  {
    if(this->is_running())
    {
      thread_->interrupt();
      thread_->join();
    }
    thread_.reset();
  }

  // Deleting the outgoing (instead of incoming) edges in case of async
  // pipeline. This is only slightly better option than the incoming
  // edges as it follows the top-down data flow. The crashes
  // due to unprotected pointers along the edges are still possible but
  // hopefully less likely (as observed in one case).
  typedef std::vector<pipeline_edge*>::iterator itr;
  for( itr i = outgoing_edges_.begin(); i != outgoing_edges_.end(); ++i )
  {
    delete *i;
  }
}


/// Reset the pipeline node after a failure so that it can be restarted
bool
async_pipeline_node
::reset()
{
  if( this->is_running() )
  {
    LOG_ERROR("Failed to reset running node "<<this->name() );
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

  try
  {
    if (super_process * s = dynamic_cast<super_process*>(this->process_.ptr()))
    {
      s->run(this);
    }
    else
    {
      this->run();
    }
  }
  catch ( boost::thread_interrupted const& )
  {
    LOG_DEBUG( "Thread interrupt for process " <<
               ( (this->process_) ? this->process_->name() : "-- no process --") );
  }
}


void
async_pipeline_node
::run()
{
  if( !this->is_executable_ )
  {
    return;
  }

  while( this->last_execute_state() != process::FAILURE )
  {
    process::step_status edge_status = this->execute_incoming_edges();
    process::step_status node_status = process::FAILURE;

    bool skip_recover = false;

    if( edge_status == process::FAILURE )
    {
      this->set_last_execute_to_failed();
    }
    else if( edge_status == process::SKIP )
    {
      if( this->skipped() )
      {
        node_status = this->execute();
        skip_recover = true;
      }
      else
      {
        this->set_last_execute_to_skip();
      }
    }
    else if( edge_status == process::FLUSH )
    {
      this->set_last_execute_to_flushed();
    }
    else
    {
      node_status = this->execute();
    }

    this->log_status(edge_status, skip_recover, node_status);

    if( node_status != process::NO_OUTPUT )
    {
      this->execute_outgoing_edges();
    }
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
  bool found_flush = false;
  typedef std::vector<pipeline_edge*>::iterator itr;
  itr it = incoming_edges_.begin();
  for( ; it != incoming_edges_.end(); ++it )
  {
    // push data from the edge to the node
    // always pops from the edge status queue
    // pops from the edge data queue if status is SUCCESS
    process::step_status status = (*it)->push_data();

#if LOCAL_DEBUG
    // you don't want this on all the time, but it is handy to find
    // out which edge won't quit
    LOG_TRACE( "Moving data on incoming edge from " << (*it)->from_port_name_
               << " to " << (*it)->to_port_name_
               << " for process " << this->get_process()->name()
               << " status: " << status );
#endif

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
    if( status == process::FLUSH )
    {
      found_flush = true;
      break;
    }
    else if( !(*it)->optional_connection_ &&
             status == process::FAILURE )
    {
      found_failure = true;
    }
  }

  if( found_flush )
  {
    // in order to clear all incoming edges, and have them remain synchronized
    // for future inputs at the same time, we must wait until we have received
    // the flush command on all incoming edges. We know that we have already
    // received a flush command for the iterator it.
    for( itr it2 = incoming_edges_.begin(); it2 != incoming_edges_.end(); ++it2 )
    {
      if( it2 != it )
      {
        while( (*it2)->push_data() != process::FLUSH );
      }
    }
    return process::FLUSH;
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
  typedef std::vector<pipeline_edge*>::iterator itr;
  for( itr it = outgoing_edges_.begin();
        it != outgoing_edges_.end(); ++it )
  {
    // pull the data and last executed state from the node into the outgoing edge
    (*it)->pull_data();

#if LOCAL_DEBUG
    // you don't want this on all the time, but it is handy to find
    // out which edge won't quit
    LOG_TRACE( "Moving data on outgoing edge from " << (*it)->from_port_name_
               << " to " << (*it)->to_port_name_
               << " for process " << this->get_process()->name() );
#endif
  }
  return process::SUCCESS;
}

unsigned
async_pipeline_node
::max_outgoing_queue_length() const
{
  unsigned max = 0;

  for( unsigned i = 0; i < this->outgoing_edges_.size(); i++ )
  {
    const unsigned edge_length = outgoing_edges_[i]->edge_queue_length();

    if( edge_length > max )
    {
      max = edge_length;
    }
  }

  return max;
}

bool
async_pipeline_node
::is_running()
{
  boost::unique_lock<boost::mutex> lock(this->mut_);
  return is_running_internal(lock);
}

bool
async_pipeline_node
::is_running_internal(  boost::unique_lock<boost::mutex> & lock )
{
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

void
async_pipeline_node
::manage_thread(bool is_crit)
{
  boost::unique_lock<boost::mutex> lock(this->thread_mut_);
  is_critical_ = is_crit;
  // only restart a job if it is not running and if it has been reset
  // last_execute_state() == SUCCESS after a reset()
  if(!thread_ || ( !this->is_running() && this->last_execute_state() == process::SUCCESS ) )
  {
    thread_.reset(new boost::thread( boost::bind(&async_pipeline_node::thread_job, this) ));
  }
  //else do nothing.  There is only one thread per process.
}

void
async_pipeline_node
::wait()
{
  boost::unique_lock<boost::mutex> lock(this->thread_mut_);
  if(thread_)
  {
    if(is_critical_)
    {
      thread_->join();
    }
    else
    {
      thread_->interrupt();
      thread_->join();
    }
  }
  //If there is not thread, this is a no op
}

void
async_pipeline_node
::cancel()
{
  boost::unique_lock<boost::mutex> lock(this->thread_mut_);
  if(thread_)
  {
    thread_->interrupt();
    thread_->join();
  }
  //If there is not thread, this is a no op
}

void
async_pipeline_node
::interrupt()
{
  boost::unique_lock<boost::mutex> lock(this->thread_mut_);
  if(thread_)
  {
    thread_->interrupt();
  }
}

void
async_pipeline_node
::join()
{
  boost::unique_lock<boost::mutex> lock(this->thread_mut_);
  if(thread_)
  {
    thread_->join();
  }
}

} // end namespace vidtk
