/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_PASS_THRU_HELPER_H_
#define _VIDTK_PASS_THRU_HELPER_H_

#include <utilities/timestamp.h>
#include <ostream>
#include <list>

// ----------------------------------------------------------------
/** Pass througth helper
 *
 * This class provides all the boiler plate for adding a set of pass
 * through ports to a process.  It facilitates collecting a set of
 * inputs into a group associated with a timestamp. These inputs are
 * held internally until they are popped and made available to the
 * output ports.
 *
 * The list of data items to be passed through is driven by entries in
 * a macro, so it is easy to add or delete items to be passed through.
 *
 * The goal of this class is to provide pass-thru capability while
 * retaining a light touch on the main process.
 *
 * Sets of input data that are to be passed through are pushed into
 * the internal FIFO storage. Data that needs to be passed to the
 * output ports is poped from the internal storage. The output data is
 * keyed on the output timestamp. If the timestamp from the pass-thru
 * storage is no the same as the timestamp on the output ports, a
 * message is logged.
 *
 * The pass thru data list is usually defined in the process include
 * file as follows:
 \code
  // Pass through ports (type, name, default-value)
  // The name is process specific to prevent macro name collisions since
  // this symbol is visible to all who include the process interface.
#define PASS_THRU_DATA_LIST(CALL)                                             \
  CALL ( vidtk::image_to_image_homography,  src_to_ref_homography,   vidtk::image_to_image_homography()) \
  CALL ( vidtk::image_to_utm_homography,    src_to_utm_homography,   vidtk::image_to_utm_homography() ) \
  CALL ( vidtk::image_to_plane_homography,  ref_to_wld_homography,   vidtk::image_to_plane_homography()) \
  CALL ( double,                            world_units_per_pixel,   0.0 ) \
  CALL ( vidtk::video_modality,             video_modality,          vidtk::VIDTK_INVALID ) \
  CALL ( vidtk::shot_break_flags,           shot_break_flags,        vidtk::shot_break_flags() )

\endcode

To support templated parameters a type can be forward referenced and
used as the type name in the list:

\code
CALL( local_image_t, image, local_image_t() ) \
\endcode

The type name used in the data list needs to be defined in the
templated class as follows:

\code
template < class PixType >
process foo_process
: public vidtk::process
{
public:
  // -- TYPES --
  typedef vil_image_view< PixType > local_image_t; //  templated image type
  typedef foo_process self_type;

  - - - - some code - - -

  // ------------- pass thru ports ------------

  // This user supplied code segment declares the input and output
  // methods for this process. This code is application specific since it
  // needs to match the port naming convention. A similar piece of code
  // needs to be in the implementation to define the input and output
  // methods.

#define DECLARE_PASS_THRU_PORTS(T,N,D)                  \
void input_ ## N( T const& val);                        \
VIDTK_OPTIONAL_INPUT_PORT( input_ ## N, T const& );     \
T const& output_ ## N() const;                          \
VIDTK_OUTPUT_PORT( T const&, output_ ## N);

  PASS_THRU_DATA_LIST(DECLARE_PASS_THRU_PORTS)

#undef GENERATE_PORTS

- - - more code - - -

}; // end process class
\endcode

The following is an example of how the helper is used in the source file.

The input and output methods can be defined as follows. The actual code will
depend on how the process manages its storage.

\code
// set up to use the list of process ports. The macro PASS_THRU_PORTS must be
// defined before including the helper code.
#define PASS_THRU_PORTS(C) PASS_THRU_DATA_LIST(C)

#include <utilities/pass_thru_helper.h>

// An easy to use the helper is to derive the internal implementation
// class from the helper.
class foo_process::impl
  : public pass_thru_helper
{
};

// Define pass thru methods for process class
#define DEFINE_PASS_THRU_METHODS(T,N,D)                                 \
  void foo_process::set_ ## N( T const& val) { this->impl_->in_ ## N(val); } \
  T foo_process::get_ ## N() const { return this->impl_->output_slice_.N; }

  PASS_THRU_PORTS(DEFINE_PASS_THRU_METHODS)

#undef DEFINE_PASS_THRU_METHODS
\endcode

A typical process step() method would use the pass-thru helper as follows:

\code
process::step_status
foo_process
::step2()
{
  process::step_status ret_code;

  output_terminated_tracks_.clear();
  output_active_tracks_.clear();

  // All the inputs that have been sent to the input ports defined in the helper
  // are pushed into the helpers internal storage. The timestamp is used to
  // uniquely identify the set of inputs.
  impl_->push_pass_thru_input( current_ts_ ); // save pass through data

  if ( disabled_ )
  {
    ret_code = disabled_processing();
  }
  else
  {
    ret_code = enabled_processing();
  }

  if ( ret_code != SKIP )
  {
    // The oldest set of pass-thru data is popped and made available to the
    // output ports. An warning is logged if the supplied timestamp does not
    // match the one with the popped set of data.
    impl_->pop_pass_thru_output( finalized_ts_ );
  }

  return ret_code;
}
\endcode

 */
