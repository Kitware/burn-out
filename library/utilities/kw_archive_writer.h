/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _KW_ARCHIVE_WRITER_H_
#define _KW_ARCHIVE_WRITER_H_

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>

#include <vcl_string.h>
#include <vcl_vector.h>
#include <vul/vul_file.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>
#include <vcl_fstream.h>

//
// Partial types.
//
class vsl_b_ostream;

namespace vidtk
{

// ----------------------------------------------------------------
/** KW Archive format writer.
 *
 * This class is the base class for all applications writing the KW
 * Archive format.
 *
 * There are two variants depending on what data types are available.
 */
class kw_archive_writer
{
public:
  typedef vcl_vector< vnl_vector_fixed< double, 2 > > corners_t;
  enum { KWA_FORMAT = 2 } ; // format version number for archive.

  //
  // frame related data - these data are written to the archive.
  //
  class data_set {
  public:
    vidtk::timestamp                 m_timestamp;
    double                           m_gsd;
    vidtk::image_to_image_homography m_src_to_ref_homog;
    vil_image_view< vxl_byte >       m_image;

    virtual void clear()
    {
      m_timestamp = vidtk::timestamp();
      m_gsd = 0;
      m_image = vil_image_view < vxl_byte >();
      m_src_to_ref_homog.set_invalid();
    }

  protected:
    data_set() { clear(); } // CTOR

  };


  // ----------------------------------------------------------------
/** Data set containing the src to utm homography.
 *
 * This class variant contains the src to utm homography for those
 * cases where the corner points are not directly available.  The
 * corner points are calcualted using this homography.
 */

  class data_set_homog : public data_set
  {
  public:
    vidtk::image_to_utm_homography   m_src_to_utm_homog;

    data_set_homog() { }

    virtual void clear()
    {
      m_src_to_utm_homog.set_invalid();
      data_set::clear();
    }
  };


// ----------------------------------------------------------------
/** Data set containing corner points.
 *
 * This class variant contains the four image corner points in
 * lat/lon. The corner points are ordered from upper left,
 * clockwise. (i.e. ul, ur, ll, lr)
 *
 * A lat/lon pair is represented as a vector [2] of doubles.
 */

  class data_set_corners : public data_set
  {
  public:
    corners_t corners;

    data_set_corners() { }

    virtual void clear()
    {
      corners.clear();
      data_set::clear();
    }

    void add_corner(vnl_vector_fixed< double, 2 > latlon)
    {
      corners.push_back (latlon);
    }

  };


  // -- CONSTRUCTOR --
  kw_archive_writer(); // CTOR
  virtual ~kw_archive_writer();

  /** Specify the directory and optional file base for generated files.
   * @param[in] output_path - directory where files will be
   * created. This can be either a full path or a relative path.
   * @param[in] base_name - base name for all created files.
   * @param[in] overwrite - set to allow overwriting of existing files.
   * @retval true - files open and ready to proceed.
   * @retval false - could not open files.
   */
  virtual bool set_up_files(vcl_string const& output_path, vcl_string const& base_name, bool overwrite = false);
  void close_files();

//@{
  /** Output path. The output path specifies the directroy where the
   * files are to be written.
   */
  vcl_string output_path() const { return ( this->m_output_path ); }
  kw_archive_writer & output_path(vcl_string v) { this->m_output_path = v; return ( *this ); }
//@}

//@{
  /** Base file name. The base file name is the main portion of the
   * file name used to identify the set of files created. A set of
   * files comprising the archive is created each with the same base
   * name and different extensions.
   */
  vcl_string base_name() const { return ( this->m_base_name ); }
  kw_archive_writer & base_name(vcl_string v) { this->m_base_name = v; return ( *this ); }
//@}


  vcl_string mission_id() const { return ( this->m_mission_id ); }
  kw_archive_writer & mission_id(vcl_string v) { this->m_mission_id = v; return ( *this ); }

//@{
/** Write data set to the archive. This method writes the supplied
 * data set to the archive.
 * @param[in] data - address of the data set to write.
 */
  void write_frame_data(data_set_homog const& data);
  void write_frame_data(data_set_corners const& data);
//@}

private:
  void write_frame_data(data_set const& data, corners_t const& corners);
  void calculate_frame_corner_points( kw_archive_writer::data_set_homog const& data,  corners_t & corners);


  vcl_ofstream* m_archive_stream;
  vcl_ofstream* m_meta_stream;
  vcl_ofstream* m_index_stream;

  vsl_b_ostream* m_archive;
  vsl_b_ostream* m_meta;

  vcl_string m_output_path;
  vcl_string m_base_name;
  vcl_string m_mission_id;

  vcl_string m_archive_path;
  vcl_string m_meta_path;
  vcl_string m_index_path;

  // used to cache previous lat/lon values, in case this
  // particular homography is bad
  corners_t prior_lat_lons; // order: lat/lon CW from UL

}; // end class kw_archive_writer

} // end namespace

#endif /* _KW_ARCHIVE_WRITER_H_ */
