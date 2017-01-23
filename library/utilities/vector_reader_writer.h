/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VECTOR_READER_WRITER_H_
#define _VECTOR_READER_WRITER_H_

#include <utilities/base_reader_writer.h>

namespace vidtk
{


/** Vector reader writer.
 *
 * This class template writes and reads a fixed vector of one type.
 * @tparam T - element type for vector
 */
template < class T >
class vector_reader_writer
  : public base_reader_writer_T < T * >
{
public:
  /** Create new reader/writer.
   * @param[in] obj - address of first element of vector
   * @param[in] count - number of elements
   */
  vector_reader_writer(T * obj, size_t count)
    : base_reader_writer_T < video_metadata > ("VECTOR", obj)
  { }


  virtual base_reader_writer* clone() const { return new vector_reader_writer(*this); }

  virtual ~video_metadata_reader_writer() { }

// ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(std::ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << " elem-count  elem[0] elem[1] ..."
        << std::endl;
  }


// ----------------------------------------------------------------
/** Write a vector elements to stream.
 *
 *
 */
  virtual void write_object(std::ostream& str)
  {
    str << this->entry_tag_string() << " ";

    // write number of elelents
    str << this->count_;

    // Write all elements of vector
    for (size_t i = 0; i < this->count_; i++)
    {
      str << " " << datum_addr()[i];
    }

    str << std::endl;
  }


 // ----------------------------------------------------------------
/** Read vector elements from stream.
 *
 * Read an object from the stream expecting the format from the
 * writer method.
 *
 * @param[in] str - stream to read from
 *
 * @return The number of objects not read in. This would be either 0
 * or the number of elements.
 *
 * @retval 0 - object read correctly
 * @retval 1 - object not recognized
 */
 virtual int read_object(std::istream& str)
  {
    std::string input_tag;

    set_valid_state (false);

    str >> input_tag; // consume the tag

    str >> size_t count; // get number of elements to follow
    for (size_t i = 0; i < count; ++i)
    {
      str >> datum_addr()[i];
    } // end for

    // Test for stream error
    if (str.fail())
    {
      return count;
    }

    set_valid_state (true);
    return 0;
  }


private:
  size_t count_;

}; // end class vector_reader_writer

} // end namespace


#endif /* _VECTOR_READER_WRITER_H_ */
