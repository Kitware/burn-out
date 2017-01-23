/*ckwg +5
g * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _SHOT_BREAK_FLAGS_READER_WRITER_H_
#define _SHOT_BREAK_FLAGS_READER_WRITER_H_

#include <utilities/base_reader_writer.h>

#include <tracking_data/shot_break_flags.h>
#include <cstdio>
#include <iomanip>



namespace vidtk
{

// ----------------------------------------------------------------
/** shot_break_flags reader/writer class
 *
 * This class represents a matched set of reader/writer that can store
 * and retrieve a timestamp to/from a stream.
 *
 * Both parts of the process (read and write) are contained in the
 * same class so any format issues can be readily recognized and
 * remedied.
 */
  class shot_break_flags_reader_writer
    : public base_reader_writer_T < vidtk::shot_break_flags >
  {
  public:
    shot_break_flags_reader_writer(vidtk::shot_break_flags * obj)
      : base_reader_writer_T <vidtk::shot_break_flags >("PIPESTAT", obj)
    { }

    virtual base_reader_writer* clone() const { return new shot_break_flags_reader_writer(*this); }

    virtual ~shot_break_flags_reader_writer() { }


// ----------------------------------------------------------------
/** Write shot_break_flags to stream.
 *
 * The currently active shot_break_flags is written to the current stream.
 * A status entry is a "\n" terminated string that starts with "PIPESTAT:".
 *
 * @param[in] str - stream to format on
 */
  virtual void write_object(std::ostream& str)
  {
    str << this->entry_tag_string() << " "
        << datum_addr()->is_shot_end() << " "
        << datum_addr()->is_frame_usable() << " "
        << datum_addr()->is_frame_not_processed() << std::endl;
  }


// ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(std::ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  is-shot-end  is-frame-usable  is-frame-not-processed" << std::endl;
  }


// ----------------------------------------------------------------
/** Read shot_break_flags from stream.
 *
 * Read a shot_break_flags from the stream expecting the format from the
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
    bool shot_end;
    bool frame_usable;
    bool frame_not_proc;

    set_valid_state (false);

    str >> input_tag
        >> shot_end
        >> frame_usable
        >> frame_not_proc;

    // Test for stream error
    if (str.fail())
    {
      return 1;
    }

    datum_addr()->set_shot_end (shot_end);
    datum_addr()->set_frame_usable (frame_usable);
    datum_addr()->set_frame_not_processed (frame_not_proc);

    set_valid_state (true);
    return  0;
  }


  }; // end class shot_break_flags_reader_writer


} // end namespace

#endif /* _SHOT_BREAK_FLAGS_READER_WRITER_H_ */
