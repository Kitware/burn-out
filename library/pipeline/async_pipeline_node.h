/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef async_pipeline_node_h_
#define async_pipeline_node_h_


#include <pipeline/pipeline_node.h>
#include <process_framework/process.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace vidtk
{

class async_pipeline;
class super_process;

class async_pipeline_node
  : public pipeline_node
{
public:

  async_pipeline_node();

  async_pipeline_node( process* p );

  async_pipeline_node( process::pointer p );

  /// Return true if the process is still running
  bool is_running();

protected:
  friend class async_pipeline;
  friend class super_process;

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

  boost::mutex mut_;
  boost::condition_variable cond_is_running_;

};


} // end namespace vidtk


#endif // async_pipeline_node_h_
