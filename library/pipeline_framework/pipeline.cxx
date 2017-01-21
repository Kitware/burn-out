/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pipeline.h"

#include <vbl/vbl_smart_ptr.hxx>
#include <vul/vul_timer.h>
#include <pipeline_framework/super_process.h>

#include <fstream>
#include <set>
#include <deque>
#include <algorithm>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER("pipeline_cxx");

pipeline
::pipeline( unsigned edge_capacity )
  : elapsed_ms_( 0.0 ),
    step_count_( 0 ),
    total_elapsed_ms_( 0.0 ),
    total_step_count_( 0 ),
    print_detailed_report_( true ),
    edge_capacity_( edge_capacity ),
    do_nothing_execute_()
{
}


pipeline
::~pipeline()
{
  typedef std::map<node_id_t,pipeline_node*>::iterator node_itr;

  for( node_itr nit = nodes_.begin();
       nit != nodes_.end(); ++nit ) {
    delete nit->second;
  }
}


void
pipeline
::add( pipeline_node * n )
{
  if( n )
  {
    LOG_ASSERT( nodes_.find( n->id() ) == nodes_.end(),
      "Rejecting duplicate node ("<< n->name() <<") in the pipeline." );

    nodes_[ n->id() ] = n;
    insert_order_.push_back( n );
    n->is_executable_ = true;
    reorder();
  }
}


void
pipeline
::add_without_execute( pipeline_node * n )
{
  if( n )
  {
    nodes_[ n->id() ] = n;
    // Override the execute function with one that does nothing...
    n->set_execute_func( do_nothing_execute_ );
    // Since this has no execute function, it can run forever, and so
    // it cannot be an output node. (If it were, the pipeline would never
    // stop executing.)
    n->is_output_node_ = pipeline_node::NO;
    n->is_executable_ = false;

    insert_order_.push_back( n );
    reorder();
  }
}


void
pipeline
::add_execution_dependency( process::pointer source_proc,
                            process::pointer sink_proc )
{
  add_execution_dependency( source_proc.ptr(), sink_proc.ptr() );
}

void
pipeline
::add_execution_dependency( node_id_t source_id, node_id_t sink_id )
{
  pipeline_node* source_node = get_node_with_id( source_id );
  pipeline_node* sink_node = get_node_with_id( sink_id );

  pipeline_edge* e = new pipeline_edge;
  e->from_ = source_node;
  e->to_ = sink_node;
  e->implies_execution_dependency_ = true;
  e->optional_connection_ = false;

  sink_node->add_incoming_edge( e, "" );
  source_node->add_outgoing_edge( e );

  reorder();
}


void
pipeline
::add_optional_execution_dependency( node_id_t source_id, node_id_t sink_id )
{
  pipeline_node* source_node = get_node_with_id( source_id );
  pipeline_node* sink_node = get_node_with_id( sink_id );

  pipeline_edge* e = new pipeline_edge;
  e->from_ = source_node;
  e->to_ = sink_node;
  e->implies_execution_dependency_ = true;
  e->optional_connection_ = true;

  sink_node->add_incoming_edge( e, "" );
  source_node->add_outgoing_edge( e );

  reorder();
}


/// If anything is running asynchronously, shut down the threads cleanly
/// In a fully synchronous pipeline, this function does nothing
bool
pipeline
::cancel()
{
  return true;
}


void
pipeline
::do_connect( node_id_t source_id, node_id_t sink_id, pipeline_edge* edge, std::string const& input_name)
{
  pipeline_node* source_node = get_node_with_id( source_id );
  pipeline_node* sink_node = get_node_with_id( sink_id );

  if( !(source_node->is_executable_ && sink_node->is_executable_) )
  {
    LOG_AND_DIE("Should only connect executable nodes otherwise we will have strange bugs.  These processes are not going to be executed: "
                << ((source_node->is_executable_)? source_node->name():"") << " "
                << ((sink_node->is_executable_)?   sink_node->name():  ""));
  }

  edge->from_ = source_node;
  edge->to_ = sink_node;

  sink_node->add_incoming_edge( edge, input_name );
  source_node->add_outgoing_edge( edge );

  reorder();
}


