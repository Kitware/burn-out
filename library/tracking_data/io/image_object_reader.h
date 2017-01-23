/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _IMAGE_OBJECT_READER_H_
#define _IMAGE_OBJECT_READER_H_

#include <string>
#include <vector>
#include <tracking_data/image_object.h>
#include <utilities/timestamp.h>
#include <tracking_data/io/image_object_reader_interface.h>
#include <boost/shared_ptr.hpp>


namespace vidtk {

// ----------------------------------------------------------------
/** Image object file reader.
 *
 * This class converts image object data from a disk into the memory
 * resident structures.
 *
 * The intent of this class is that an object is instantiated with the
 * filename each time a file needs to be read. There is explicitly no
 * close() method. The object is not to be reused. When done reading
 * the file, delete the reader and make a new one.
 *
 * There are many file formats that are handled internally. If you
 * have a custom file format that you need supported, add the reader
 * object immediately after creating a reader object.
 *
 * Example:
 \code
image_object_reader the_reader(filename);
image_object_reader::object_vector_t image_objects;

// Optionally add custom reader objects here.
// Allocate readers from heap since they will be transferred to
// the object_reader.
the_reader.add_reader( new image_object_reader_foo() );
// the_reader now owns foo_reader and will delete the object when done.

if (! the_reader.open())
{
    LOG_ERROR("Could not open file " << filename);
}

size_t count = the_reader.read_all(image_objects);
\endcode
*
\msc
App, image_object_reader, format_1_reader, format_2_reader;

App=>image_object_reader [ label="<<create>>" ];
image_object_reader=>format_1_reader [ label="<<create>>" ];
image_object_reader=>image_object_reader [ label="add_reader(reader 1)" ];
image_object_reader=>format_2_reader [ label="<<create>>" ];
image_object_reader=>image_object_reader [ label="add_reader(reader 2)" ];

... [ label="later" ];
App=>image_object_reader [ label="Open()" ];
image_object_reader=>format_1_reader [ label="open()" ];
image_object_reader<<format_1_reader [ label="return(false)" ];
image_object_reader=>format_2_reader [ label="open()" ];
image_object_reader<<format_2_reader [ label="return(true)" ];

App=>image_object_reader [ label="read_all()" ];
image_object_reader=>format_2_reader [ label="read_all()" ];
\endmsc
*/

class image_object_reader
{
public:
  typedef std::vector < ns_image_object_reader::datum_t > object_vector_t;
  typedef std::pair< vidtk::timestamp, std::vector< vidtk::image_object_sptr > > ts_object_vector_t;

  image_object_reader(std::string const& filename);
  virtual ~image_object_reader();

  /**
   * @brief Try to open file.
   *
   * The open method tries all registered readers to see if the file
   * format is recognised.
   *
   * @retval TRUE - file is opend and ready to read.
   * @retval FALSE - file format not recognised.
   */
  bool open();

  /**
   * @brief Read all objects from the file.
   *
   * All objects from the file are read and appended to the upplied
   * vector.
   *
   * @param datum Vector to receive objects from file.
   * @return The number of objects read is returned
   */
  size_t read_all(object_vector_t & datum);


  /**
   * @brief Read all image objects from file.
   *
   * A vector of all timestamp and image object vectors is returned.
   *
   * @param[out] datum Vector of timestamped
   */
  void read_all(std::vector < ns_image_object_reader::ts_object_vector_t > & datum);


  /**
   * @brief Read next object from the file.
   *
   * The next object is read from the file. False is returned if there
   * are no more obejcts in the file.
   *
   * @param datum Caller allocated area for object from file.
   *
   * @return False is returned if there are no more objects in the
   * file. The value in \c datum is undefined if false is returned.
   * True is returned if an object is returend from the file.
   */
  bool read_next(ns_image_object_reader::datum_t& datum);

  /**
   * @brief Read all objects for next timestamp.
   *
   * This method returns the list of image objects for the next
   * timestamp found in the input file. The timestamp and list of
   * objects are returned through the parameter.
   *
   * @note This method has persistent state information. If you start
   * reading image objects with this method, do not call any other
   * read methods; the results are unspecified.
   *
   * @param[out] datum Image objects for next timestamp
   *
   * @return \b true of data is valid. \b false if end of file.
   */
  bool read_next_timestamp( ts_object_vector_t& datum );

  /**
   * @brief Add new reader to list of supported formats.
   */
  void add_reader(ns_image_object_reader::image_object_reader_interface* reader);

private:
  typedef boost::shared_ptr < ns_image_object_reader::image_object_reader_interface > reader_handle_t;
  std::vector < reader_handle_t > reader_list_;
  std::string filename_;
  reader_handle_t current_reader_;

  ns_image_object_reader::datum_t rnt_last_read_;

}; // end class image_object_reader

} // end namespace

#endif /* _IMAGE_OBJECT_READER_H_ */
