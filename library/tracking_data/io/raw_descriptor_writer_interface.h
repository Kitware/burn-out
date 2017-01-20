/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _vidtk_raw_descriptor_writer_interface_h_
#define _vidtk_raw_descriptor_writer_interface_h_

#include <tracking_data/raw_descriptor.h>

#include <string>
#include <iostream>


namespace vidtk {

/// Establish a nested name space to keep all the implementation
/// classes from polluting the main name space.
namespace ns_raw_descriptor_writer {

// ----------------------------------------------------------------
/** \brief Interface for raw descriptor writers
 *
 * Ths class represents the abstract interface for descriptor
 * writers. Concrete writers must inherit from this base class and
 * implement this interface.
 */
class raw_descriptor_writer_interface
{
public:
  raw_descriptor_writer_interface();
  virtual ~raw_descriptor_writer_interface();

  bool write( raw_descriptor::vector_t const& desc );
  virtual bool write( descriptor_sptr_t desc ) = 0;


  /**
   * \brief Initialize writer object.
   *
   * This method is called to initialize the selected writer. This
   * method saves the supplied stream pointer into a member variable
   * and calls the derived class initializer initialize().
   *
   * @param str Stream to format on.
   *
   * @return \b true if initialization succeeds; \b false otherwide.
   * @sa initialize()
   */
  bool initialize( std::ostream* str );


  /**
   * \brief Finalize writer.
   *
   * This method is called to signal that the last descriptor has been
   * written. The output stream is guaranteed to be active at this
   * time. Derived classes can use this to cleanup any internal data
   * or to add trailer data to the output stream.
   *
   * Once a writer has been finalized, it is not expected to do any
   * more writing.
   */
  virtual void finalize() = 0;


  /**
   * \brief Is the stream in an error state.
   *
   * This method returns an indication of the stream health. It is
   * equivalent to the good() method on std::ostream, and is provided
   * as a convenience, as in some cases the original stream is not
   * readily available. The same indication can be obtained by
   * checking the original stream passed to the initialize() method.
   *
   * @return \b true if stream is good; \b false otherwise.
   */
  bool is_good() const;


protected:
  /**
   * \brief Initialize writer object.
   *
   * This method is where the writer initializes its internal
   * state. It is called after the object is created and before any
   * write() calls. Derived classes should do whatever is needed for a
   * one time initialization, such as write the output data header.
   *
   * @return \b true if initialization succeeds; \b false otherwide.
   */
  virtual bool initialize() = 0;

  /**
   * \brief Get active output stream.
   *
   * This method returns a reference to the current output stream to
   * provide more syntactic similarity to typical stream usage. If no
   * stream is currently active, an error is logged and std::err is
   * returned.
   *
   * @return Current output stream.
   */
  std::ostream& output_stream();

  /// Pointer to output stream. We do not own this output stream, so
  /// the creator must close and delete it.
  std::ostream* output_stream_;

}; // end class raw_descriptor_writer_interface


} // end namespace raw_descriptor_writer
} // end namespace vidtk

#endif /* _vidtk_raw_descriptor_writer_interface_h_ */