bool
pipeline
::initialize()
{
  typedef std::vector<pipeline_node*>::iterator itr;

  for( itr it = execution_order_.begin(); it != execution_order_.end(); ++it ) {
    if( ! (*it)->initialize() ) {
      LOG_WARN( "Failed to initialize node " << (*it)->name() );
      return false;
    }
  }

  return true;
}


/// Reset the pipeline after a failure so that it can be restarted
bool
pipeline
::reset()
{
  typedef std::vector<pipeline_node*>::iterator itr;

  for( itr it = execution_order_.begin(); it != execution_order_.end(); ++it ) {
    if( ! (*it)->reset() ) {
      LOG_WARN( "Failed to reset node " << (*it)->name() );
      return false;
    }
  }

  return true;
}


bool
pipeline
::reset_node( node_id_t id )
{
  pipeline_node* node = this->get_node_with_id( id );
  if( node == NULL )
  {
    return false;
  }
  else
  {
    return node->reset();
  }
}


/// Reset only this node and the nodes in the pipeline that are downstream..
/// These are the nodes that should have fail as a result of \a id failing.
bool
pipeline
::reset_downstream(node_id_t id)
{
  // find all downstream nodes
  std::vector< pipeline_node* > downstream;
  downstream = downstream_nodes( get_node_with_id( id ) );

  // reset all the downstream nodes
  typedef std::vector<pipeline_node*>::iterator node_itr;
  for( node_itr it = downstream.begin(); it != downstream.end(); ++it )
  {
    if( ! (*it)->reset() )
    {
      LOG_WARN( "Failed to reset node " << (*it)->name() );
      return false;
    }
  }

  return true;
}

bool
pipeline
::set_params_downstream( config_block const& all_params, node_id_t id)
{
  // find all downstream nodes
  std::vector< pipeline_node* > downstream;
  downstream = downstream_nodes( get_node_with_id( id ) );

  // Try to set the parameters for each of the nodes.  If any one of
  // them fail, we fail as a whole.  (But even if the first node
  // fails, we will still try to set the parameters for the
  // subsequent nodes.)
  bool okay = true;
  typedef std::vector<pipeline_node*>::iterator node_itr;
  for( node_itr it = downstream.begin(); it != downstream.end(); ++it )
  {
    LOG_INFO( "Set parameters for " << (*it)->name());
    if( ! (*it)->set_params( all_params ) )
    {
      LOG_ERROR( "Set parameters failed for " << (*it)->name() );
      okay = false;
    }
  }

  return okay;
}


config_block
pipeline
::params() const
{
  typedef std::vector<pipeline_node*>::const_iterator itr;

  config_block all_params;

  for( itr it = execution_order_.begin(); it != execution_order_.end(); ++it ) {
    (*it)->append_params( all_params );
  }

  return all_params;
}


bool
pipeline
::set_params( config_block const& all_params )
{
  typedef std::vector<pipeline_node*>::iterator itr;

  // Try to set the parameters for each of the nodes.  If any one of
  // them fail, we fail as a whole.  (But even if the first node
  // fails, we will still try to set the parameters for the
  // subsequent nodes.)
  bool okay = true;
  for( itr it = execution_order_.begin(); it != execution_order_.end(); ++it )
  {
    LOG_INFO( "Set parameters for " << (*it)->name());
    if( ! (*it)->set_params( all_params ) )
    {
      LOG_ERROR( "Set parameters failed for " << (*it)->name() );
      okay = false;
    }
  }

  return okay;
}


