/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
  \file
   \brief
   Method and field definitions necessary for a process.
*/

#ifndef vidtk_process_h_
#define vidtk_process_h_

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <utilities/config_block.h>
#include <boost/function.hpp>

namespace vidtk
{


class process;

void delete_process_object( process* );

// ----------------------------------------------------------------
/** Parent class for processes.
 *
 * In general, a process is expected to make sure that the objects
 *  returned by the output methods are valid until the next call to
 *  step2(). In the example above, p2.set_input2() could take it's
 *  argument by reference, store that reference, and expect the
 *  reference to be valid when executing p2.step2(). This implies that
 *  the output of p1.output5() should not be a temporary.
 *
 * A process typically has a series of set_X() methods to set the
 * inputs to the algorithm, and a set of Y() methods to retrieve the
 * output.  The step2() function is called between these two sets, so
 * that the typical execution order for two processes is:
 *
 * \code
 *   p1->set_input1( ... );
 *   p1->set_input2( ... );
 *   p1->step2();
 *   p2->set_input1( p1->output2() );
 *   p2->set_input2( p1->output5() );
 *   p2->step2();
 * \endcode
 *
 * The folw of control in a pipeline is shown in the following diagram:
 *
 * \msc
 main, pipeline, process_1, process_2, pipeline_node, pipeline_edge;

 main=>process_1 [ label = "<<create>>" ];
 main=>process_2 [ label = "<<create>>" ];

 main=>pipeline [ label = "add(process_1)"];
 main=>pipeline [ label = "add(process_2)"];
 main=>pipeline [ label = "connect(s,d)" ];

 main=>pipeline [ label = "params()" ];
 main=>process_1 [ label = "params()" ];
 main=>process_2 [ label = "params()" ];

 main=>pipeline [ label = "set_params(config)" ];
 main=>process_1 [ label = "set_params(config)" ];
 main=>process_2 [ label = "set_params(config)" ];

 main=>pipeline [ label = "initialize()" ];
 main=>process_1 [ label = "initialize()" ];
 main=>process_2 [ label = "initialize()" ];

 main=>pipeline [ label = "execute()" ];
 pipeline=>pipeline_node [ label = "execute_incoming_edges()" ];
 pipeline_node=>pipeline_edge [ label = "execute()" ];
 pipeline_edge=>process_1 [ label = "<move data on edge>" ];
 pipeline_edge=>process_2 [ label = "<move data on edge>" ];
 pipeline=>pipeline_node [ label = "execute()" ];
 pipeline_node=>process_1 [ label = "step2()" ];
 pipeline=>pipeline_node [ label = "execute()" ];
 pipeline_node=>process_2 [ label = "step2()" ];
 * \endmsc
 *
 * @todo Make processes always dynamic objects.  This will avoid
 * issues with pipeline nodes owning process pointers, and deleting
 * \b this resources, and all that.
 */
class process
  : public vbl_ref_count
{
public:
  ///Creates name for type vbl_smart_ptr<process>
  typedef vbl_smart_ptr<process> pointer;

  /// These are the return states of the \c step2() method
  enum step_status
  {
    /// Indicates a total process failure. The process will not be
    /// called again.
    FAILURE,

    /// Indicates process succeeded and process outputs are valid.
    SUCCESS,

    /// Indicates process is still o.k. but did not create outputs.
    /// In this case, no downstream processes will not be supplied
    /// with inputs and may not be called at all if they have no
    /// optional inputs, but that is another story.  \sa skipped()
    SKIP,

    /// Indicates we should flush all elements in all edges between
    /// downstream processes and pipelines from this node, as quickly
    /// as possible. This call is non-deterministic and useful in a few
    /// limited circumstances, such as when we know that all processing
    /// in processes after this node is irrelevant for future outputs.
    FLUSH,

    /// Indicates process is still o.k. but did not create outputs.
    /// For processing nodes running in async mode, the difference between
    /// this output and a SKIP operation is that no output will be pushed
    /// unto outgoing pipeline edges. It is a simple special case speed
    /// optimization for when you know all downstream processes stem
    /// from this process, which should only be used in rare circumstances.
    NO_OUTPUT
  };

  typedef boost::function< void (step_status) > push_output_function_type;

  /** Create a process with name \a name.
  *
  * Derived classes should force the user to set \a name, since it
  * should be an application specific name.  For example, the user may
  * name two input sources "video0" and "video1".  However, the
  * derived class should \b not allow the user to change \a
  * class_name, and should set it to the name of its own class.  This
  * allows applications to output better help messages.  For example,
  * when displaying the pipeline graph, the video nodes could be
  * annotated as "video0/generic_frame_process",
  * "video1/usb_webcam_process".  The user would then be more aware of
  * the actual processing being node on a particular node.
  *
  * \note We ask the derived classes to set a class name because there
  * is no standard way in C++ to get a human-readable name for a class
  * from within an application.
  *
  * @param name - name of this process instance
  * @param class_name - name of this class of process
  */
  process( std::string const& name, std::string const& class_name );

  /** Returns the current configuration parameters.
   *
   * A process must create a \c config_block that contains all
   * parameters that it is expecting or can support. It is best to
   * create this block in the constructor. The parameter block
   * returned will be tagged with the process name supplied to the
   * constructor.
   */
  virtual config_block params() const = 0;

  ///
  /** Allows the process to set the configuration parameters.
   *
   * This method provides the configuration block that the process
   * provided via the params() method with the updated config values.
   * The values are usually taken from a config file or
   * programatically generated, depending on the environment the
   * process is running in.
   *
   * The process should extract all values from this config_block.
   */
  virtual bool set_params( config_block const& ) = 0;

  /// \brief Update a single parameter in the block.
  ///
  /// This updates the parameter \a name to \a value while keeping all
  /// the other parameters unchanged.
  virtual bool set_param( std::string const& name, std::string const& value );

  ///Initializes the module.
  virtual bool initialize() = 0;

  /// Reset the process to its initial state.
  /// This function is called before restarting the pipeline after a failure.
  virtual bool reset()
  {
    return true;
  }

  /// Cancel all threads. Works best for super processes.
  virtual bool cancel() { return true; }

  /** Convert inputs to outputs.
   * The step function is where the input is processed into the output.
   *
   * This method is primarily for backward compatibility with the
   * single threaded pipeline. All new processes that wish to take
   * advantage of the multi-threaded (concurrent) pipeline should
   * implement step2().
   *
   * \return \c TRUE is returned if the outputs are valid and the
   * process should be stepped again. \c FALSE is returned if the
   * process is not to be called again. \c FALSE is usually returned
   * when the process has no more input or it has failed.
   */
  virtual bool step()
  {
    return false;
  }

  /**
   * \brief Skip recovery.
   *
   * This method is called when a process directly upstream returns a
   * SKIP.  Returning \c false (the default) results in the default
   * behavior of not calling the step2() method. Returning \c true
   * forces the step2() method to be called where it would not
   * otherwise be called.
   *
   * HERE THERE BE DRAGONS.
   *
   * Welcome to the dragon's den that is skip recovery. By overloading this
   * method, you presumably want to return \c true under some conditions. It is
   * highly recommended that 'some' be 'when at least one input was set' to try
   * and keep any edge cases away from the dragons (they have the strangest
   * phobias). Returning \c true if *all* inputs have a SKIP is likely a no-op
   * in step2().
   *
   * In the simplest case, a process can just use defaults for its outputs or
   * persist the last steps' results for the next step (Smaug warns: don't
   * share the actual memory for the same data being pushed out multiple
   * times). This can be seen in the \c skip_adder process in the pipeline test
   * \c sample_nodes.h header (Drogon warns: that process does silly things in
   * the name of testing; take it with a grain of salt).
   *
   * In the more complex case, a process might want to synchronize two streams
   * of data which may independently cause a SKIP within the pipeline. These
   * processes should be designed carefully. Some guidelines:
   *
   *   - check inputs before use on every step (Ridley warns: unset inputs
   *     could be used)
   *   - clear all inputs after each step (Tiamat warns: "stale" input could
   *     persist in the input variables causing improper behavior)
   *   - don't return FAILURE from step2(); if a process returns FAILURE when
   *     recovering from a SKIP, the bottom part of the pipeline will shut down
   *     (Obi Wan warns: this is almost certainly not the behavior you're
   *     looking for)
   *   - implement step2() (Alexstrasza warns: you can't SKIP from step())
   *   - add pass through ports (King Ghidorah warns: having a stream pass by
   *     the process in the pipeline could cause all kinds of fun downstream
   *     since the process you're writing is not synchronized with its inputs
   *     (presumably))
   *
   * The overall algorithm for step2 for a process which handles SKIP should
   * look something like:
   *
   * \code
   *   // Put valid input data into the cache.
   *   cached_inputs.store(new_inputs);
   *
   *   // Look at the ranges of data available to compute with.
   *   newest = max(cached_inputs);
   *   oldest = min(cached_inputs);
   *
   *   // If we've delayed enough, force computation on what is available.
   *   while (cached_inputs.have_all(oldest) ||
   *          (max_delay < (newest - oldest))) {
   *     // Compute using the oldest available data.
   *     set_outputs(compute(cached_inputs[oldest]));
   *     push_output(SUCCESS);
   *
   *     // Drop the oldest data.
   *     cached_inputs.drop(oldest);
   *     oldest = min(cached_inputs);
   *   }
   *   // Any data is pushed out via push_output, just SKIP since there's
   *   // nothing to output at this time.
   *   return SKIP;
   * \endcode
   *
   * The use of push_output pretty much means that the process will only work
   * in the \p async_pipeline. If the \p sync_pipeline needs to be supported,
   * the process would need to have all optional input ports (so that a FAILURE
   * on the inputs doesn't cause a FAILURE in the process itself), return
   * SUCCESS when a computation can be done, FAILURE when no more input is
   * available, or SKIP otherwise. Some pseudo-code:
   *
   * \code
   *   // Put valid input data into the cache.
   *   cached_inputs.store(new_inputs);
   *   // Look at the ranges of data available to compute with.
   *   newest = max(cached_inputs);
   *   oldest = min(cached_inputs);
   *   // If we've delayed enough, force computation on what is available.
   *   if (cached_inputs.have_all(oldest) ||
   *       (max_delay < (newest - oldest))) {
   *     // Compute using the oldest available data.
   *     set_outputs(compute(cached_inputs[oldest]));
   *     // Let downstream processes know about the new result(s).
   *     ret = SUCCESS;
   *
   *     // Drop the oldest data.
   *     cached_inputs.drop(oldest);
   *   } else if (cached_inputs.empty()) {
   *     // There's nothing left to compute.
   *     ret = FAILURE;
   *   } else {
   *     // Nothing to see here; move along.
   *     ret = SKIP;
   *   }
   *   return ret;
   * \endcode
   *
   * In this case, \c skipped() *must* return \c false if all inputs SKIPped
   * since cached_inputs would otherwise be empty and a FAILURE would occur.
   */
  virtual bool skipped() { return false; }

  /** \brief  Convert inputs to outputs.
   *
   * The step function is where the input is processed into the output.
   *
   * All new processes that wish to take advantage of the
   * multi-threaded (concurrent) pipeline should implement step2().
   *
   * This base class contains a default implementation that just calls
   * the legacy step() method.
   *
   * \return An element of the step_status enumeration is returned
   * depending on the status of the process.
   */
  virtual step_status step2()
  {
    return this->step() ? SUCCESS : FAILURE;
  }

  /** Set a function to call when \c push_output() is called.  This is
   * nothing the casual process writer needs to worry about. It is
   * used by the async pipeline infrastructure to implement the
   * push_output() method functionality.
   * @param f - this function is called to implement push_output()
   */
  process& set_push_output_func( push_output_function_type const& f )
  {
    this->push_output_func_ = f;
    return *this;
  }

  /// \brief The name of the process.
  ///
  /// This is used for output error messages and whenever else a name is required.
  std::string const& name() const;

  /// The class name of the process.
  ///
  /// This is used to describe the node type.
  std::string const& class_name() const;

  ///Set the name of the process.
  void set_name( std::string const& );

  // There is no set_class_name by design, since the class name should never change.

  // If you get an error that this destructor is private, that is
  // because you are attempting to delete the object yourself, instead
  // of using a process smart pointer to manage the lifetime of the
  // object.  Use a process_sptr, and you will be happy.
  ///Destructor.
  virtual ~process() {};

protected:

  /** \brief Supply process outputs without returning from step2().
   *
   * In an multi-threaded (asynchronous) pipeline, push output without
   * returning from step.  This simulates returning from step with \a
   * status without actually returning.  This allows multiple outputs
   * for a single input in an asynchronous pipeline.  A final output is
   * triggered as usual when step actually returns.  In a synchronous
   * pipeline this is a no-op.
   *
   * In typical usage, a process has to call push_output() for all but
   * the last set of outputs. This sequencing can be hard to
   * coordinate, so there is an alternate approach. A process can call
   * push_output() for all sets of outputs and then return \c SKIP from
   * step2().
   *
   * \msc
   derived_process,process,pipeline;

   derived_process=>process [ label = "push_output()" ];
   process=>pipeline [ label = "push_output_func()"];
   pipeline=>derived_process [ label = "call process output methods" ];
   process<<pipeline [ label = "push_output_func()"];
   derived_process<<process [ label = "push_output()" ];
   * \endmsc
   */
  bool push_output(step_status status);

private:

  friend void delete_process_object( process* );

  push_output_function_type push_output_func_;

  std::string name_;
  std::string class_name_;
};


// ----------------------------------------------------------------
/** A smart pointer to a process.
 *
 * Since process obejcts are very dynamic, we use a smart pointer to
 * manage them.
 */
template<class SelfT>
class process_smart_pointer
  : public vbl_smart_ptr<process>
{
  ///Creates name for type process_smart_pointers
  typedef process_smart_pointer self;

public:
  ///Constructor : pass a pointer of SelfT type.
  explicit process_smart_pointer( SelfT* p = NULL )
    : vbl_smart_ptr<process>( p )
  {
  }
  ///Return the base smart pointer.
  vbl_smart_ptr<process> get_base_ptr()
  {
    vbl_smart_ptr<process> &tmp = this;
    return tmp;
  }
  ///Overload of the equal operator for use with smart pointers.
  self& operator=( self const &r )
  {
    this->operator=( r.as_pointer() );
    return *this;
  }

  //Arslan's Fault. see below.
  ///Overload of the equal operator for use with processes.
  self& operator=( process * r )
  {
    vbl_smart_ptr<process>::operator=( r );
    return *this;
  }
  ///Overload of the not equal operator for use with smart pointers.
  bool operator!=(SelfT const *p) const
  {
    return *(static_cast<const vbl_smart_ptr<process>* > (this) ) !=
      static_cast<const process *> (p);
  }
  ///Return value of the smart pointer.
  SelfT* operator->() const
  {
    return ptr();
  }
  ///Return value of smart pointer.
  SelfT* ptr() const
  {
    return static_cast<SelfT*>( vbl_smart_ptr<process>::ptr() );
  }
};


} // end namespace vidtk


#endif // vidtk_process_h_
