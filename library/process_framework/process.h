/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
  \file
   \brief
   Method and field definitions necessary for a process.
*/

#ifndef vidtk_process
#define vidtk_process

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <utilities/config_block.h>
#include <boost/function.hpp>

namespace vidtk
{


class process;

void delete_process_object( process* );

//TODO: make processes always dynamic objects.  This will avoid issues
//with pipeline nodes owning process pointers, and deleting this
//resources, and all that.

/// A generic processing element.
///
/// A process typically has a series of set_X() methods to set the
/// inputs to the algorithm, and a set of Y() methods to retrieve the
/// output.  The step() function is called between these two sets, so
/// that the typical execution order for two processes is
/// \code
///    p1->set_input1( ... );
///    p1->set_input2( ... );
///    p1->step();
///    p2->set_input1( p1->output2() );
///    p2->set_input2( p1->output5() );
///    p2->step();
/// \endcode
///

/// \brief Parent class for processes.
///
/// In general, a process is expected to make sure that the objects
/// returned by the output methods are valid until the next call to
/// step().  In the example above, p2.set_input2() could take it's
/// argument by reference, store that reference, and expect the
/// reference to be valid when executing p2.step().  This implies that
/// the output of p1.output5() should not be a temporary.
class process
  : public vbl_ref_count
{
public:
  ///Creates name for type vbl_smart_ptr<process>
  typedef vbl_smart_ptr<process> pointer;

  /// These are the return states of the \c step2()
  enum step_status {FAILURE, SUCCESS, SKIP};

  typedef boost::function< void (step_status) > push_output_function_type;

  /// \brief Create a process with name \a name.
  ///
  /// Derived classes should force the user to set \a name, since it
  /// should be an application specific name.  For example, the user
  /// may name two input sources "video0" and "video1".  However, the
  /// derived class should \b not allow the user to change \a
  /// class_name, and should set it to the name of its own class.
  /// This allows applications to output better help messages.  For
  /// example, when displaying the pipeline graph, the video nodes
  /// could be annotated as "video0/generic_frame_process",
  /// "video1/usb_webcam_process".  The user would then be more aware
  /// of the actual processing being node on a particular node.
  ///
  /// \note There are no default to force the derived classes to set
  /// the \a class_name, and to request the user for a \name.
  ///
  /// \note We ask the derived classes to set a class name because
  /// there is no standard way in C++ to get a human-readable name for
  /// a class from within an application.

  ///Constructor : pass name and class name.
  process( vcl_string const& name, vcl_string const& class_name );

  ///Returns the current configuration parameters.
  virtual config_block params() const = 0;

  ///Allows the user to set the configuration parameters. The child class must implement this.
  virtual bool set_params( config_block const& ) = 0;

  /// \brief Update a single parameter in the block.
  ///
  /// This updates the parameter \a name to \a value while keeping all
  /// the other parameters unchanged.
  virtual bool set_param( vcl_string const& name, vcl_string const& value );

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

  /// The step function is where the input is processed into the output.
  /// Child classes must implement either this or \c step2().
  virtual bool step()
  {
    return false;
  }

  /// Calls \c step() for backward compatibility, but allows for more return states.
  virtual step_status step2()
  {
    return this->step() ? SUCCESS : FAILURE;
  }

  /// Set a function to call when \c push_output() is called
  process& set_push_output_func( push_output_function_type const& f )
  {
    this->push_output_func_ = f;
    return *this;
  }

  /// \brief The name of the process.
  ///
  /// This is used for output error messages and whenever else a name is required.
  vcl_string const& name() const;

  /// The class name of the process.
  ///
  /// This is used to describe the node type.
  vcl_string const& class_name() const;

  ///Set the name of the process.
  void set_name( vcl_string const& );

  // There is no set_class_name by design, since the class name should never change.

  // If you get an error that this destructor is private, that is
  // because you are attempting to delete the object yourself, instead
  // of using a process smart pointer to manage the lifetime of the
  // object.  Use a process_sptr, and you will be happy.
  ///Destructor.
  virtual ~process() {};

protected:

  /// In an asynchronous pipeline, push output without returning from step.
  /// This simulates returning from step with \a status without actually
  /// returning.  This allows multiple outputs for a single input in an
  /// asynchronous pipeline.  A final output is triggered as usual when step
  /// actually returns.  In a synchronous pipeline this is a no-op.
  bool push_output(step_status status);

private:

  friend void delete_process_object( process* );

  push_output_function_type push_output_func_;

  vcl_string name_;
  vcl_string class_name_;
};



///
/// Since all process objects should be using these smart pointers,
/// this implementation passes const through to the object.  That is,
/// a pointer<T> const is treated as pointer<const T> const.
template<class SelfT>
/// A smart pointer to a process.
class process_smart_pointer
  : public vbl_smart_ptr<process>
{
  ///Creates name for type process_smart_pointers
  typedef process_smart_pointer self;

public:
  ///Constructor : pass a pointer of SelfT type.
  explicit process_smart_pointer( SelfT* p )
    : vbl_smart_ptr<process>( p )
  {
  }
  ///Return the base smart pointer.
  vbl_smart_ptr<process> get_base_ptr()
  {
    vbl_smart_ptr<process> &tmp = this;
    return tmp;
  }
  ///Return smart pointer.
  operator process::pointer ( )
  {
    process::pointer &p = *this;
    return p;
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
  SelfT* operator->()
  {
    return static_cast<SelfT*>( this->ptr() );
  }
  ///Return value of the smart pointer.
  SelfT const* operator->() const
  {
    vidtk::process_smart_pointer< SelfT >* my_nonconst_this =
      const_cast< vidtk::process_smart_pointer< SelfT >* >(this);
    SelfT * my_ptr = my_nonconst_this->ptr();

    return const_cast<SelfT const*>( my_ptr );
  }
  ///Return value of smart pointer.
  SelfT* ptr()
  {
    return static_cast<SelfT*>( vbl_smart_ptr<process>::ptr() );
  }
};


} // end namespace vidtk


#endif // vidtk_process