void
pipeline::
set_output_node( node_id_t node_id, bool value )
{
  pipeline_node* n = get_node_with_id( node_id );
  n->is_output_node_ = ( value ? pipeline_node::YES: pipeline_node::NO );
}


void
pipeline
::set_edge_capacity( unsigned edge_capacity )
{
  edge_capacity_ = edge_capacity;
}


void
pipeline
::output_graphviz_file( std::string const& filename ) const
{
  std::ofstream fstr( filename.c_str() );

  if( ! fstr )
  {
    LOG_ERROR( "Couldn't open " << filename << " for writing" );
    return;
  }

  typedef std::vector<pipeline_node*>::const_iterator node_itr;

  // Figure out the ports at each node by querying the information in
  // the edges.

  std::map< std::string, std::set< std::string > > inp_ports;
  std::map< std::string, std::set< std::string > > out_ports;
  for( node_itr nit = insert_order_.begin();
       nit != insert_order_.end(); ++nit )
  {
    pipeline_node* node = *nit;
    typedef std::vector<pipeline_edge*>::iterator edge_itr;
    for( edge_itr eit = node->incoming_edges_.begin();
         eit != node->incoming_edges_.end(); ++eit )
    {
      pipeline_edge* e = *eit;

      std::string from_port;
      if( ! e->from_port_name_.empty() )
      {
        from_port = e->from_port_name_;
      }
      else
      {
        from_port = "(direct)";
      }
      out_ports[e->from_->name()].insert( from_port );

      std::string to_port;

      // Encode whether the port is optional in the string name.  This
      // shouldn't cause a conflict because there should only be one
      // connection to each input port.  If there is more than one,
      // there is a error in the pipeline data flow.
      to_port = ( e->optional_connection_ ? 'O' : 'R' );

      if( ! e->to_port_name_.empty() )
      {
        to_port += e->to_port_name_;
      }
      else
      {
        to_port += "(direct)";
      }

      inp_ports[e->to_->name()].insert( to_port );
    }
  }


  // Find the execution order number for each node
  std::map< pipeline_node*, unsigned > execution_order_number;
  for( unsigned i = 0; i < execution_order_.size(); ++i )
  {
    execution_order_number[ execution_order_[i] ] = i;
  }


  fstr << "digraph G {\n"
       << "concentrate=true;\n";

  this->output_graphviz_legend( fstr );

  // Output all the nodes.  We'll output the connections between nodes
  // later.
  std::string port_node_style = "shape=none,height=0,width=0,fontsize=7";
  std::string port_edge_style = "color=black";
  for( node_itr nit = insert_order_.begin();
       nit != insert_order_.end(); ++nit )
  {
    pipeline_node* node = *nit;
    if( dynamic_cast<super_process_pad*>(node->get_process().ptr()) )
    {
      if( node->is_output_node() )
      {
        fstr << "\"set_value|"<<node->name()<<"\" "
             << "[label=\"(" << execution_order_number[node] << ")\\n"
             << node->name() << "\",shape=house]\n";
      }
      else
      {
        fstr << "\"value|"<<node->name()<<"\" "
             << "[label=\"" << node->name()
             << "\\n(" << execution_order_number[node] << ")\","
             << "shape=invhouse]\n";
      }
    }
    else
    {
      fstr << "subgraph cluster_" << node->name() << " {\n";
      fstr << "  color=lightgrey; style=filled\n";
      fstr << "  \"" << node->name() << "\" [label = \""
          << node->name() << "\\n[" << node->type() << "]\\n"
          << "(" << execution_order_number[node] << ")\"];\n";

      std::set< std::string > const& inp_set = inp_ports[ node->name() ];
      std::set< std::string > const& out_set = out_ports[ node->name() ];

      typedef std::set< std::string >::const_iterator set_iter_type;

      for( set_iter_type it = inp_set.begin(); it != inp_set.end(); ++it )
      {
        char optional = ( (*it)[0] == 'O' );
        std::string port_name = it->substr(1) + "|" + node->name();
        fstr << "  \"" << port_name << "\" [constraint=false,label=\""
            << it->substr(1) << "\"," << port_node_style << "];\n";
        fstr << "  \"" << port_name << "\" -> \""
            << node->name() << "\" [" << port_edge_style
            << (optional?",style=dotted":"") << "];\n";
      }

      for( set_iter_type it = out_set.begin(); it != out_set.end(); ++it )
      {
        std::string port_name = *it + "|" + node->name();
        fstr << "  \"" << port_name << "\" [label=\""
            << *it << "\"," << port_node_style << "];\n";
        fstr << "  \"" << node->name() << "\" -> \""
            << port_name << "\" [arrowhead=none," << port_edge_style << "];\n";
      }
      fstr << "}\n"; // end cluster
    }
  }


  for( node_itr nit = insert_order_.begin();
       nit != insert_order_.end(); ++nit )
  {
    pipeline_node* node = *nit;
    typedef std::vector<pipeline_edge*>::iterator edge_itr;
    for( edge_itr eit = node->incoming_edges_.begin();
         eit != node->incoming_edges_.end(); ++eit )
    {
      pipeline_edge* e = *eit;

      unsigned from_time = execution_order_number[e->from_];
      unsigned to_time = execution_order_number[e->to_];

      std::string from_port_node;
      if( ! e->from_port_name_.empty() )
      {
        from_port_node = e->from_port_name_ + "|" + e->from_->name();
      }
      else
      {
        from_port_node = e->from_->name();
      }

      std::string to_port_node;
      if( ! e->to_port_name_.empty() )
      {
        to_port_node = e->to_port_name_ + "|" + e->to_->name();
      }
      else
      {
        to_port_node = e->to_->name();
      }

      fstr << "\"" << from_port_node << "\" -> \""
           << to_port_node << "\" [minlen=1";
      if( e->optional_connection_ )
      {
        fstr << ",color=grey50";
      }
      else
      {
        fstr << ",color=black";
      }
      fstr <<",samehead=\""
           << to_port_node << "\", sametail=\""
           << from_port_node << "\"";
      if( ! e->implies_execution_dependency_ )
      {
        fstr << ",style=dashed,arrowhead=none,constraint=false";
      }
      else if( ! e->to_port_name_.empty() )
      {
        // if we are connecting to a named port, don't draw an arrow
        // head, because there will be one from the port to the node.
        fstr << ",arrowhead=none";
      }

      // Weight the edges by the execution time between the nodes.
      // This will make the graph layout basically follow the
      // execution order, while still attempting to show the data
      // flow.
      if( from_time > to_time )
      {
        fstr << ",constraint=false";
      }
      else
      {
        fstr << ",weight=" << execution_order_.size()+1-to_time+from_time;
      }
      fstr << "];\n";
    }
  }


#if 0
  // The green arrows indicating execution order can be awfully
  // confusing for larger pipelines.
  if( execution_order_.size() > 1 )
  {
    node_itr prev = execution_order_.begin();
    node_itr next = prev;
    for( ++next; next != execution_order_.end(); ++prev, ++next )
    {
      fstr << "\"" << (*prev)->name() << "\" -> \""
           << (*next)->name() << "\" [color=green, constraint=false];\n";
    }
  }
#endif
  fstr << "}\n";

  //Implement super process stuff.
  for( node_itr nit = insert_order_.begin();
       nit != insert_order_.end(); ++nit )
  {
    pipeline_node* node = *nit;

    super_process * s = dynamic_cast<
      super_process * > ( node->get_process().as_pointer() );
    if( !s ) continue;

    // Run graphviz on this node.
    std::string new_file = filename.substr( 0,( filename.size( )-4 ) )
      + "_" + node->name() + ".dot";
    s->get_pipeline()->output_graphviz_file( new_file );
  }

}


