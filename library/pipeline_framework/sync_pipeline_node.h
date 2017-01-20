/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sync_pipeline_node_h_
#define vidtk_sync_pipeline_node_h_


#include <pipeline_framework/pipeline_node.h>
#include <process_framework/process.h>

namespace vidtk
{

class sync_pipeline;

class sync_pipeline_node
  : public pipeline_node
{
public:

  sync_pipeline_node( node_id_t p );

  sync_pipeline_node( process::pointer p );

  ~sync_pipeline_node();

protected:
  friend class sync_pipeline;

  process::step_status execute_incoming_edges();

};


} // end namespace vidtk


#endif // sync_pipeline_node_h_
