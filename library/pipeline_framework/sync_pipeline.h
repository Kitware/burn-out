/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sync_pipeline_h_
#define vidtk_sync_pipeline_h_

#include <vbl/vbl_smart_ptr.h>

#include <pipeline_framework/pipeline.h>
#include <pipeline_framework/sync_pipeline_node.h>
#include <pipeline_framework/sync_pipeline_edge.h>

#include <process_framework/process.h>


namespace vidtk
{


class sync_pipeline
  : public pipeline_impl<sync_pipeline>
{
public:

  template <class From, class To>
  struct edge
  {
    typedef sync_pipeline_edge_impl<From,To> type;
  };

  sync_pipeline();

  void add( sync_pipeline_node const& n );

  virtual void add( node_id_t p );

  virtual void add( process::pointer p );

  // Add a pipeline node that will not execute, but
  // will be part of the pipeline. Useful for allowing
  // a configuration file to contain settings for
  // pipeline elements that are not always used.
  void add_without_execute( sync_pipeline_node const& n );

  // Add a process that will not execute, but
  // will be part of the pipeline. Useful for allowing
  // a configuration file to contain settings for
  // pipeline elements that are not always used.
  virtual void add_without_execute( node_id_t p );
  virtual void add_without_execute( process::pointer p );


  /// This will attempt to execute all the nodes that previously did
  /// not fail to execute. ("Execute" on a process node is calling the
  /// "step" function.)  If a node does not execute successfully, then
  /// none of its dependent nodes are executed, and those dependent
  /// nodes are considered to have failed too. This method will return
  /// true if at least one "output node" executed, and false otherwise.
  ///
  /// An output node is one that is considered to have a side-effect that
  /// not visible in the pipeline graph. By default, all sink nodes (nodes
  /// that don't have any outgoing edges) are considered output nodes.
  /// The user can override this heuristic by explicitly calling
  /// set_output_node on certain nodes.
  process::step_status execute();

  /// Execute the pipeline in a loop until all processes have failed.
  virtual bool run();

  static bool const is_async = false;
};

typedef vbl_smart_ptr<sync_pipeline> sync_pipeline_sptr;

} // end namespace vidtk


#endif // sync_pipeline_h_
