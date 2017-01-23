/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pipeline_queue_monitor_h_
#define vidtk_pipeline_queue_monitor_h_

#include <pipeline_framework/pipeline.h>
#include <pipeline_framework/pipeline_node.h>
#include <pipeline_framework/pipeline_edge.h>

#include <string>

namespace vidtk
{

/// \brief A class which monitors the outgoing queue length of multiple nodes.
///
/// This class is generally useful for asynchronous pipelines, where we must
/// take some action (such as downsampling the input stream) if the queue
/// surrounding a particular edge exceeds a certain threshold.
class pipeline_queue_monitor
{
public:

  pipeline_queue_monitor() {}
  virtual ~pipeline_queue_monitor() {}

  /// \brief Configure this class to monitor a certain node, if possible.
  ///
  /// Takes in the node (process) name, and a pipeline to search for this
  /// node in. Returns false if we are unable to find the node in the given
  /// pipeline.
  virtual bool monitor_node( const std::string& node_id, const pipeline* p );

  /// \brief Configure this class to monitor a certain node, if possible.
  ///
  /// Returns false if we are unable to find the node in the given pipeline.
  virtual bool monitor_node( const pipeline_node* pnode );

  /// \brief Returns the maximum queue length of all currently monitored nodes.
  virtual unsigned current_max_queue_length() const;

  /// \brief Reset the list of nodes this class is monitoring
  virtual void reset();

private:

  std::vector< const pipeline_node* > nodes_to_monitor_;

};

} // end namespace vidtk

#endif
