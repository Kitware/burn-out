/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */



#include <pipeline/sync_pipeline_node.h>


namespace vidtk
{

sync_pipeline_node
::sync_pipeline_node()
{
}


sync_pipeline_node
::sync_pipeline_node( process* p )
  : pipeline_node( p )
{
}


sync_pipeline_node
::sync_pipeline_node( process::pointer p )
  : pipeline_node( p )
{
}


process::step_status
sync_pipeline_node
::execute_incoming_edges()
{
  bool found_skip = false;
  typedef vcl_vector<pipeline_edge*>::iterator itr;
  for( itr it = incoming_edges_.begin();
        it != incoming_edges_.end(); ++it )
  {
    // If the source node executed, we transfer data.  If the source
    // node did not execute, and we need it, then we fail.
    //
    sync_pipeline_node* from = dynamic_cast<sync_pipeline_node*>((*it)->from_);
    assert(from);
#ifdef VIDTK_SKIP_IS_PROPOGATED_OVER_OPTIONAL_CONNECTIONS
    bool skip_optional = true;
#else
    bool skip_optional = !(*it)->optional_connection_;
#endif
    if( from->last_execute_state() == process::SUCCESS )
    {
      (*it)->execute();
    }
    else if( skip_optional &&
             from->last_execute_state() == process::SKIP )
    {
      // don't return SKIP here because another input may have failed
      found_skip = true;
    }
    else if( !(*it)->optional_connection_ &&
             from->last_execute_state() == process::FAILURE )
    {
      return process::FAILURE;
    }
  }

  if( found_skip )
  {
    return process::SKIP;
  }

  // If we get to here, we executed all the edges, and therefore set
  // all the required inputs.
  //
  return process::SUCCESS;
}


} // end namespace vidtk
