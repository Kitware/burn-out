/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "group_data_reader_writer.h"

#include <iostream>
#include <boost/foreach.hpp>
#include <logger/logger.h>


namespace vidtk
{

  VIDTK_LOGGER("group_data_reader_writer");

//
// Define the default policy
//
class def_policy_t
  : public group_data_reader_writer::mismatch_policy
{
public:
  virtual ~def_policy_t() { }

  // Allow all unrecognized input
  virtual bool unrecognized_input (const char * /*line*/) { return true; }

  // Do not allow group members to be not satisfied.
  virtual size_t missing_input (group_data_reader_writer::rw_vector_t const& objs) { return objs.size(); }
};

static def_policy_t default_policy;

static const char s_group_marker[] = "#(@)--> start of group <--";  // group start marker


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
group_data_reader_writer
::group_data_reader_writer()
  : base_reader_writer("group"),
  m_currentPolicy ( &default_policy )
{
}


group_data_reader_writer
::group_data_reader_writer ( mismatch_policy * policy )
  : base_reader_writer("group"),
    m_currentPolicy ( policy )
{
}


// ----------------------------------------------------------------
group_data_reader_writer
::~group_data_reader_writer()
{
  BOOST_FOREACH ( base_reader_writer * obj, m_objectList )
  {
    delete obj;
  }
}


// ----------------------------------------------------------------
/** Set policy object
 *
 *
 */
void group_data_reader_writer
::set_policy(mismatch_policy * policy)
{
  this->m_currentPolicy = policy;
}


// ----------------------------------------------------------------
void group_data_reader_writer
::write_object(std::ostream& str)
{
  str << s_group_marker << "\n";  // group start marker

  BOOST_FOREACH ( base_reader_writer * obj, m_objectList )
  {
    obj->write_object(str);
  }
}


