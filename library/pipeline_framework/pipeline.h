/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pipeline_h_
#define vidtk_pipeline_h_

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <pipeline_framework/pipeline_node.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <map>
#include <vector>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_pipeline_h__
VIDTK_LOGGER("pipeline_h");


template<class T>
struct cannot_convert
{
  cannot_convert( T )
  {
  }
};

namespace vidtk
{


// ----------------------------------------------------------------
/** Abstract base class for pipelines.
 *
 * A pipeline a directed acyclic graph (DAG) of processes that are
 * executed based on their connections. The pipeline provides the
 * overall structure and control to manage a collection of processes,
 * move data from one process to another, and handle process failures.
 */
class pipeline
  : public vbl_ref_count
{
public:
  pipeline( unsigned edge_capacity );
  virtual ~pipeline();

  virtual void add( node_id_t p ) = 0;
  virtual void add( process::pointer p ) = 0;

  ///@{
  /** Add a process that will not execute. The process will be part of
   * the pipeline and participate in configuration operations. Useful
   * for allowing a configuration file to contain settings for
   * pipeline elements that are not always used.
   */
  virtual void add_without_execute( node_id_t p ) = 0;
  virtual void add_without_execute( process::pointer p ) = 0;
  ///@}

  void add_execution_dependency( node_id_t source_id, node_id_t sink_id );
  void add_execution_dependency( process::pointer, process::pointer );
  void add_optional_execution_dependency( node_id_t source_id, node_id_t sink_id );


  /// This will attempt to execute all the nodes that previously did
  /// not fail to execute. ("Execute" on a process node is calling the
  /// "step" function.)  If a node does not execute successfully, then
  /// none of its dependent nodes are executed, and those dependent
  /// nodes are considered to have failed too. This method will return
  /// true if at least one "output node" executed, and false otherwise.
  ///
  /// An output node is one that is considered to have a side-effect that
  /// not visible in the pipeline graph. By default, all sink nodes (nodes
  /// that don't have any outgoing edges) are considered output nodes.
  /// The user can override this heuristic by explicitly calling
  /// set_output_node on certain nodes.
  virtual process::step_status execute() = 0;

  /// Execute the pipeline in a loop until all processes have failed.
  virtual bool run() = 0;

  /// If anything is running asynchronously, shut down the threads cleanly
  /// In a fully synchronous pipeline, this function does nothing
  virtual bool cancel();


  virtual bool initialize();

  /// Reset the pipeline after a failure so that it can be restarted
  virtual bool reset();

  /// Reset one pipeline node after a failure so that it can be restarted
  virtual bool reset_node( node_id_t id );

  /// Reset only this node and the nodes in the pipeline that are downstream..
  /// These are the nodes that should have fail as a result of \a id failing.
  bool reset_downstream(node_id_t id);

  config_block params() const;


  bool set_params( config_block const& all_params );

  /// Set parameters only this node and the nodes in the pipeline that are downstream.
  bool set_params_downstream( config_block const& all_params, node_id_t id );

  void set_output_node( node_id_t node_id, bool value );

  /// Set a new default edge capacity size such that all new port connections
  /// (pipeline edges) created after this call will be made with the new edge
  /// capacity, without modifying any pre-existing edge lengths.
  void set_edge_capacity( unsigned edge_capacity );

  /** Write pipeline structure to a file.  This method writes the
   * structure of this pipeline to the specified file in "dot"
   * compatible format.
   * @param[in] filename - name of file to create.
   */
  void output_graphviz_file( std::string const& filename ) const;

  /** Get map of processes in pipeline. A map is returned that is
   * indexed by process instance name and refers to the associated
   * pipeline node object. This looks really awsome for those that
   * know what to do with it.
   */
  std::map< std::string, pipeline_node* > enumerate_nodes() const;

  std::map<std::string, double> collect_node_timing() const;

  void output_detailed_report( std::string const& indent = "" );

  /// Output timing measurements in a DartMeasurement XML format for use in CDash.
  void output_timing_measurements() const;
  void output_timing_measurements(std::ostream& str) const;

  void set_print_detailed_report( bool );

protected:
  /// add a pipeline node and take ownership of it
  void add( pipeline_node * n );
  /// add a non-executing pipeline node and take ownership of it
  void add_without_execute( pipeline_node * n );

  /// connect a source id to sink id using an already constructed edge.
  void do_connect(node_id_t source_id,
                  node_id_t sink_id,
                  pipeline_edge* edge,
                  std::string const& input_name);

  /// Return this node and the nodes in the pipeline that are downstream..
  std::vector< pipeline_node* > downstream_nodes( pipeline_node* n );

  /// find the pipeline node for a given id (process address)
  pipeline_node* get_node_with_id( node_id_t id ) const;

  const std::vector<pipeline_node*>& execution_order()
  {
    return execution_order_;
  }

  // These functions are used to ensure at compile time that the
  // output type of source node matches the input type of the sink
  // node.  This allows the type error to be flagged at the connect()
  // line, as opposed to an error being triggered much later somewhere
  // else in the code.  These ideas are lifted from Boost the
  // concept_check code.

  template<class A, class B>
  void type_mismatch( B )
  {
  }

  template<class A, class B>
  void convertable_check_trigger( A a, B )
  {
    // If an error message point you to here, then the input and
    // output data types in your connection don't match.
    type_mismatch<A, B>( a );
  }

  template<class From, class To>
  inline void convertable_check()
  {
    void (pipeline::*x)(From,To) = &pipeline::convertable_check_trigger<From,To>;

    // to avoid an unused variable warning on gcc.
    void (pipeline::*y)(From,To) = NULL;
    x = y;
    y = x;
  }

private:
  void reorder();

  void visit_depth_first( pipeline_node* node );

  void output_graphviz_legend( std::ostream& fstr ) const;

protected:
  double elapsed_ms_;
  unsigned long step_count_;

  double total_elapsed_ms_;
  unsigned long total_step_count_;

  /// Whether to call output_detailed_report() at the end of execution or not.
  bool print_detailed_report_;

  // Maximum number of data items that can be stored inside a pipeline edge.
  unsigned edge_capacity_;

private:
  // If true, the node as already been inserted into the execution
  // list (and thus will be executed before any node that has not been
  // visited).
  std::map< node_id_t, bool > visited_;

  // 0 = not processed, 1 = in process, 2 = fully explored.
  // Used to detect cycles.
  std::map< node_id_t, int > color_;

  std::vector<pipeline_node*> insert_order_;
  std::vector<pipeline_node*> execution_order_;
  std::map<node_id_t, pipeline_node*> nodes_;

  pipeline_node::execute_function_type do_nothing_execute_;
};  // class pipeline


typedef vbl_smart_ptr<pipeline> pipeline_sptr;


/// Intermediate pipeline base class to handle template connection functions
///
/// The template parameter is the derived class itself (this is a CRTP).  For example,
/// \code
/// class my_pipeline : public pipeline_impl<my_pipeline>;
/// \endcode
/// The derived class should provided a nested struct \c edge that has template
/// parameters \c From and \c To.  The \c edge struct should define a typedef
/// \c type that provides to type of edge to use in connecting nodes.
///
/// \code
/// template <class From, class To>
/// struct edge
/// {
///   typedef my_pipeline_edge_impl<From,To> type;
/// };
/// \endcode
template <class Pipeline>
class pipeline_impl
  : public pipeline
{
public:
  /// @{
  /// Connect nodes.
  ///
  /// See the overall class documentation about the return value.
  template< class R1, class R2 >
  void connect( pipeline_aid::output_port_description<R1> const& out,
                pipeline_aid::input_port_description<R2> const& in )
  {
    convertable_check<R1,R2>();
    do_connect( out.id_, out.out_func_,
                in.id_, in.in_func_,
                /*add execution dependency=*/ true,
                /*connection is optional=*/ in.optional_,
                out.name_,
                in.name_ );
  }


  template< class R1, class R2 >
  void connect_optional( pipeline_aid::output_port_description<R1> const& out,
                         pipeline_aid::input_port_description<R2> const& in )
  {
    convertable_check<R1,R2>();
    do_connect( out.id_, out.out_func_,
                in.id_, in.in_func_,
                /*add execution dependency=*/ true,
                /*connection is optional=*/ true,
                out.name_,
                in.name_ );
  }


  template< class R1, class R2 >
  void connect_without_dependency( pipeline_aid::output_port_description<R1> const& out,
                                   pipeline_aid::input_port_description<R2> const& in )
  {
    convertable_check<R1,R2>();
    do_connect( out.id_, out.out_func_,
                in.id_, in.in_func_,
                /*add execution dependency=*/ false,
                /*connection is optional=*/ in.optional_,
                out.name_,
                in.name_ );
  }

  template< class R1, class R2 >
  void connect_multiple( pipeline_aid::output_port_description<R1> const& out,
                         pipeline_aid::input_port_description<R2> const& in )
  {
    if (Pipeline::is_async)
    {
      LOG_AND_DIE("Connecting to an input port multiple times "
                  "is not supported in an async pipeline.");
      return;
    }

    if (!in.multiple_)
    {
      LOG_AND_DIE("The input port does not support "
                  "multiple connections.");
      return;
    }

    // Shame the user who calls this method.
    LOG_INFO( "A duplicate connection is being made between "
              << out.id_->name() << "." << out.name_ << " and "
              << in.id_->name() << "." << in.name_ << " which "
              "is discouraged and deprecated." );

    convertable_check<R1,R2>();
    do_connect( out.id_, out.out_func_,
                in.id_, in.in_func_,
                /*add execution dependency=*/ true,
                /*connection is optional=*/ in.optional_,
                out.name_,
                in.name_,
                true );
  }

  template< class R1, class R2 >
  void connect_optional_multiple( pipeline_aid::output_port_description<R1> const& out,
                                  pipeline_aid::input_port_description<R2> const& in )
  {
    if (Pipeline::is_async)
    {
      LOG_AND_DIE("Connecting to an input port multiple times "
                  "is not supported in an async pipeline.");
      return;
    }

    if (!in.multiple_)
    {
      LOG_AND_DIE("The input port does not support "
                  "multiple connections.");
      return;
    }

    // Shame the user who calls this method.
    LOG_INFO( "An optional duplicate connection is being made between "
              << out.id_->name() << "." << out.name_ << " and "
              << in.id_->name() << "." << in.name_ << " which "
              "is discouraged and deprecated." );

    convertable_check<R1,R2>();
    do_connect( out.id_, out.out_func_,
                in.id_, in.in_func_,
                /*add execution dependency=*/ true,
                /*connection is optional=*/ true,
                out.name_,
                in.name_,
                true );
  }

  /// @}

protected:
  pipeline_impl( unsigned edge_capacity )
    : pipeline( edge_capacity )
  {
  }
  virtual ~pipeline_impl()
  {
  }

private:
  template<class From, class To>
  void do_connect( node_id_t source_id, boost::function< From() > const& output,
                   node_id_t sink_id, boost::function< void(To) > const& input,
                   bool _add_execution_dependency,
                   bool connection_is_optional,
                   std::string const& output_name,
                   std::string const& input_name,
                   bool multiple = false )
  {
    typedef typename Pipeline::template edge<From,To>::type edge_type;
    pipeline_edge_impl<From,To>* e = new edge_type( edge_capacity_ );
    e->from_port_name_ = output_name;
    e->to_port_name_ = input_name;
    e->get_output_ = output;
    e->set_input_ = input;
    e->implies_execution_dependency_ = _add_execution_dependency;
    e->optional_connection_ = connection_is_optional;

    try
    {
      pipeline::do_connect(source_id, sink_id, e, multiple ? "" : input_name);
    }
    catch (std::runtime_error &exc)
    {
      LOG_ERROR ("Could not connect \"" << output_name << "\" port to \"" << input_name
                 << "\" port\n" << exc.what() );

      // rethrow exception
      throw exc;

    } // end catch
  }
};  // class pipeline_impl

} // end namespace vidtk


#endif // pipeline_h_
