/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_async_pipeline_node_h_
#define vidtk_async_pipeline_node_h_


#include <pipeline_framework/pipeline_node.h>
#include <process_framework/process.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>

namespace vidtk
{

class async_pipeline;
class super_process;

class async_pipeline_node
  : public pipeline_node
{
public:

  async_pipeline_node( node_id_t p );

  async_pipeline_node( process::pointer p );

  virtual ~async_pipeline_node();

  /// Return true if the process is still running
  bool is_running();

  /// Returns the maximum output queue length for this node
  virtual unsigned max_outgoing_queue_length() const;

protected:
  friend class async_pipeline;
  friend class super_process;

  ///Thread management stuff
  void manage_thread(bool is_critical);
  void interrupt();
  void join();
  void wait();
  void cancel();
  bool is_critical()
  {return is_critical_;}

  bool is_running_internal( boost::unique_lock<boost::mutex> & lock );

  /// Reset the pipeline node after a failure so that it can be restarted
  virtual bool reset();

  void thread_job();

  /// execute just this node in a while loop until failure.
  void run();

  void push_output( process::step_status status );

  process::step_status execute_incoming_edges();
  process::step_status execute_outgoing_edges();

private:
  async_pipeline_node(const async_pipeline_node&);
  bool is_running_;

  boost::mutex mut_, thread_mut_;
  boost::condition_variable cond_is_running_;
  //Because thread groups does not clear out terminated threads,
  //and the threads have different terminated conditions, we
  //are doing thread managment in the node.  Most of the time
  //we want only one thread per node.  The exception is pads,
  //but that thread is handled seperately in async_pipeline
  boost::shared_ptr<boost::thread> thread_;
  bool is_critical_;

};


} // end namespace vidtk


#endif // async_pipeline_node_h_
