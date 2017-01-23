/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_video_metadata_vector_reader_writer_h_
#define vidtk_video_metadata_vector_reader_writer_h_

#include <utilities/base_reader_writer.h>

#include <utilities/video_metadata.h>
#include <utilities/video_metadata_util.h>

#include <iomanip>
#include <vector>


namespace vidtk
{

// ----------------------------------------------------------------
/** Video metadata reader/wrtiter.
 *
 * @todo need test for this class.
 */
class video_metadata_vector_reader_writer
  : public base_reader_writer_T < std::vector < video_metadata > >
{
public:
  video_metadata_vector_reader_writer(std::vector < video_metadata > * obj)
    : base_reader_writer_T < std::vector < video_metadata > > ("VMDVEC", obj)
  { }

  virtual base_reader_writer* clone() const { return new video_metadata_vector_reader_writer(*this); }

  virtual ~video_metadata_vector_reader_writer() { }


// ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(std::ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  element-count timeUTC platform-loc-lat platform-loc-lon "
        << " plat-alt plat-roll plat-pitch plat-yaw"
        << " sensor-roll sensor-pitch sensor-yaw"
        << " [corner points ul, ur, lr, ll / lat,lon]"
        << " slant-range sens-horiz-fov sens-vert-fov"
        << " frame center lat/lon"
        << std::endl;
  }


// ----------------------------------------------------------------
/** Write a metadata object to stream.
 *
 *
 */
  virtual void write_object(std::ostream& str)
  {
    str << this->entry_tag_string() << " "
        << datum_addr()->size() << " ";

    // Use serialization method for now. this means we do not have
    // total control over the format though.
    for (unsigned int i = 0; i < datum_addr()->size() ; ++i)
    {
      ascii_serialize (str, (*datum_addr())[i]);
      str << " ";
    } // end for

    str << std::endl;
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
 * @todo This really should be a more robust reader that can tell of
 * the input is in the expected format.
 */
 virtual int read_object(std::istream& str)
  {
    std::string input_tag;
    unsigned int count;

    set_valid_state (false);

    str >> input_tag // consume the tag
        >> count; // number of elements in vector
    if(str.fail())
    {
      return 1;
    }

    for (unsigned int i = 0; i < count; ++i)
    {
      vidtk::video_metadata vmd;
      ascii_deserialize (str, vmd);

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

#endif /* vidtk_video_metadata_vector_reader_writer_h_ */
