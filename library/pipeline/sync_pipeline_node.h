/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef sync_pipeline_node_h_
#define sync_pipeline_node_h_


#include <pipeline/pipeline_node.h>
#include <process_framework/process.h>

namespace vidtk
{

class sync_pipeline;

class sync_pipeline_node
  : public pipeline_node
{
public:

  sync_pipeline_node();

  sync_pipeline_node( process* p );

  sync_pipeline_node( process::pointer p );


protected:
  friend class sync_pipeline;

  process::step_status execute_incoming_edges();

};


} // end namespace vidtk


#endif // sync_pipeline_node_h_
