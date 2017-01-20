/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pipeline_queue_monitor.h"

#include <process_framework/process.h>

#include <pipeline_framework/super_process.h>

#include <map>

namespace vidtk
{

bool
pipeline_queue_monitor
::monitor_node( const std::string& node_id, const pipeline* p )
{
  if( !p )
  {
    return false;
  }

  std::map< std::string, pipeline_node* > pnodes = p->enumerate_nodes();

  typedef std::map< std::string, pipeline_node* >::iterator itr;

  for( itr i = pnodes.begin(); i != pnodes.end(); i++ )
  {
    if( i->first == node_id )
    {
      nodes_to_monitor_.push_back( i->second );
      return true;
    }

    process::pointer proc_ptr = i->second->get_process();

    super_process* super_proc_ptr = dynamic_cast< super_process* >( proc_ptr.ptr() );

    if( super_proc_ptr )
    {
      const pipeline* super_proc_pipe = super_proc_ptr->get_pipeline().ptr();

      if( this->monitor_node( node_id, super_proc_pipe ) )
      {
        return true;
      }
    }
  }

  return false;
}

bool
pipeline_queue_monitor
::monitor_node( const pipeline_node* pnode )
{
  if( !pnode )
  {
    return false;
  }

  nodes_to_monitor_.push_back( pnode );
  return true;
}

void
pipeline_queue_monitor
::reset()
{
  nodes_to_monitor_.clear();
}

unsigned
pipeline_queue_monitor
::current_max_queue_length() const
{
  unsigned max = 0;

  for( unsigned i = 0; i < nodes_to_monitor_.size(); i++ )
  {
    const unsigned mqlen = nodes_to_monitor_[i]->max_outgoing_queue_length();

    if( mqlen > max )
    {
      max = mqlen;
    }
  }

  return max;
}

} // end namespace vidtk
