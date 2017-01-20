/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_RAW_DESCRIPTOR_WRITER_H_
#define _VIDTK_RAW_DESCRIPTOR_WRITER_H_

#include <tracking_data/io/raw_descriptor_writer_interface.h>

#include <utilities/config_block.h>

#include <map>
#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>


namespace vidtk {

// ----------------------------------------------------------------
/**
 * \brief Raw descriptor writer.
 *
 * This class implements the raw descriptor writer for collection of
 * file formats.
 *
 * The following sequence diagram shows how this writer is used in a
 * process context. Use this as a guide when using the writer in other
 * contexts.
 *
 * \msc
proc, writer, writer_inst;
proc->writer [ label = "<<create>>" ];
writer=>writer_inst [ label = "<<create>>" ];
writer=>writer [ label = "add_writer(csv)" ];
writer=>writer_inst [ label = "<<create>>" ];
writer=>writer [ label = "add_writer(etc)" ];
proc<-writer;

proc=>writer [ label = "get_writer_names()" ];
--- [ label = "config selects writer type" ];
proc=>writer [ label = "set format(type)" ];
proc=>writer [ label = "initialize(stream)" ];
writer=>writer_inst [ label = "initialize(stream)" ];

proc=>writer [ label = "write()" ];
writer=>writer_inst [ label = "write()" ];

--- [ label = "when all descriptors are processed" ];
proc=>writer [ label = "finalize()" ];
writer=>writer_inst [ label = "finalize()" ];

--- [ label = "close output stream" ];
 * \endmsc
 */
class raw_descriptor_writer
  : private boost::noncopyable
{
public:

  // Calling this a handle to differentiate from a regular pointer and
  // from a vbl pointer.
  typedef boost::shared_ptr< ns_raw_descriptor_writer::raw_descriptor_writer_interface > handle_t;

  raw_descriptor_writer();
  virtual ~raw_descriptor_writer();

  /**
   * \brief Select output format.
   *
   * This method selects the output format to be used for writing the
   * raw events. \b False is returned if the requested format has not
   * been registered with thie writer.
   *
   * @param format Name of output format to use.
   *
   * @return \b true is returned if the format is recognised; \b false
   * otherwise.
   */
  bool set_format(std::string const& format);


  /**
   * \brief Add writer for specific format.
   *
   * This method adds specific output format raw descriptor writer to
   * the set of output formats available. Since these writers are
   * managed by shared pointers, a single concrete writer instance can
   * support multiple format types.
   *
   * Typical use of this function is as follows:
   \code
   rdw->add_writer( "csv", new raw_descriptor_writer_csv() );

   {
     // add one writer with multiple types.
     handle_t ptr = new raw_descriptor_writer_foo();
     rdw->add_writer( "foo", ptr );
     rdw->add_writer( "bar", ptr );
   }
   \endcode
   *
   * @param type Name of output format type. This may be the file
   * extension or some other descriptive word. It also may or may not
   * be the actual output file extension.
   *
   * Ownership of the writer object is transferred to this writer.
   *
   * @param writer Pointer to writer object.
   */
  void add_writer( std::string const& type, handle_t writer );


  /**
   * \brief Get list of implementation types.
   *
   * This method returns a string containing the config help texts for
   * all discovered writers.
   *
   * This list is useful for building configuration parameter
   * documentation and help text.
   *
   * @return Writer based descriptions for use in config parameter
   * description.
   */
  std::string const& get_writer_formats() const;

 /**
  * \brief Initialize selected writer.
  *
  * This call initializes the selected writer type. The writer will
  * format its output on the supplied stream. The stream is owned by
  * the caller so the caller is responsible for closing it when all
  * writing is done.
  *
  * @param str Stream to format output on
  *
  * @return \b true if initialization succeeded; \b false otherwise.
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
  void finalize();


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


  /**
   * \brief Write set of descriptors.
   *
   * This method writes each descriptor in the collection to the
   * stream.
   *
   * @param desc Collection of descriptors to write.
   *
   * @return \b true if write completed o.k.; \b false otherwise.
   */
  bool write( raw_descriptor::vector_t const& desc );


  /**
   * \brief Write single descriptor.
   *
   * This method writes a single descriptor to the stream.
   *
   * @param desc Descriptor to write.
   *
   * @return \b true if write completed o.k.; \b false otherwise
   */
  bool write( descriptor_sptr_t desc );


private:
  handle_t get_writer( std::string const& tag );

  handle_t m_raw_writer;          // currently active writer
  std::string m_writer_formats;
  config_block m_plugin_config;

  std::map< std::string, handle_t > m_implementation_list;

}; // end class raw_descriptor_writer

} // end namespace vidtk

#endif /* _VIDTK_RAW_DESCRIPTOR_WRITER_H_ */
