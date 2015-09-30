/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDEO_METADATA_VECTOR_READER_WRITER_H_
#define _VIDEO_METADATA_VECTOR_READER_WRITER_H_


#include <utilities/base_reader_writer.h>

#include <utilities/video_metadata.h>
#include <vcl_iomanip.h>
#include <vcl_vector.h>


namespace vidtk
{

// ----------------------------------------------------------------
/** Video metadata reader/wrtiter.
 *
 * @todo need test for this class.
 */
class video_metadata_vector_reader_writer
  : public base_reader_writer_T < vcl_vector < video_metadata > >
{
public:
  video_metadata_vector_reader_writer(vcl_vector < video_metadata > * obj)
    : base_reader_writer_T < vcl_vector < video_metadata > > ("VMDVEC", obj)
  { }

  virtual base_reader_writer* clone() const { return new video_metadata_vector_reader_writer(*this); }

  virtual ~video_metadata_vector_reader_writer() { }


// ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(vcl_ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  element-count timeUTC platform-loc-lat platform-loc-lon "
        << " plat-alt plat-roll plat-pitch plat-yaw"
        << " sensor-roll sensor-pitch sensor-yaw"
        << " [corner points ul, ur, lr, ll / lat,lon]"
        << " slant-range sens-horiz-fov sens-vert-fov"
        << " frame center lat/lon"
        << vcl_endl;
  }


// ----------------------------------------------------------------
/** Write a metadata object to stream.
 *
 *
 */
  virtual void write_object(vcl_ostream& str)
  {
    str << this->entry_tag_string() << " "
        << datum_addr()->size() << " ";

    // Use serialization method for now. this means we do not have
    // total control over the format though.
    for (unsigned int i = 0; i < datum_addr()->size() ; ++i)
    {
      (*datum_addr())[i].ascii_serialize (str);
      str << " ";
    } // end for

    str << vcl_endl;
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
 *
 * @TODO This really should be a more robust reader that can tell of
 * the input is in the expected format.
 */
 virtual int read_object(vcl_istream& str)
  {
    vcl_string input_tag;
    unsigned int count;

    set_valid_state (false);

    str >> input_tag // consume the tag
        >> count; // number of elements in vector

    for (unsigned int i = 0; i < count; ++i)
    {
      vidtk::video_metadata vmd;
      vmd.ascii_deserialize (str);

      datum_addr()->push_back(vmd);

      // Test for stream error
      if (str.fail())
      {
        return 1;
      }
    } // end for


    set_valid_state (true);
    return 0;
  }

}; // end class video_metadata_vector_reader_writer

} // end namespace

#endif /* _VIDEO_METADATA_VECTOR_READER_WRITER_H_ */
