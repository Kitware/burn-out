/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _GROUP_DATA_READER_WRITER_H_
#define _GROUP_DATA_READER_WRITER_H_

#include <utilities/base_reader_writer.h>
#include <vector>


namespace vidtk
{

// ----------------------------------------------------------------
/** \brief Reads and Writes a set of data types.
 *
 * This class represents a set of data elements that are to be
 * read/written to/from a stream.  The goal of this class is to
 * provide a framework to easily write and read sets of objects while
 * not enforcing any formatting constraints on these objects.
 *
 * An object derived from base_reader_writer (or base_reader_writer_T)
 * class is added to the group. These objects are copied into the
 * group, so they are no longer needed after they are added.
 *
 * Note: Since this class is derived from base_reader_writer, a
 * group_data_reader_writer object can be added to the list of objects
 * to handle. This gives you the flexibility to add predefined groups
 * as well as single objects.
 *
 * Usually the write operations will be done is a process (derived
 * from base_writer_process) in the pipeline. Reading can be easily
 * done from a stand-alone program by creating a
 * group_data_reader_writer that contains the desired reader/writer
 * objects.
 *
 * Example:
 \code

class my_reader : public group_data_reader_writer
{
public:
  my_reader()
  {
    // allocate writer objects pointing to input data fields
    gsd_reader_writer * gsd_rw = add_data_reader_writer (gsd_reader_writer( & m_gsd ) );
    gsd_rw->add_instance_name ("mumph");
    timestamp_reader_writer * ts_rw = add_data_reader_writer (timestamp_reader_writer ( & m_timestamp ) );
  }

  // target data areas filled in by read
  double m_gsd;
  vidtk::timestamp m_timestamp;
};



int main(int argc, char *argv[])
{

  my_reader the_reader();

  // open input stream
  std::istream = ...;

  while (0 == the_reader.read_object(str))
  {
  // m_gsd and m_timestamp now have the new values read from the
  // stream


  } // end while

  // could be read error or EOF
  if (str.eof())
  {
    // handle EOF
  }
  else
  {
    // handle read error
  }

  return 0;
}
  \endcode
 *
 *
 */
class group_data_reader_writer
  : public base_reader_writer
{
public:
  typedef std::vector < base_reader_writer * > rw_vector_t;
  typedef rw_vector_t::iterator rw_iterator_t;


  /** \brief Input file / object list mismatch.
   *
   * This abstract base class defines the interface that is used to
   * report mismatches between the input file and the list of
   * registered objects.
   */
  class mismatch_policy
  {
  public:
    virtual ~mismatch_policy() { }

    /** \brief Unrecognized input found.
     *
     * This method is called when a line is discovered in the input
     * stream that is not claimed by a reader object. A policy handler
     * could look for a specific tag value and perform some
     * operation. This will allow a tag to initiate an execution of
     * something.
     *
     * @param[in] line - input line, including tag, that is not
     * recognized by the group.
     *
     * @return True is returned to indicate that it is ok to ignore
     * this line. False indicates an error.
     */
    virtual bool  unrecognized_input (const char * line) = 0;

    /** \brief Missing input detected.
     *
     * This method is called when there are reader objects that have
     * not found input in the input stream.
     *
     * One example implementation could look for default values for
     * unsatisfied objects from an alternate source.
     *
     * @param[in] objs list of objects that have not received input.
     *
     * @return Number of objects that still do not have input. A zero
     * indicates that all objects have been satisfied.
     */
    virtual size_t missing_input (rw_vector_t const& objs) = 0;
  };



// ----------------------------------------------------------------
/** \brief Default constructor.
 *
 * This method creates a new object with the default policy for
 * handling mismatches between reader-writer objects and the data file
 * contents.
 */
  group_data_reader_writer();


// ----------------------------------------------------------------
/** \brief Constructor with policy.
 *
 * This method creates a new object with the specified policy object
 * for handling mismatches between reader-writer objects and the data
 * file contents.
 *
 * The caller is responsible for managing the storage for the policy
 * object.
 *
 * @param[in] policy - specific policy.
 */
  group_data_reader_writer(mismatch_policy * policy);

  virtual ~group_data_reader_writer();

  /** Return number of reader/writer objects in the list
   */
  int size() const;


// ----------------------------------------------------------------
/** \brief Set policy object.
 *
 * @param[in] policy - new policy to put into effect.
 */
  void set_policy(mismatch_policy * policy);


// ----------------------------------------------------------------
/** \brief Write data header.
 *
 * This method writes all headers to the stream. Each registered
 * object's write_header() method is called to generate a complete
 * list fo headers.
 *
 * @param[in] str - stream to accept the data.
 */
  virtual void write_header(std::ostream & str);


// ----------------------------------------------------------------
/** \brief Write a group of objects.
 *
 * This method writes the output from the list of reader/writer
 * objects. This is done by calling the write() method on each object.
 *
 * @param[in] str - stream to accept the data.
 */
  virtual void write_object(std::ostream & str);


// ----------------------------------------------------------------
/** \brief Read a group of objects.
 *
 * This method reads a group of objects from the stream. The added
 * complication is that the readers may not be called in the same
 * order as the writers. This method also handles situations where
 * there were more or fewer writers than readers.
 *
 * If there is an input line that is not recognised, it is passed to
 * the unrecognized_input() method of the current policy object.
 *
 * If there are input objects that have not found their input types,
 * they are passed to the missing_input() method of the current policy
 * object.
 *
 * @param[in] str - stream to read from
 *
 * @return The number of objects that did not find their input.
 */
 virtual int read_object(std::istream & str);


// ----------------------------------------------------------------
/** \brief Add data element IO handler.
 *
 * This method adds the specified data reader-writer object to the
 * group. The data elements are written in the order these writers are
 * added to the group, but that should not be a driving assumption,
 * since they may be reordered.
 *
 * Reader/writer objects are \b copied into the group so you can dispose
 * of the passed object at your convenience. This is done to simplify
 * the management of the objects in the collection. Passing a
 * temporary version of a reader/writer will also work.
 *
 * The address of the object being added is returned so that you can
 * use it to check for successful reads. This is generally not
 * necessary since the aggregate status is returned by the
 * read_object() method, but if you need to know explicitly which
 * reader failed, they can be checked individually. This is alos
 * useful for adding instance names.
 *
 * @param[in] obj - writer object to add
 *
 * @return The pointer to the object in the collection is returned.
 */
  base_reader_writer * add_data_reader_writer (base_reader_writer const& obj);


// ----------------------------------------------------------------
/** \brief Verify tag at head of stream.
 *
 * This method verifies that the tag at the head of the stream is or
 * is not recognized by the current set of data readers. The stream
 * position is not altered by this method.
 *
 * @param[in] str - stream to read from
 */
  virtual bool verify_tag(std::istream & str) const;



protected:
  bool skip_line(std::istream & str) const;
  int skip_comment(std::istream & str) const;
  int find_group_start(std::istream & str, std::string & line);


private:
  virtual base_reader_writer* clone() const { return 0; }

  rw_vector_t m_objectList;

  mismatch_policy *  m_currentPolicy;

  std::string m_commentBuffer;

}; // end class group_data_writer


} // end namespace

#endif /* _GROUP_DATA_READER_WRITER_H_ */
