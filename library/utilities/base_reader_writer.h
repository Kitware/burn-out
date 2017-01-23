/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _BASE_READER_WRITER_H_
#define _BASE_READER_WRITER_H_

#include <string>
#include <ostream>
#include <istream>


namespace vidtk
{

// ----------------------------------------------------------------
/** Reads and writes one data type.
 *
 * This class represents an abstract base class that contains a
 * matched set of reader/writer that can store and retrieve things
 * to/from a stream.
 *
 */
class base_reader_writer
{
public:
  /** Constructor. The tag specified with this constructor is used to
   * identify the type of data to be handled. A ':' is always apended
   * to the name as a terminator/separator, and is therefore not
   * allowed in a tag name. Blanks are not allowed either. If you have
   * a group where you need to write more than one instance of data
   * type must be handled, use the set_instance_name() method to
   * supply a unique instance name for the data reader/writer objects.
   *
   * An instance name is appended to the tag name when a data item is
   * written or read, creating an extended tag that can be seen in the
   * data file.
   *
   * @param[in] tag - tag representing data type handled.
   *
   * @sa set_instance_name
   */
  base_reader_writer (std::string const& tag);

  virtual base_reader_writer* clone() const = 0;

  virtual ~base_reader_writer()
  { }

  /** Add instance name to reader/writer. The specified instance name
   * is appended to the current data reader/writer tag string. A ':'
   * is always appended to the instance name as a
   * terminator/separator, and is therefore not allowed in a tag
   * name. Blanks are not allowed either. It is possible to generate
   * highly structured names by adding multiple instance names. this
   * may be useful if you are manually combining data files. Then
   * again, it may not.
   *
   * @param[in] name - instance name to append.
   *
   * @return The fully amended tag name is returned.
   */
  std::string const& add_instance_name (std::string const& name);
  std::string const& get_instance_name () const { return m_instanceName; }

  std::string const& entry_tag_string() const;

// ----------------------------------------------------------------
/** Verify tag at head of stream.
 *
 * This method verifies that the tag at the head of the stream is or
 * is not recognized by this data reader. The stream position is not
 * altered by this method, meaning it still points to the start of the
 * tag.
 *
 * @param[in] str - stream to read from
 *
 * @retval true - tag recognised
 * @retval false - tag not recognised
 */
  virtual bool verify_tag(std::istream & str) const;

  virtual void write_object(std::ostream & str) = 0;
  virtual void write_header(std::ostream & str) = 0;

  /** Read object from stream.
   *
   * @return The number of objects not read. So return zero (0) for
   * successful reads from methods that read one object.
   */
  virtual int read_object(std::istream & str) = 0;

  /** Indicates valid input read.  This predicate indicates that valid
   * datum has been read.
   */
  bool is_valid() const { return m_validEntry; }
  void set_valid_state (bool v) { m_validEntry = v; }


protected:


private:
  std::string m_entryTag;
  std::string m_instanceName;
  bool m_validEntry;

}; // end class base_reader_writer



// ----------------------------------------------------------------
/** Base reader/writer class for a specific type.
 *
 * This class represents an abstract base class that contains a
 * matched set of reader/writer that can store and retrieve specific
 * data types to/from a stream.
 *
 * Both parts of the process (read and write) are contained in the
 * same class so any format issues can be readily recognized and
 * remedied.
 *
 * @tparam T - type of data to manage on a stream.
 */
template < class T >
class base_reader_writer_T
  : public base_reader_writer
{
public:
  base_reader_writer_T(std::string const& tag, T * obj)
    : base_reader_writer (tag),
      m_objectData(obj)
  { }

  virtual ~base_reader_writer_T()
  { m_objectData = 0; }

  /** Reset pointer to datum to read/write
   */
  virtual void set_datum_addr ( T * ptr) { m_objectData = ptr; }

protected:
  virtual T * datum_addr() { return m_objectData; }


private:
  // pointer to storage for read/write operation
  T * m_objectData;

}; // end class base_reader_writer_T

} // end namespace

#endif /* _BASE_READER_WRITER_H_ */
