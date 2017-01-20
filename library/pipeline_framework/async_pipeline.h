/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_async_pipeline_h_
#define vidtk_async_pipeline_h_

#include <vbl/vbl_smart_ptr.h>
#include <pipeline_framework/pipeline.h>
#include <pipeline_framework/async_pipeline_node.h>
#include <pipeline_framework/async_pipeline_edge.h>
#include <process_framework/process.h>
#include <utilities/deprecation.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_async_pipeline_h__
VIDTK_LOGGER("async_pipeline_h");


namespace vidtk
{

/// A helper class to keep track of whether or not a thread is still running.
/// A an external bool is set to true in the contructor and false in the
/// destructor (locking with the provided mutex).  Creating an instance of
/// this class on the stack in the thread function will cause this variable
/// to be automatically set to false when the function exits, even if
/// the thread is interrupted or some other exception occurs.
class async_is_running_marker
{
public:
  async_is_running_marker(bool &is_running, boost::mutex &mut,
                          boost::condition_variable *cond = NULL)
  : is_running_( &is_running ),
    mut_( &mut ),
    cond_( cond )
  {
    boost::lock_guard<boost::mutex> lock(*this->mut_);
    *is_running_ = true;
  }

  ~async_is_running_marker()
  {
    {
      boost::lock_guard<boost::mutex> lock(*this->mut_);
      *is_running_ = false;
    }
    if( cond_ )
    {
      cond_->notify_all();
    }
  }

private:
  bool *is_running_;
  boost::mutex *mut_;
  boost::condition_variable *cond_;
};


class async_pipeline
  : public pipeline_impl<async_pipeline>
{
public:

  template <class From, class To>
  struct edge
  {
    typedef async_pipeline_edge_impl<From,To> type;
  };

  /*
   * Given the following pipeline:
   *
   *          ____super___
   *         |            |
   *  src -> | -> dupe -> | -> sink
   *         |            |
   *         |___process__|
   *
   * where 'src' outputs:
   *
   *   10 20 30 <fail>
   *
   * and 'dupe' outputs each input twice in succession, if the super process'
   * pipeline uses ENABLE_STATUS_FORWARDING, the 'sink' process will see:
   *
   *   10 10 20 20 30 30 <fail>
   *
   * and DISABLE_STATUS_FORWARDING:
   *
   *   10 10 20 <fail>
   *
   * this is because with DISABLE_STATUS_FORWARDING, when the super process
   * sees failure on its input, it will forward the status to the super process
   * outputs directly rather than to the inputs of its subprocesses.
   */
  /// Values to make enabling and disabling of status forwarding explicit.
  enum status_forward_t {DISABLE_STATUS_FORWARDING,
                         ENABLE_STATUS_FORWARDING};

  /// Pipeline status forwarding can be enabled or disabled in the constructor.
  /// Status forwarding means that \c process::step_status states received
  /// in an super process are pushed directly into the nested pipeline.
  /// Status forwarding breaks compatiblity with the sync_pipeline.
  /// For example, if input to the super process fails, the super process must
  /// fail with fowarding disabled.  With forwarding enabled it could continue
  /// to run forever if the super process contains an infinite source node.
  /// Status forwarding also allows some more advanced behavior.
  /// A super process can produce more outputs than the number of inputs it
  /// receives.  With status forwarding disabled this would result in dead lock.
  async_pipeline(status_forward_t sf = DISABLE_STATUS_FORWARDING,
                 unsigned edge_capacity = 10);

  virtual ~async_pipeline();

  void add( async_pipeline_node const& n );

  virtual void add( node_id_t p );

  virtual void add( process::pointer p );

  // Add a pipeline node that will not execute, but
  // will be part of the pipeline. Useful for allowing
  // a configuration file to contain settings for
  // pipeline elements that are not always used.
  void add_without_execute( async_pipeline_node const& n );

  // Add a process that will not execute, but
  // will be part of the pipeline. Useful for allowing
  // a configuration file to contain settings for
  // pipeline elements that are not always used.
  virtual void add_without_execute( node_id_t p );
  virtual void add_without_execute( process::pointer p );

  /// Reset the pipeline after a failure so that it can be restarted
  virtual bool reset();

  /// Asynchronous pipelines cannot execute one step at a time.
  process::step_status execute()
  {
    LOG_ERROR("Cannot execute an asynchronous pipeline.");
    return process::FAILURE;
  }

  /// Execute the pipeline in a loop until all processes have failed.
  /// This is a blocking call.  It does not return until all processing
  /// is finished.
  virtual bool run();

  /// Start asynchronous processing.
  /// This is a non-blocking call.  It launches all threads and then
  /// returns immediately.
  void run_async();

  /// Shut down the running threads cleanly to cancel the pipeline run
  /// does not wait for all processing to finish
  virtual bool cancel();

  /// Return true if the threads are still running
  bool is_running();

  /// Block until the threads are no longer running
  void wait();

  static bool const is_async = true;

protected:
  friend class super_process;
  friend class async_pipeline_node;
  /// This function handles running nested asynchronous pipelines
  /// Returns true if the pipeline was reset and needs to be re-run
  bool run_with_pads(async_pipeline_node* parent);

  /// This function handles the thread for feeding data to the input pads
  void feed_pads_job(async_pipeline_node* parent,
                     std::vector<async_pipeline_node*> input_pad_nodes);

  /// Return true if the feeder thread is running
  bool feeder_is_running();

  /// Join all critical threads
  void join_all_critical();
  /// Join all non-critical threads
  void join_all_non_critical();

  /// Interrupt all critical threads
  void interrupt_all_critical();
  /// Interrupt all non-critical threads
  void interrupt_all_non_critical();


private:
  bool status_forwarding_;

  //The pad_thread_ is a special case so we manage it here.
  boost::shared_ptr<boost::thread> pad_thread_;
  boost::condition_variable cond_status_available_;
  boost::mutex mut_, pad_thread_mut_;
  std::list<process::step_status> edge_status_queue_;
  bool feeder_is_running_;
  boost::condition_variable cond_feeder_is_running_;
  bool feeder_failed_;
};

typedef vbl_smart_ptr<async_pipeline> async_pipeline_sptr;

} // end namespace vidtk


#endif // async_pipeline_h_