class pass_thru_helper
{
private:

#define PORT_LIST_META(M,C) M(C)

  // Define pass through data structure
  // Represents a input data slice
  struct pass_thru_data {
      // -- TYPES --
      // bit mask for inputs that are present
      enum {
#define DEF_PRESENT(T, N, D) N ## _present,
        PORT_LIST_META(PASS_THRU_PORTS, DEF_PRESENT)
#undef DEF_PRESENT

        NUMBER_OF_INPUTS
      };

    // local storage
#define DEF(T,N,D)  T N;

    PORT_LIST_META(PASS_THRU_PORTS, DEF)

#undef DEF

    // These are control fields, not part of the slice.
    vidtk::timestamp timestamp_;
    int input_count_;
    unsigned input_mask_;

    // CTOR
    pass_thru_data()
      :
#define CTOR(T,N,D) N(D),
      PORT_LIST_META(PASS_THRU_PORTS, CTOR)
#undef CTOR

      timestamp_( vidtk::timestamp() ),
      input_count_(0),
      input_mask_(0)
    {
    }

    std::ostream & inputs_present (std::ostream & str) const
    {
#define DATUM_DEF(T, N, V)                                              \
      str << # N                                                        \
          << ( (this->input_mask_ & (1 << N ## _present)) ? "" :  " not") \
          << " present, ";

      PORT_LIST_META(PASS_THRU_PORTS, DATUM_DEF)
#undef DATUM_DEF

      return (str);
    }

  };                            // end of pass_thru_data

  // List of pass-thru slices we are buffering.
  std::list< pass_thru_data > pass_thru_vector_;

protected:
  pass_thru_helper()
  {
  }


public:

  virtual ~pass_thru_helper()
  {
  }

// Define accessor methods for current slices
#define PASS_THRU_IO_METHODS(T,N,D)                                     \
  void in_ ## N( T const& val)                                          \
  {                                                                     \
    this->input_slice_.N = val;                                         \
    this->input_slice_.input_count_++;                                  \
    this->input_slice_.input_mask_ |= (1 << pass_thru_data:: N ## _present); \
  }                                                                     \
  T const& out_ ## N() const { return this->output_slice_.N; }

  PASS_THRU_PORTS(PASS_THRU_IO_METHODS)

#undef PASS_THRU_IO_METHODS


  void reset_pass_thru_helper()
  {
    this->pass_thru_vector_.clear();
  }


  // ----------------------------------------------------------------
  /** \brief Is pass-thru queue empty.
   *
   * Determines if the pass thru queue is empty
   *
   * @return \b true is queue is empty; \b false if not
   */
  bool pass_thru_helper_empty() const
  {
    return this->pass_thru_vector_.empty();
  }


  // ----------------------------------------------------------------
  /** \brief Push current input slice onto list.
   *
   * @param ts - timestamp to put on slice.
   */
  void push_pass_thru_input( vidtk::timestamp const& ts)
  {
    this->input_slice_.timestamp_ = ts;

    this->pass_thru_vector_.push_back( this->input_slice_ );
    this->input_slice_ = pass_thru_data(); // set to default values
  }


  // ----------------------------------------------------------------
  /** \brief Pop oldest set of inputs.
   *
   * The oldest set of inputs is removed from the list. If a valid
   * timestamp is supplied, it is checked against the timestamp of the
   * slice to be output. An message is logged if they do not match.
   *
   * @param ts Expected timestamp of output slice.
   */
  void pop_pass_thru_output( vidtk::timestamp const& ts)
  {
    if ( this->pass_thru_vector_.empty() )
    {
      LOG_WARN( "Underflow in pass thru data list at " << ts
                << ". Supplying default values" );
      this->output_slice_ = pass_thru_data(); // set to default values
      return;
    }

    this->last_output_slice_ =  // save last outputs
    this->output_slice_      = this->pass_thru_vector_.front();
    this->pass_thru_vector_.pop_front();

    if ( ts.is_valid() && ( ts != this->output_slice_.timestamp_ ) )
    {
      LOG_WARN( "output timestamp [" << this->output_slice_.timestamp_
                << "] does not match expected timestamp [" << ts << "]" );
    }
  }


  // ----------------------------------------------------------------
  /*! \brief Return last timestamp generated.
   *
   * This is needed by some processes that have to generate an extra
   * cycle when flushing internal data areas.
   */
  vidtk::timestamp last_pass_thru_timestamp()
  {
    return this->last_output_slice_.timestamp_;
  }


  // ----------------------------------------------------------------
  /*! \brief Return last output slice.
   *
   * This is needed by some processes that have to generate an extra
   * cycle when flushing internal data areas.
   */
  void last_pass_thru_output()
  {
    this->output_slice_ = this->last_output_slice_;
  }


  // Storage for current input and current output slices
  pass_thru_data input_slice_;
  pass_thru_data output_slice_;

  // Last set of outputs
  pass_thru_data last_output_slice_;


#undef PORT_LIST_META

}; // end class pass_thru_helper

#endif /* _VIDTK_PASS_THRU_HELPER_H_ */
