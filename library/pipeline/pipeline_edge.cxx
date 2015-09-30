/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "pipeline_edge.h"
#include <pipeline/pipeline_node.h>

namespace vidtk
{

process::step_status
pipeline_edge
::from_node_last_execute_state()
{
  return from_->last_execute_state();
}

} // end namespace vidtk
