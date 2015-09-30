/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _IMAGE_VIEW_READER_WRITER_H_
#define _IMAGE_VIEW_READER_WRITER_H_

#include <utilities/base_reader_writer.h>

#include <logger/logger.h>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vcl_fstream.h>
#include <vcl_iomanip.h>
#include <vil/io/vil_io_image_view.h>
#include <vil/vil_image_view.h>
#include <vsl/vsl_binary_io.h>
#include <vul/vul_file.h>


namespace vidtk
{
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __image_view_reader_writer_logger__
  VIDTK_LOGGER("image_view_reader_writer");

// ----------------------------------------------------------------
/** Image view readedr writer.
 *
 * This class writes and reads a vil_image_view<T> for a specific type
 * T.  The default stream is written with the filename and offset into
 * that file of the image written. It is unfortunate that an
 * additional file is required, but it is impractical to write a large
 * binary object (in ASCII) to the default stream.  When written, the
 * stream entry always contains the same file name that was supplied
 * to the constrctor.  When read, the file name is taken from the
 * default stream.
 *
 * The type of the image view is fixed, rather than being a template
 * parameter because the tag type must be specifically tied to a known
 * image type.  If additional pixel types are needed, this can be
 * refactored into templates and instantiators, but for now, with only
 * one pixel type needed, we do this.
 */
  template <class T>
class image_view_reader_writer
  : public base_reader_writer_T < vil_image_view < T > >
{
public:
  typedef vil_image_view < T > image_t;

  image_view_reader_writer(image_t * obj, vcl_string const& filename)
    : base_reader_writer_T < image_t > ("VIVIEW", obj),
      m_archive_stream(0),
      m_archive(0),
      m_first_write(true)
  {
    m_image_filename = vul_file::basename (filename);
    m_image_filename = vul_file::strip_extension (m_image_filename);
  }


  image_view_reader_writer(image_t * obj)
    : base_reader_writer_T < image_t > ("VIVIEW", obj),
      m_archive_stream(0),
      m_archive(0),
      m_first_write(true)
  {
    m_image_filename = "default";
  }


  virtual base_reader_writer* clone() const { return new image_view_reader_writer(*this); }


  virtual ~image_view_reader_writer()
  {
    delete m_archive;
    m_archive = 0;

    if (m_archive_stream) m_archive_stream->close();
    delete m_archive_stream;
    m_archive_stream = 0;
  }


// ----------------------------------------------------------------
/** Get image file name.
 *
 * This method returns the name of the file where the image data is
 * written.
 */
  vcl_string const& get_image_file_name() const { return m_image_filename; }


// ----------------------------------------------------------------
/** Set file name for image file.
 *
 * This method sets the base name for the image data file. Whatever is
 * specified will have ".imgdata" appended.
 *
 * @param[in] name - base name for the image data file
 */
  void set_image_file_name (vcl_string const & name)
  {
    m_image_filename =  vul_file::basename (name);
    m_image_filename = vul_file::strip_extension (m_image_filename);
}


// ----------------------------------------------------------------
/** Write data header.
 *
 * This header line indicates the data values in the line.
 */
  virtual void write_header(vcl_ostream & str)
  {
    str << "# " << this->entry_tag_string()
        << "  offset  file-name"
        << vcl_endl;
  }


// ----------------------------------------------------------------
/** Write image to stream.
 *
 *
 */
  virtual void write_object(vcl_ostream& str)
  {
    // Open file and create binary stream on first write.  This is
    // delayed until this point so that we will not create a new file
    // if one of these objects is instantiated only for writing.
    if (m_first_write)
    {
      // Open file
      open_file_for_output();

      m_first_write = false;
    }

    const vcl_streampos pos ( this->m_archive_stream->tellp() );

    this->m_archive->clear_serialisation_records();
    vsl_b_write( *this->m_archive, *this->datum_addr() );

    // Write index of binary image
    str << this->entry_tag_string() << " "
        << pos << " " << m_image_filename
        << vcl_endl;

    this->m_archive_stream->flush();
  }


// ----------------------------------------------------------------
/** Read object from stream.
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
 * @throws vcl_runtime_error - if format error.
 */
 virtual int read_object(vcl_istream& str)
  {
    vcl_string input_tag;
    vcl_string filename;
    vxl_int_64 offset;

    this->set_valid_state (false);

    str >> input_tag
        >> offset
        >> filename;

    if (str.fail() )
    {
      return 1;
    }

    //
    // 1 read image from data file.
    //
    // We have two options here. We can assume that the current file
    // (m_image_filename) is the same as the file name we just read and seek
    // to offset in that file, -or-
    // We can open the specified file, seek, read, close.
    //
    // The latter is safer, but more time consuming.
    //
    vcl_ifstream file_stream (filename.c_str(), vcl_ifstream::binary);
    if ( ! file_stream )
    {
      // throw ??
      LOG_ERROR ("Could not open archive file: " << filename);
      return 1;
    }

    vsl_b_istream bstream (&file_stream);

    file_stream.seekg( offset );
    if(file_stream.fail() )
    {
      LOG_ERROR ("Failed to seek to position " << offset);
      return 1;
    }

    bstream.clear_serialisation_records();
    vsl_b_read( bstream, *this->datum_addr() );

    if(file_stream.fail() )
    {
      LOG_ERROR ("Failed to read data from position " << offset );
      return 1;
    }

    this->set_valid_state (true);

    return 0;
  }


protected:
  bool open_file_for_output()
  {
    // Open file
    m_archive_stream = new vcl_ofstream;

    // Create file name
    if ( ! this->get_instance_name().empty())
    {
      m_image_filename += "_" + this->get_instance_name();
    }

    m_image_filename += ".imgdata"; // add extension

    m_archive_stream->open(m_image_filename.c_str(), vcl_ofstream::out
                           | vcl_ofstream::binary
                           | vcl_ofstream::trunc);

    if ( ! m_archive_stream )
    {
      // throw?
      LOG_ERROR ("Could not open archive file: " << m_image_filename);
      return false;             // error return
    }

    m_archive = new vsl_b_ostream(m_archive_stream);

    return true;
  }


private:
  vcl_string m_image_filename;
  vcl_ofstream* m_archive_stream;
  vsl_b_ostream* m_archive;

  bool m_first_write;


}; // end class image_view_reader_writer

#undef VIDTK_DEFAULT_LOGGER

} // end namespace

#endif /* _IMAGE_VIEW_READER_WRITER_H_ */
