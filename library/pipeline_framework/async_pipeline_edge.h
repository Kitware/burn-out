/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_async_pipeline_edge_h_
#define vidtk_async_pipeline_edge_h_

#include <boost/function.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include <string>
#include <list>

#include <pipeline_framework/pipeline_edge.h>

/*#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_async_pipeline_edge_h__
VIDTK_LOGGER("async_pipeline_edge_h");
*/


namespace vidtk
{

/// These classes are used to strip the & or const & from the edge data type.
template<class T>
struct async_pipeline_edge_data
{
  typedef T type;
};

template<class T>
struct async_pipeline_edge_data<const T&>
{
public:
  typedef T type;
};

template<class T>
struct async_pipeline_edge_data<T&>
{
public:
  typedef T type;
};


template<class OT, class IT>
struct async_pipeline_edge_impl
  : pipeline_edge_impl<OT, IT>
{
  typedef typename async_pipeline_edge_data<OT>::type nonref_OT;
  std::list<nonref_OT> data_queue_;
  std::list<process::step_status> status_queue_;
  boost::condition_variable cond_data_available;
  boost::condition_variable cond_queue_not_full;
  boost::mutex mut;
  unsigned max_queue_size;
  nonref_OT last_data;
  bool no_more_input;
  unsigned last_size;

  async_pipeline_edge_impl( unsigned edge_capacity )
    : max_queue_size( edge_capacity ),
      no_more_input( false ),
      last_size( 0 )
  {
  }

  ~async_pipeline_edge_impl()
  {
  }

  /// Remove all success messages from the internal status queue.
  virtual void flush_status_queue()
  {
    typedef std::list<process::step_status>::iterator itr;

    if( !this->status_queue_.empty() )
    {
      std::list<process::step_status> new_list;

      for( itr p = this->status_queue_.begin();
           p != this->status_queue_.end();
           p++ )
      {
        if( *p != process::SUCCESS )
        {
          new_list.push_back( *p );
        }
      }

      this->status_queue_ = new_list;
    }
  }

  /// Pull the data from the source node and add push it into the async queue.
  virtual void pull_data()
  {
    {
      boost::unique_lock<boost::mutex> lock(this->mut);
      if( this->from_node_last_execute_state() == process::FLUSH )
      {
        this->flush_status_queue();
        this->data_queue_.clear();
      }
      while( this->max_queue_size != 0 && this->status_queue_.size() > this->max_queue_size )
      {
        this->cond_queue_not_full.wait(lock);
      }
      this->status_queue_.push_front(this->from_node_last_execute_state());
      this->no_more_input = false;
      if( this->from_node_last_execute_state() == process::SUCCESS )
      {
        this->data_queue_.push_front(this->get_output_());
      }
      last_size = this->status_queue_.size();
    }
    this->cond_data_available.notify_one();
  }


  /// Pop data from the async queue and push the data to the sink node.
  virtual process::step_status push_data()
  {
    process::step_status status;
    {
      boost::unique_lock<boost::mutex> lock(this->mut);
      // When the status queue is empty and the last status was FAILURE then
      // assume that no more input is coming and continue to return FAILURE.
      // This is required for optional connections that continue to run and
      // probe for input after a failure.
      if( this->no_more_input )
      {
        return process::FAILURE;
      }
      while(status_queue_.empty())
      {
        this->cond_data_available.wait(lock);
      }
      status = this->status_queue_.back();
      this->status_queue_.pop_back();
      if( status == process::SUCCESS )
      {
        this->last_data = this->data_queue_.back();
        this->data_queue_.pop_back();
        this->set_input_( this->last_data );
      }
      else if( status == process::FAILURE && this->status_queue_.empty() )
      {
        this->no_more_input = true;
      }
      last_size = this->status_queue_.size();
    }
    this->cond_queue_not_full.notify_one();
    return status;
  }

  virtual bool reset()
  {
    async_pipeline_node* to = dynamic_cast<async_pipeline_node*>(this->to_);
    if( to && to->is_running() )
    {
      LOG_ERROR("Failed to reset edge with running downstream node " << to->name() );
      return false;
    }
    if( this->status_queue_.size() > 0 || this->data_queue_.size() > 0 )
    {
      LOG_ERROR("During reset of edge, edge queue length non-zero" );
    }
    no_more_input = false;
    return true;
  }

  /// Returns the length of the queue for this edge.
  virtual unsigned edge_queue_length() const
  {
    return last_size;
  }
};


} // end namespace vidtk


#endif // async_pipeline_edge_h_
