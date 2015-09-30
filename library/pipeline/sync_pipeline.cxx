/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sync_pipeline.h"

#include <vbl/vbl_smart_ptr.txx>
#include <vul/vul_timer.h>

#include <utilities/log.h>

namespace vidtk
{


void
sync_pipeline
::add( sync_pipeline_node const& n )
{
  pipeline::add(new sync_pipeline_node( n ));
}


void
sync_pipeline
::add( process* p )
{
  pipeline::add( new sync_pipeline_node( p ) );
}

void
sync_pipeline
::add( process::pointer p )
{
  pipeline::add( new sync_pipeline_node( p ) );
}

void
sync_pipeline
::add_without_execute( sync_pipeline_node const& n )
{
  pipeline::add_without_execute( new sync_pipeline_node( n ) );
}


void
sync_pipeline
::add_without_execute( process* p )
{
  pipeline::add_without_execute( new sync_pipeline_node( p ) );
}

void
sync_pipeline
::add_without_execute( process::pointer p )
{
  pipeline::add_without_execute( new sync_pipeline_node( p ) );
}


process::step_status
sync_pipeline
::execute()
{
  typedef vcl_vector<pipeline_node*>::const_iterator itr;

  vul_timer timer;

  bool all_outputs_failed = true;
  bool any_outputs_skipped = false;
  for( itr it = execution_order().begin(); it != execution_order().end(); ++it ) {
    sync_pipeline_node* node = dynamic_cast<sync_pipeline_node*>(*it);
    assert(node);
    if( node->last_execute_state() != process::FAILURE )
    {
      process::step_status edge_status = node->execute_incoming_edges();
      if( edge_status == process::FAILURE )
      {
        node->set_last_execute_to_failed();
      }
      else if( edge_status == process::SKIP )
      {
        node->set_last_execute_to_skip();
      }
      else
      {
        node->execute();
      }
      process::step_status node_status = node->last_execute_state();
      if( node->is_output_node() )
      {
        if( node_status != process::FAILURE )
        {
          all_outputs_failed = false;
        }
        if( node_status == process::SKIP )
        {
          any_outputs_skipped = true;
        }
      }
      node->log_status(edge_status, node_status);
    }
  }

  double t = timer.real();
  elapsed_ms_ += t;
  ++step_count_;
  total_elapsed_ms_ += t;
  ++total_step_count_;

  if( elapsed_ms_ > 2000 )
  {
    vcl_cout << "PIPELINE: steps/second (over last 2 seconds) = "
             << double(step_count_)*1000.0 / elapsed_ms_ << "\n";
    vcl_cout << "          steps/second (overall)             = "
             << double(total_step_count_)*1000.0 / total_elapsed_ms_
             << vcl_endl;
    vcl_cout << "          seconds/frame (overall)            = "
             << 1 / ( double(total_step_count_)*1000.0 / total_elapsed_ms_ )
             << vcl_endl;
    elapsed_ms_ = 0;
    step_count_ = 0;
  }
  if( this->print_detailed_report_ && all_outputs_failed )
  {
    output_detailed_report();
  }

  if( all_outputs_failed )
  {
    return process::FAILURE;
  }

  if( any_outputs_skipped )
  {
    return process::SKIP;
  }

  return process::SUCCESS;
}


/// Execute the pipeline in a loop until all processes have failed.
bool
sync_pipeline
::run()
{
  while( this->execute() != process::FAILURE );
  return true;
}


} // end namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::sync_pipeline );
