/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef pipeline_h_
#define pipeline_h_


#include <vcl_map.h>
#include <vcl_vector.h>
#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <pipeline/pipeline_node.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/log.h>



template<class T>
struct cannot_convert
{
  cannot_convert( T )
  {
  }
};

namespace vidtk
{


class pipeline
  : public vbl_ref_count
{
public:
  pipeline();
  ~pipeline();

  virtual void add( process* p ) = 0;
  virtual void add( process::pointer p ) = 0;

  // Add a process that will not execute, but
  // will be part of the pipeline. Useful for allowing
  // a configuration file to contain settings for
  // pipeline elements that are not always used.
  virtual void add_without_execute( process* p ) = 0;
  virtual void add_without_execute( process::pointer p ) = 0;

  void add_execution_dependency( void* source_id, void* sink_id );
  void add_execution_dependency( process::pointer, process::pointer );
  void add_optional_execution_dependency( void* source_id, void* sink_id );


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
  virtual bool reset( void* id );

  /// Reset only this node and the nodes in the pipeline that are downstream..
  /// These are the nodes that should have fail as a result of \a \id failing.
  bool reset_downstream(void* id);

  config_block params() const;


  bool set_params( config_block const& all_params );

  /// Set parameters only this node and the nodes in the pipeline that are downstream.
  bool set_params_downstream( config_block const& all_params, void* id );

  void set_output_node( void* node_id, bool value );

  void output_graphviz_file( vcl_string const& filename ) const;

  vcl_map< vcl_string, pipeline_node* > enumerate_nodes() const;

  vcl_map<vcl_string, double> collect_node_timing() const;

  void output_detailed_report( vcl_string const& indent = "" );

  // output timing measurements in a DartMeasurement XML format for use in CDash.
  void output_timing_measurements() const;
  void output_timing_measurements(vcl_ostream& str) const;

  void set_print_detailed_report( bool );

protected:
  /// add a pipeline node and take ownership of it
  void add( pipeline_node * n );
  /// add a non-executing pipeline node and take ownership of it
  void add_without_execute( pipeline_node * n );

  /// connect a source id to sink id using an already constructed edge.
  void do_connect( void* source_id, void* sink_id, pipeline_edge* edge);

  /// Return this node and the nodes in the pipeline that are downstream..
  vcl_vector< pipeline_node* > downstream_nodes( pipeline_node* n );

  /// find the pipeline node for a given id (process address)
  pipeline_node* get_node_with_id( void* id ) const;

  const vcl_vector<pipeline_node*>& execution_order()
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

  void output_graphviz_legend( vcl_ostream& fstr ) const;

protected:
  double elapsed_ms_;
  unsigned long step_count_;

  double total_elapsed_ms_;
  unsigned long total_step_count_;

  /// Whether to call output_detailed_report() at the end of execution or not.
  bool print_detailed_report_;

private:
  // If true, the node as already been inserted into the execution
  // list (and thus will be executed before any node that has not been
  // visited).
  vcl_map< void*, bool > visited_;

  // 0 = not processed, 1 = in process, 2 = fully explored.
  // Used to detect cycles.
  vcl_map< void*, int > color_;

  vcl_vector<pipeline_node*> insert_order_;
  vcl_vector<pipeline_node*> execution_order_;
  vcl_map<void*, pipeline_node*> nodes_;

  pipeline_node::execute_function_type do_nothing_execute_;
};  // class pipeline


typedef vbl_smart_ptr<pipeline> pipeline_sptr;


/// Intermediate pipeline base class to handle template connection functions
///
/// The template parameter is the derived class itself.  For example,
/// \code class my_pipeline : public pipeline_impl<my_pipeline>; \endcode
/// The derived class should provided a nested struct \c edge that has template
/// parameters \c From and \c To.  The \c edge struct should define a typedef
/// \c type that provides to type of edge to use in connecting nodes.
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
  template<class R, class F, class L, class F2, class L2>
  void connect( void* source_id, boost::_bi::bind_t<R,F,L> output,
                void* sink_id, boost::_bi::bind_t<void,F2,L2> input )
  {
    do_connect( source_id, boost::function<R()>(output),
                sink_id, boost::function<void(R)>(input),
                /*add execution dependency=*/ true,
                /*connection is optional=*/ false );
  }


