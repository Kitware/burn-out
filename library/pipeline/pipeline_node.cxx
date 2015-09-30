/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_timer.h>

#include <pipeline/pipeline_node.h>
#include <pipeline/super_process.h>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <utilities/log.h>

#include <pipeline/super_process.h>

#ifdef USE_RIGHT_TRACK
#include <right_track/BoundedEvent.h>
#endif

namespace vidtk
{

unsigned pipeline_node::unnamed_count_ = 0;

pipeline_node
::pipeline_node( process* p )
  : id_( p ),
    process_( NULL ),
    name_( p->name() ),
    type_( p->class_name() ),
    execute_( boost::bind( &vidtk::process::step2, p ) ),
    recover_( NULL ),
    params_( boost::bind( vcl_mem_fun(&vidtk::process::params), p ) ),
    set_params_( boost::bind( &vidtk::process::set_params, p, _1 ) ),
    initialize_( boost::bind( &vidtk::process::initialize, p ) ),
    last_execute_state_( process::SUCCESS ),
    is_sink_node_( false ),
    is_output_node_( UNKNOWN ),
    elapsed_ms_( 0.0 ),
    step_count_( 0 ),
    m_event( 0 )
{
  if( super_process* sp = dynamic_cast<super_process*>(p) )
  {
    recover_ = boost::bind( &vidtk::super_process::fail_recover, sp );
  }

#ifdef USE_RIGHT_TRACK
  vcl_string evn(p->class_name());
  evn += "." + p->name();
  m_event = new RightTrack::BoundedEvent (evn, "process", 0xf5e011);
#endif

}

pipeline_node
::pipeline_node( process::pointer p )
  : id_( p.ptr() ),
    process_( p ),
    name_( p->name() ),
    type_( p->class_name() ),
    execute_( boost::bind( &vidtk::process::step2, p.ptr() ) ),
    recover_( NULL ),
    params_( boost::bind( vcl_mem_fun(&vidtk::process::params), p.ptr() ) ),
    set_params_( boost::bind( &vidtk::process::set_params, p.ptr(), _1 ) ),
    initialize_( boost::bind( &vidtk::process::initialize, p.ptr() ) ),
    last_execute_state_( process::SUCCESS ),
    is_sink_node_( false ),
    is_output_node_( UNKNOWN ),
    elapsed_ms_( 0.0 ),
    step_count_( 0 ),
    m_event( 0 )
{
  if( super_process* sp = dynamic_cast<super_process*>(p.ptr()) )
  {
    recover_ = boost::bind( &vidtk::super_process::fail_recover, sp );
  }

#ifdef USE_RIGHT_TRACK
  vcl_string evn(p->class_name());
  evn += "." + p->name();
  m_event = new RightTrack::BoundedEvent (evn, "process", 0xf5e011);
#endif

}


pipeline_node
::pipeline_node()
  : id_( this ),
    process_( NULL ),
    last_execute_state_( process::SUCCESS ),
    is_sink_node_( false ),
    is_output_node_( UNKNOWN ),
    elapsed_ms_( 0.0 ),
    step_count_( 0 ),
    m_event( 0 )
{
  name_ = (boost::format("(unnamed-%1%)") % unnamed_count_).str();
  type_ = "unknown";
  ++unnamed_count_;
}


pipeline_node
::~pipeline_node()
{
  typedef vcl_vector<pipeline_edge*>::iterator itr;
  // assume incoming_edges_ owns the edges (not outgoing_edges_)
  for( itr i = incoming_edges_.begin(); i != incoming_edges_.end(); ++i )
  {
    delete *i;
  }
}


double
pipeline_node
::steps_per_second() const
{
  return double(step_count_)*1000.0 / elapsed_ms_;
}


bool
pipeline_node
::is_output_node() const
{
  return is_output_node_ == YES ||
          ( is_output_node_ == UNKNOWN && is_sink_node_ );
}


config_block
pipeline_node
::get_params() const
{
  config_block p;
  if ( params_ )
  {
    p = params_();
  }
  return p;
}


process::pointer
pipeline_node
::get_process()
{
  return process_;
}


process::step_status
pipeline_node
::execute()
{
  vul_timer t;
  ++step_count_;

  if( execute_ )
  {

#ifdef USE_RIGHT_TRACK
    if (this->m_event) this->m_event->Start();
#endif

    last_execute_state_ = execute_();

#ifdef USE_RIGHT_TRACK
    if (this->m_event) this->m_event->End();
#endif

    super_process* sp = dynamic_cast<super_process*>(this->get_process().ptr());
    if( sp )
    {
      sp->post_step();
    }
    // attempt to recover from failure
    if( last_execute_state_ == process::FAILURE &&
        this->recover_ && this->recover_() )
    {
      this->last_execute_state_ = process::SKIP;
    }
  }
  else
  {
    last_execute_state_ = process::SUCCESS;
  }

  elapsed_ms_ += t.real();

  return last_execute_state_;
}


bool
pipeline_node
::initialize()
{
  if( initialize_ ) {
    return initialize_();
  } else {
    return true;
  }
}


/// Reset the pipeline after a failure so that it can be restarted
bool
pipeline_node
::reset()
{
  typedef vcl_vector<pipeline_edge*>::iterator itr;
  for( itr it = outgoing_edges_.begin();
       it != outgoing_edges_.end(); ++it )
  {
    if( ! (*it)->reset() )
    {
      return false;
    }
  }
  last_execute_state_ = process::SUCCESS;
  return process_->reset();
}


// node takes ownership of the edge
void
pipeline_node
::add_incoming_edge( pipeline_edge* e )
{
  incoming_edges_.push_back( e );
  assert( e->to_ == this );
}


// node does not take ownership of the edge
void
pipeline_node
::add_outgoing_edge( pipeline_edge* e )
{
  outgoing_edges_.push_back( e );
  assert( e->from_ == this );
}


void
pipeline_node
::set_last_execute_to_failed()
{
  last_execute_state_=  process::FAILURE;
}


void
pipeline_node
::set_last_execute_to_skip()
{
  last_execute_state_=  process::SKIP;
}


void
pipeline_node
::set_last_execute_to_success()
{
  last_execute_state_=  process::SUCCESS;
}


process::step_status
pipeline_node
::last_execute_state() const
{
  return last_execute_state_;
}


void
pipeline_node
::append_params( config_block& all_params )
{
  if( params_ )
  {
    config_block p = params_();
    all_params.add_subblock( p, this->name() );
  }
}


bool
pipeline_node
::set_params( config_block const& all_params )
{
  if( set_params_ )
  {
    config_block p = all_params.subblock( this->name() );
    return set_params_( p );
  }
  else
  {
    // If the node does not have a set_param function, we assume it
    // configured properly.
    return true;
  }
}


/// log the execution status of this node.
/// \param edge_status is the status of the incoming edges
/// \param node_status is the status after calling \c execute()
void
pipeline_node
::log_status(process::step_status edge_status,
             process::step_status node_status)
{
  switch (edge_status)
  {
    case process::FAILURE:
      log_info( "Did not execute node " << this->name()
                << " (required nodes failed)\n" );
      break;
    case process::SKIP:
      log_info( "Skipped node " << this->name() <<" \n" );
      break;
    case process::SUCCESS:
      switch (node_status)
      {
        case process::FAILURE:
          log_info( "Failed to execute node " << this->name() << "\n" );
          break;
        case process::SKIP:
          log_info( "Executed node " << this->name() <<" - produced SKIP\n" );
          break;
        case process::SUCCESS:
          log_info( "Executed " << this->name() << "\n" );
          break;
      }
      break;
  }
}

void
pipeline_node
::output_detailed_report( vcl_string const& indent )
{
  log_info( indent << name() << ": steps/second = " << steps_per_second() << vcl_endl );
  super_process * s = dynamic_cast<
    super_process * > ( process_.as_pointer() );
  if( s ) s->get_pipeline()->output_detailed_report( indent + "    " );
}

void
pipeline_node
::set_print_detailed_report( bool print )
{
  super_process * s = dynamic_cast<
    super_process * > ( process_.as_pointer() );
  if( s )
    s->set_print_detailed_report( print );
}

} // end namespace vidtk
