/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDEO_MODALITY_READER_WRITER_H_
#define _VIDEO_MODALITY_READER_WRITER_H_

#include <utilities/base_reader_writer.h>

#include <utilities/video_modality.h>
#include <iomanip>


namespace vidtk
{


// ----------------------------------------------------------------
/** Video modality writer/reader
 *
 *
 */
class video_modality_reader_writer
  : public base_reader_writer_T < video_modality >
{
public:
  video_modality_reader_writer(video_modality * obj)
    : base_reader_writer_T < video_modality > ("VMODE", obj)
  { }

  virtual base_reader_writer* clone() const { return new video_modality_reader_writer(*this); }

  virtual ~video_modality_reader_writer() { }

// ----------------------------------------------------------------
/** Write modailty object to stream.
 *
 *
 */
  virtual void write_object(std::ostream& str)
  {
    str << this->entry_tag_string() << " "
        << vidtk::video_modality_to_string (*datum_addr())
        << std::endl;
  }


 // ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(std::ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  video-modality"
        << std::endl;
  }


// ----------------------------------------------------------------
/** Read metadata object from stream.
 *
 * Read an object from the stream expecting the format from the
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
    std::string mode;

    set_valid_state (false);

    str >> input_tag
        >> mode;

    // Test for stream error
    if (str.fail())
    {
      return (1);
    }

    // Convert string back into enum
    if ("Common" == mode) *datum_addr() = (VIDTK_COMMON);
    else if ("EO_MFOV" == mode) *datum_addr() = (VIDTK_EO_M);
    else if ("EO_NFOV" == mode) *datum_addr() = (VIDTK_EO_N);
    else if ("IR_MFOV" == mode) *datum_addr() = (VIDTK_IR_M);
    else if ("IR_NFOV" == mode) *datum_addr() = (VIDTK_IR_N);
    else if ("EO_FALLBACK" == mode) *datum_addr() = (VIDTK_EO_FB);
    else if ("IR_FALLBACK" == mode) *datum_addr() = (VIDTK_IR_FB);
    else
    {
      return (1);
    }

    set_valid_state (true);
    return (0);
  }


}; // end class video_modality_reader_writer

} // end namespace

#endif /* _VIDEO_MODALITY_READER_WRITER_H_ */
