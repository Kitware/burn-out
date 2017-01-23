/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _TIMESTAMP_READER_WRITER_H_
#define _TIMESTAMP_READER_WRITER_H_

#include <utilities/base_reader_writer.h>

#include <utilities/timestamp.h>
#include <vul/vul_sprintf.h>
#include <cstdio>


namespace vidtk
{

// ----------------------------------------------------------------
/** Timestamp read/writer class
 *
 * This class represents a matched set of reader/writer that can store
 * and retrieve a timestamp to/from a stream.
 *
 * Both parts of the process (read and write) are contained in the
 * same class so any format issues can be readily recognized and
 * remedied.
 */

class timestamp_reader_writer
  : public base_reader_writer_T< vidtk::timestamp >
{
public:
  timestamp_reader_writer(vidtk::timestamp * obj)
    : base_reader_writer_T< vidtk::timestamp >("TS", obj)
  { }

  virtual base_reader_writer* clone() const { return new timestamp_reader_writer(*this); }

  virtual ~timestamp_reader_writer() { }


// ----------------------------------------------------------------
/** Write timestamp to stream.
 *
 * The currently active timestamp is written to the current stream.
 * A timestamp entry is a "\n" terminated string that starts with "TS:".
 *
 * @param[in] str - stream to format on
 */
  virtual void write_object(std::ostream& str)
  {
    str << this->entry_tag_string() << " "
        << datum_addr()->frame_number() << " "
        << std::setprecision(20)
        << datum_addr()->time() << std::endl;
  }


// ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(std::ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  frame-number time" << std::endl;
  }


// ----------------------------------------------------------------
/** Read timestamp from stream.
 *
 * Read a timestamp from the stream expecting the format from the
 * writer method.
 *
 * @param[in] str - stream to read from
 *
 * @return The number of objects not read in. This would be either 0
 * or 1, since we are only a single object reader.
 *
 * @retval 0 - object read correctly
 * @retval 1 - object not recognized
 */
  virtual int read_object(std::istream& str)
  {
    std::string input_tag;
    unsigned frame;
    double time;

    set_valid_state (false);

    str >> input_tag
        >> frame
        >> time;

    if (str.fail())
    {
      return 1;
    }

    datum_addr()->set_frame_number(frame);
    datum_addr()->set_time(time);

    set_valid_state (true);
    return  0;
  }

};   // end class timestamp_reader_writer

} // end namespace

#endif /* _TIMESTAMP_READER_WRITER_H_ */