void
pipeline
::output_graphviz_legend( std::ostream& fstr ) const
{
  fstr <<
    "subgraph clusterLegend {\n"
    "  \"process node\" [shape=\"plaintext\"]\n"
    "  subgraph clusterLegendNode {\n"
    "    color=lightgrey; style=filled\n"
    "    LegendNode__ [label = \"name\\n[class name]\\n(execution order)\"];\n"
    "  }\n"
    "  f1 [label=\"function\",shape=none,height=0,width=0,fontsize=7]\n"
    "  sf1 [label=\"set_function\",shape=none,height=0,width=0,fontsize=7]\n"
    "  s1 [label=\"\"]\n"
    "  d1 [label=\"\"]\n"
    "\n"
    "  f2 [label=\"function\",shape=none,height=0,width=0,fontsize=7]\n"
    "  sf2 [label=\"set_function\",shape=none,height=0,width=0,fontsize=7]\n"
    "  s2 [label=\"\"]\n"
    "  d2 [label=\"\"]\n"
    "\n"
    "  f3 [label=\"function\",shape=none,height=0,width=0,fontsize=7]\n"
    "  sf3 [label=\"set_function\",shape=none,height=0,width=0,fontsize=7]\n"
    "  s3 [label=\"\"]\n"
    "  d3 [label=\"\"]\n"
    "\n"
    "  \"super process input pad\" [shape=\"plaintext\"]\n"
    "  subgraph clusterLegendInPad {\n"
    "    color=none;\n"
    "    inpad [label=\"pad name\\n(execution order)\",shape=invhouse]\n"
    "  }\n"
    "\n"
    "  \"super process output pad\" [shape=\"plaintext\"]\n"
    "  subgraph clusterLegendOutPad {\n"
    "    color=none;\n"
    "    outpad [label=\"(execution order)\\npad name\",shape=house]\n"
    "  }\n"
    "\n"
    "  \"required data flow\" [shape=\"plaintext\"]\n"
    "  subgraph clusterLegendData {\n"
    "    color=none;\n"
    "    s1->f1 [color=black,arrowhead=none];\n"
    "    f1->sf1 [color=black,arrowhead=none];\n"
    "    sf1->d1 [color=black];\n"
    "    { rank=same; s1; f1; sf1; d1; }\n"
    "  }\n"
    "\n"
    "  \"optional data flow\" [shape=\"plaintext\"]\n"
    "  subgraph clusterLegendOptionalData {\n"
    "    color=none;\n"
    "    s2->f2 [color=black,arrowhead=none];\n"
    "    f2->sf2 [color=grey50,arrowhead=none];\n"
    "    sf2->d2 [color=black,style=dotted];\n"
    "    { rank=same; s2; f2; sf2; d2; }\n"
    "  }\n"
    "\n"
    "  \"flow backward in time\" [shape=\"plaintext\"]\n"
    "  subgraph clusterLegendBackwardData {\n"
    "    color=none;\n"
    "    s3->f3 [color=black,arrowhead=none];\n"
    "    f3->sf3 [color=black,style=dashed,arrowhead=none];\n"
    "    sf3->d3 [color=black];\n"
    "    { rank=same; s3; f3; sf3; d3; }\n"
    "  }\n"
    "\n"
    "  subgraph clusterLegendLabels {\n"
    "    color=none;\n"
    "    \"process node\";\n"
    "    \"super process input pad\";\n"
    "    \"super process output pad\";\n"
    "    \"required data flow\";\n"
    "    \"optional data flow\";\n"
    "    \"flow backward in time\";\n"
    "  }\n"
    "\n"
    "  \"process node\"->\"super process input pad\"->\"super process output pad\"->\"required data flow\"->\"optional data flow\"->\"flow backward in time\" [style=invis];\n"
    "  LegendNode__ -> inpad -> outpad -> s1 -> s2 -> s3 [style=invis];\n"
    "  label=\"Legend\";\n"
    "}\n";
}


