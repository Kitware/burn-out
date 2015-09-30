/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef pipeline_edge_h_
#define pipeline_edge_h_

#include <vcl_string.h>
#include <boost/function.hpp>
#include <process_framework/process.h>


namespace vidtk
{


class pipeline_node;

/// Base class to represent and edge between nodes in a pipeline graph.
/// \note The base class is trivial but not abstract.  The base edge can be
/// constructed to represent certain types of dummy edges.
struct pipeline_edge
{
  pipeline_node* from_;
  pipeline_node* to_;
  vcl_string from_port_name_;
  vcl_string to_port_name_;
  bool implies_execution_dependency_;
  bool optional_connection_;

  /// Pull the data from the source node and temporarily store it in the edge.
  /// This function may be called by \c execute() or used asynchronously from
  /// \c push_data() depending on the type of pipeline.
  virtual void pull_data()
  {
  }

  /// Push the data stored on the edge to the sink node.
  /// This function may be called by \c execute() or used asynchronously from
  /// \c pull_data() depending on the type of pipeline.
  virtual process::step_status push_data()
  {
    return process::SUCCESS;
  }

  /// Transmit the data through this edge from source to sink.
  /// Pulls the data from the source node and pushes it to the sink node using
  /// the \c pull_data() and \c push_data() functions.
  /// \note this function is only used in synchronous pipelines
  virtual void execute()
  {
    pull_data();
    push_data();
  }

  virtual ~pipeline_edge()
  {
  };

  virtual bool reset()
  {
    return true;
  }

protected:

  process::step_status from_node_last_execute_state();
};


template<class OT, class IT>
struct pipeline_edge_impl
  : pipeline_edge
{
  typedef boost::function< OT() > output_func_type;

  boost::function< OT() > get_output_;
  boost::function< void(IT) > set_input_;

  protected:
  // only allow construction via derived classes
  pipeline_edge_impl() {}
};


} // end namespace vidtk


#endif // pipeline_edge_h_