 // ----------------------------------------------------------------
void group_data_reader_writer
::write_header(std::ostream & str)
{
  BOOST_FOREACH ( base_reader_writer * obj, m_objectList )
  {
    obj->write_header(str);
  }
}


// ----------------------------------------------------------------
/* Read a group of objects.
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
 * or -1 for end of file.
 */
int group_data_reader_writer
::read_object(std::istream& str)
{
  int full_count(0); // total number of objects read
  int skip_error_count(0);
  bool marker_seen(false);

  //
  // We need to take into account the situation where the data was
  // written in a different order than we are reading it.
  //
  rw_vector_t local_list = this->m_objectList;

  // loop over all elements in our collection.  This allows each one
  // to accept an input line.
  full_count = this->m_objectList.size();
  for (int i = 0; i < full_count; i++)
  {
    // skip over uninteresting stuff
    int type = skip_comment (str);
    if (-1 == type) // io error - eof
    {
      return -1;
    }

    // Check for group boundry
    // If we see a marker on the first element, then it is starting the group.
    // If we see the marker on any other element, then this is an end of group.
    if ( (1 == type) && (i > 0) )
    {
      marker_seen = true;
      break;
    }

    // Loop over all entries to see which one recognises the current
    // input line. Loop until line is recognised or run out of input
    // objects.
    rw_iterator_t ix;
    bool valid_read(false);

    for (ix = local_list.begin(); ix != local_list.end(); ++ix)
    {
      base_reader_writer * obj = *ix;

      valid_read = obj->verify_tag(str) && (obj->read_object(str) == 0);

      if (valid_read)
      {
        // input was recognised.
        local_list.erase (ix);
        break;
      }
    } // end for ix

    // here because recognised input or end of list
    if ( ! valid_read )
    {
      // end of loop indicates unrecognized input tag
      if (! skip_line(str))
      {
        skip_error_count++;
      }
    }

  } // end for i

  // We have satisfied all elements in the group, but there may still
  // be unaccepted input in the stream. This is the case if we have
  // not seen the marker.
  if ( ! marker_seen)
  {
    // flush until we see group marker
    std::string line;
    while (find_group_start (str, line) == 1)
    {
      m_currentPolicy->unrecognized_input (line.c_str());
    }
  }

  if ( ! local_list.empty())
  {
    // The elements in the list are those that did not recognise a tag
    // in the input.
    return m_currentPolicy->missing_input (local_list);
  }

  return local_list.size() + skip_error_count;
}


// ----------------------------------------------------------------
/** Verify tag at head of stream.
 *
 * This method verifies that the tag at the head of the stream is or
 * is not recognized by the current set of data readers.  Use this
 * method to recover from synchronization problems, such as there were
 * more objects written to the file than we are reading.
 *
 * @param[in] str - stream to read from
 *
 * @return true indicates a tag match. false indicates tag miss-match
 * or end of stream.
 */
bool group_data_reader_writer
::verify_tag(std::istream& str) const
{
  std::streampos pos = str.tellg();
  std::string tag;

  str >> tag;                   // read tag
  if (str.fail() )
  {
    return false;
  }

  LOG_TRACE ("Read tag \"" << tag << "\"");

  str.seekg(pos);               // rewind file

  // Run this up the flag pole and see who salutes.
  BOOST_FOREACH ( base_reader_writer * obj, this->m_objectList )
  {
    if (tag == obj->entry_tag_string())
    {
      LOG_TRACE ("Matched tag \"" << tag << "\"");
      return true;
    }
  }

  return false;
}


// ----------------------------------------------------------------
base_reader_writer* group_data_reader_writer
::add_data_reader_writer(base_reader_writer const& obj)
{
  base_reader_writer* new_obj = obj.clone();
  m_objectList.push_back(new_obj);
  return new_obj;
}


// ----------------------------------------------------------------
/** Return number of writers in the group.
 *
 *
 */
int group_data_reader_writer
::size() const
{
  return this->m_objectList.size();
}


// ----------------------------------------------------------------
/** Skip one line in the input stream.
 *
 *
 * @retval true - ok to ignore this line
 * @retval false - count this line as unhandled
 */
bool group_data_reader_writer
::skip_line(std::istream & str) const
{
  char buffer[4096]; // FIXME: Static buffer size.
  str.getline(buffer, sizeof buffer);

  // Pass to policy object
  return m_currentPolicy->unrecognized_input (buffer);
}


// ----------------------------------------------------------------
/** Skip comment and blank lines
 *
 * Skip blank lines and comments. In addition specific structured
 * comments are checked for and, if found, are noted in the return
 * code.
 *
 * @retval -1 - error reading
 * @retval 0 - comments skipped
 * @retval n > 0 - specific comment type
 * @retval 1 - start of group marker
 */
int group_data_reader_writer
::skip_comment(std::istream & str) const
{
  char c;
  char buffer[4096]; // FIXME: Static buffer size.

  for (;;)
  {
    str >> c;
    if (str.fail() )
    {
      return -1;
    }

    // If this is a comment line, skip until end of line.
    if (c == '#')
    {
      // look for specific comment markers
      str.seekg (-1, std::ios::cur);

      str.getline(buffer, sizeof buffer);
      if (strcmp (buffer, s_group_marker) == 0)
      {
        return 1;
      }

      continue;
    }

    if (!isspace(c))
    {
      break;
    }

  } // end for

  // Back up the stream one character
  str.seekg (-1, std::ios::cur);
  return 0;
}


// ----------------------------------------------------------------
/** Position stream at start of group marker.
 *
 * This method positions the stream at the start of the group
 * marker. If aline is found that is not a comment and not the marker,
 * it is returned.
 *
 * @retval -1 - error reading
 * @retval 1 - non comment, non marker line returned.
 * @retval 0 - start of group marker
 */
 int group_data_reader_writer
::find_group_start(std::istream & str, std::string & line)
{
  char c;
  char buffer[4096]; // FIXME: Static buffer size.

  for (;;)
  {
    std::streampos pos = str.tellg();

    str >> c;
    if (str.fail() )
    {
      return -1;
    }

    // skip junk
    // This stuff can be seen when a reader does not consume up to the
    // new-line that marks the end of its input.
    if (c < ' ')
    {
      continue;
    }
    str.seekg (-1, std::ios::cur);

    str.getline(buffer, sizeof buffer);
    if (str.fail() )
    {
      return -1;
    }

    if (strcmp (buffer, s_group_marker) == 0)
    {
      str.seekg(pos); // rewind file
      return 0;
    }

    line = buffer; // return the line
    return 1;

  } // end for

  // should never happen
  return -1;
}


} // end namespace