void
pipeline
::reorder()
{
  typedef std::vector<pipeline_node*>::const_iterator node_itr;

  execution_order_.clear();
  visited_.clear();

  for( node_itr it = insert_order_.begin(); it != insert_order_.end(); ++it )
  {
    if( visited_.find( (*it)->id() ) == visited_.end() )
    {
      color_.clear();
      visit_depth_first( *it );
    }
  }

  // We've added a node or a connection, so (re)determine which nodes
  // are sink nodes.
  for( node_itr it = insert_order_.begin(); it != insert_order_.end(); ++it )
  {
    (*it)->is_sink_node_ = true;
  }
  for( node_itr nit = insert_order_.begin();
       nit != insert_order_.end(); ++nit )
  {
    pipeline_node* node = *nit;
    typedef std::vector<pipeline_edge*>::iterator edge_itr;
    for( edge_itr eit = node->incoming_edges_.begin();
         eit != node->incoming_edges_.end(); ++eit )
    {
      pipeline_edge* e = *eit;
      e->from_->is_sink_node_ = false;
    }
  }

#ifndef NDEBUG
  // make sure every node was visited
  typedef std::map<node_id_t, pipeline_node*>::iterator itr;
  for( itr it = nodes_.begin(); it != nodes_.end(); ++it )
  {
    assert( visited_.find( it->second->id() ) != visited_.end() );
  }
#endif
}