  template< class R1, class R2 >
  void connect( vcl_pair< void*,
                          boost::function< R1() > > const& out,
                vcl_pair< void*,
                          boost::function< void(R2) > > const& in )
  {
    convertable_check<R1,R2>();
    do_connect( out.first, out.second,
                in.first, in.second,
                /*add execution dependency=*/ true,
                /*connection is optional=*/ false );
  }


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


  template<class R, class F, class L, class F2, class L2>
  void connect_optional( void* source_id, boost::_bi::bind_t<R,F,L> output,
                         void* sink_id, boost::_bi::bind_t<void,F2,L2> input )
  {
    do_connect( source_id, boost::function<R()>(output),
                sink_id, boost::function<void(R)>(input),
                /*add execution dependency=*/ true,
                /*connection is optional=*/ true );
  }


  template< class R1, class R2 >
  void connect_optional( vcl_pair< void*,
                         boost::function< R1() > > const& out,
                         vcl_pair< void*,
                         boost::function< void(R2) > > const& in )
  {
    convertable_check<R1,R2>();
    do_connect( out.first, out.second,
                in.first, in.second,
                /*add execution dependency=*/ true,
                /*connection is optional=*/ true );
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
  void connect_if( vcl_pair< void*,
                             boost::function< R1() > > const& out,
                   vcl_pair< void*,
                             boost::function< void(R2) > > const& in,
                   vcl_pair< void*,
                             boost::function< bool() > > const& verifier )
  {
    convertable_check<R1,R2>();
    do_connect( out.first, out.second,
                in.first, in.second,
                /*add execution dependency=*/ true,
                /*connection is optional=*/ false );
  }


  template<class R, class F, class L, class F2, class L2>
  void connect_without_dependency( void* source_id, boost::_bi::bind_t<R,F,L> output,
                                   void* sink_id, boost::_bi::bind_t<void,F2,L2> input )
  {
    do_connect( source_id,
                output,
                sink_id,
                /*add execution dependency=*/ false,
                /*connection is optional=*/ false );
  }


  template< class R1, class R2 >
  void connect_without_dependency( vcl_pair< void*,
                                   boost::function< R1() > > const& out,
                                   vcl_pair< void*,
                                   boost::function< void(R2) > > const& in )
  {
    convertable_check<R1,R2>();
    do_connect( out.first, out.second,
                in.first, in.second,
                /*add execution dependency=*/ false,
                /*connection is optional=*/ false );
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

  /// @}

private:
  template<class From, class To>
  void do_connect( void* source_id, boost::function< From() > const& output,
                   void* sink_id, boost::function< void(To) > const& input,
                   bool add_execution_dependency,
                   bool connection_is_optional,
                   vcl_string const& output_name = vcl_string(),
                   vcl_string const& input_name = vcl_string() )
  {
    typedef typename Pipeline::template edge<From,To>::type edge_type;
    pipeline_edge_impl<From,To>* e = new edge_type;
    e->from_port_name_ = output_name;
    e->to_port_name_ = input_name;
    e->get_output_ = output;
    e->set_input_ = input;
    e->implies_execution_dependency_ = add_execution_dependency;
    e->optional_connection_ = connection_is_optional;

    try {
      pipeline::do_connect(source_id, sink_id, e);
    }
    catch (vcl_runtime_error &e)
    {
      log_error ("Could not connect \"" << output_name << "\" port to \"" << input_name
                 << "\" port\n" << e.what() << "\n");

      // rethrow exception
      throw e;

    } // end catch
  }
};  // class pipeline_impl

} // end namespace vidtk


#endif // pipeline_h_
