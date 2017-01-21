/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sync_pipeline.h"

#include <vbl/vbl_smart_ptr.hxx>
#include <vul/vul_timer.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER("pipeline.sync");

sync_pipeline
::sync_pipeline()
  : pipeline_impl<sync_pipeline>( 1 )
{
}


void
sync_pipeline
::add( sync_pipeline_node const& n )
{
  pipeline::add(new sync_pipeline_node( n ));
}


void
sync_pipeline
::add( node_id_t p )
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
::add_without_execute( node_id_t p )
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
  typedef std::vector<pipeline_node*>::const_iterator itr;

  vul_timer timer;

  bool all_outputs_failed = true;
  bool any_outputs_skipped = false;
  bool any_outputs_flushed = false;

  for( itr it = execution_order().begin(); it != execution_order().end(); ++it )
  {
    sync_pipeline_node* node = dynamic_cast<sync_pipeline_node*>(*it);
    assert(node);

    if( node->last_execute_state() != process::FAILURE )
    {
      bool skip_recover = false;
      process::step_status edge_status = node->execute_incoming_edges();
      if( edge_status == process::FAILURE )
      {
        node->set_last_execute_to_failed();
      }
      else if( edge_status == process::SKIP )
      {
        if (node->skipped())
        {
          skip_recover = true;
          node->execute();
        }
        else
        {
          node->set_last_execute_to_skip();
        }
      }
      else if( edge_status == process::FLUSH )
      {
        node->set_last_execute_to_flushed();
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
        if( node_status == process::FLUSH )
        {
          any_outputs_flushed = true;
        }
      }
      node->log_status(edge_status, skip_recover, node_status);
    }
  }

  double t = timer.real();
  elapsed_ms_ += t;
  ++step_count_;
  total_elapsed_ms_ += t;
  ++total_step_count_;

  if( elapsed_ms_ > 2000 )
  {
    LOG_INFO( "PIPELINE: steps/second (over last 2 seconds) = "
              << double(step_count_)*1000.0 / elapsed_ms_ << "\n"
              <<"          steps/second (overall)             = "
              << double(total_step_count_)*1000.0 / total_elapsed_ms_ << "\n"
              << "          seconds/frame (overall)            = "
              << 1 / ( double(total_step_count_)*1000.0 / total_elapsed_ms_ )
      );
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

  if( any_outputs_flushed )
  {
    return process::FLUSH;
  }

  return process::SUCCESS;
}


/// Execute the pipeline in a loop until all processes have failed.
bool
sync_pipeline
::run()
{
  while( this->execute() != process::FAILURE ) ;
  return true;
}


} // end namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::sync_pipeline );