void
pipeline
::visit_depth_first( pipeline_node* node )
{
  // Rely on the default constructor for int() to create color with
  // value 0 for unvisited nodes.
  if( color_[ node->id() ] == 1 )
  {
    // We are re-visiting a node that hasn't been fullly explored,
    // which implies a cycle in the execution dependencies.
    output_graphviz_file( "debug_pipeline_cycle.dot" );
    LOG_AND_DIE( "Cycle detected in execution dependencies involving node \""
                 << node->name()
                 << "\". Graph has been output to debug_pipeline_cycle.dot" );
  }

  if( visited_.find( node->id() ) == visited_.end() )
  {
    typedef std::vector<pipeline_edge*>::iterator itr;
    color_[ node->id() ] = 1; // in process
    for( itr it = node->incoming_edges_.begin();
         it != node->incoming_edges_.end(); ++it )
    {
      if( (*it)->implies_execution_dependency_ )
      {
        visit_depth_first( (*it)->from_ );
      }
    }
    color_[ node->id() ] = 2; // completed process
    visited_[ node->id() ] = true;
    execution_order_.push_back( node );
  }
}


std::vector< pipeline_node* >
pipeline
::downstream_nodes( pipeline_node* n )
{
  // find all downstream nodes
  std::vector< pipeline_node* > downstream;
  std::deque< pipeline_node* > to_explore;
  to_explore.push_back( n );
  while( !to_explore.empty() )
  {
    n = to_explore.front();
    to_explore.pop_front();
    if( std::find(downstream.begin(), downstream.end(), n) == downstream.end() )
    {
      downstream.push_back( n );
      typedef std::vector<pipeline_edge*>::iterator edge_itr;
      for( edge_itr eit = n->outgoing_edges_.begin();
           eit != n->outgoing_edges_.end(); ++eit )
      {
        to_explore.push_back( (*eit)->to_ );
      }
    }
  }
  return downstream;
}

pipeline_node*
pipeline
::get_node_with_id( node_id_t id ) const
{
  std::map<node_id_t, pipeline_node*>::const_iterator it = nodes_.find( id );
  if( it == nodes_.end() ) {
    LOG_ERROR( "Error finding pointer: " << id );
    throw std::runtime_error( "couldn't find that node" );
  } else {
    return it->second;
  }
}


void
pipeline
::output_detailed_report( std::string const& indent )
{
  typedef std::vector<pipeline_node*>::iterator itr;

  LOG_INFO( indent << "PIPELINE timing (approximate)\n"
         << indent << "   Overall steps/second = "
         << double(step_count_)*1000.0 / elapsed_ms_ );
  for( itr it = execution_order_.begin(); it != execution_order_.end(); ++it )
  {
    (*it)->output_detailed_report( indent + "    " );
  }
}


// output timing measurements in a DartMeasurement XML format for use in CDash.
void
pipeline
::output_timing_measurements() const
{
  double sps = double(total_step_count_) * 1000.0 / total_elapsed_ms_;
  LOG_INFO( "<DartMeasurement name=\"complete_pipeline\""
           << " type=\"numeric/double\">" << sps
           << "</DartMeasurement>" );
  std::map<std::string, double> timing_map = this->collect_node_timing();
  typedef std::map<std::string, double>::const_iterator itr;
  for( itr it = timing_map.begin(); it != timing_map.end(); ++it )
  {
    LOG_INFO( "<DartMeasurement name=\""<< it->first <<"\""
             << " type=\"numeric/double\">" << it->second
             << "</DartMeasurement>" );
  }
}


// ----------------------------------------------------------------
/** Output process timings.
 *
 * This method generates a table of processing timing entries.
 *
 * @param[in] str - stream to write onto
 */
void
pipeline
::output_timing_measurements(std::ostream& str) const
{
  double sps = double(total_step_count_) * 1000.0 / total_elapsed_ms_;

  str << "Timing table begin" << std::endl
      << "   total step count: " << total_step_count_ << std::endl
      << "   total elapsed time (sec): " << (total_elapsed_ms_ / 1000.0) << std::endl
      << "   complete pipeline rate: " << sps << " steps per sec.\n";

  std::map<std::string, double> timing_map = this->collect_node_timing();

  typedef std::map<std::string, double>::const_iterator itr;

  for( itr it = timing_map.begin(); it != timing_map.end(); ++it )
  {
    str << "\"" << it->first << "\"  "
        << it->second
        << "  steps per second"
        << std::endl;
  }
}

void
pipeline
::set_print_detailed_report( bool print )
{
  print_detailed_report_ = print;

  // Propagate the flag to the underlying super process nodes too.
  typedef std::vector<pipeline_node*>::iterator itr;
  for( itr it = execution_order_.begin(); it != execution_order_.end(); ++it )
  {
    (*it)->set_print_detailed_report( print );
  }
}


std::map<std::string, double>
pipeline
::collect_node_timing() const
{
  std::map<std::string, double> timing_map;
  typedef std::vector<pipeline_node*>::const_iterator itr;
  for( itr it = execution_order_.begin(); it != execution_order_.end(); ++it )
  {
    timing_map[(*it)->name()] = (*it)->steps_per_second();
    process* p = (*it)->get_process().as_pointer();

    // check for superprocess
    if (super_process * s = dynamic_cast<super_process*>(p))
    {
      // Collect timing from that pipeline
      std::map<std::string, double> tm = s->get_pipeline()->collect_node_timing();

      // Combine sub-pipeline timing info. Add prefix to name to
      // indicate superprocess.
      typedef std::map<std::string, double>::const_iterator mitr;
      std::string prefix = (*it)->name() + ':';
      for( mitr mit = tm.begin(); mit != tm.end(); ++mit )
      {
        timing_map[prefix + mit->first] = mit->second;
      }
    }
  }
  return timing_map;
}


std::map< std::string, pipeline_node* >
pipeline
::enumerate_nodes() const
{
  typedef std::map<node_id_t,pipeline_node*>::const_iterator node_itr;
  std::map< std::string, pipeline_node* > ret;

  for( node_itr nit = nodes_.begin();
       nit != nodes_.end();
       ++nit )
  {
    ret[ nit->second->name() ] = nit->second;
  }
  return ret;
}

} // end namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::pipeline );
