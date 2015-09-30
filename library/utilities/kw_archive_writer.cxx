/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <utilities/kw_archive_writer.h>

#include <logger/logger.h>

#include <vbl/io/vbl_io_smart_ptr.h>
#include <vil/io/vil_io_image_view.h>
#include <vidl/vidl_convert.h>
#include <vnl/vnl_matrix_fixed.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/io/vnl_io_matrix_fixed.h>
#include <vnl/io/vnl_io_vector_fixed.h>
#include <vul/vul_file.h>
#include <geographic/geo_coords.h>


#include <vsl/vsl_vector_io.txx>
VSL_VECTOR_IO_INSTANTIATE( vnl_vector_fixed< double VCL_COMMA 2> );

namespace vidtk
{

VIDTK_LOGGER("kw_archive_writer");


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
kw_archive_writer::
kw_archive_writer()
  : m_archive_stream(0),
    m_meta_stream(0),
    m_index_stream(0),
    m_archive(0),
    m_meta(0)
{

  // Initialize prior values
  vnl_vector_fixed< double, 2 > latlon;
  latlon[0] = vidtk::video_metadata::lat_lon_t::INVALID;
  latlon[1] = vidtk::video_metadata::lat_lon_t::INVALID;

  for (unsigned i = 0; i < 4; ++i)
  {
    prior_lat_lons.push_back (latlon);
  }

}


kw_archive_writer::
~kw_archive_writer()
{
  close_files();
}


// ----------------------------------------------------------------
/** Set up file names and create them.
 *
 * This method creates all the necessary file names from the specified
 * base name and creates the files at the specified path location.
 *
 * @param[in] output_path - location to create files. Does not contain trailing '/'.
 * @param[in] base_name - base name for all archive files.
 *
 * @retval true - if success
 * @retval false - if error, files not open.
 */
bool kw_archive_writer::
set_up_files(vcl_string const& output_path, vcl_string const& base_name, bool allow_overwrite)
{
  m_output_path = output_path;
  m_base_name = base_name;

  if ( m_output_path.empty() )
  {
    m_output_path = ".";
  }

  m_archive_path = m_output_path + '/' + vul_file::basename(m_base_name)  + ".data";
  m_meta_path = m_output_path + '/' + vul_file::basename(m_base_name)  + ".meta";
  m_index_path = m_output_path + '/' + vul_file::basename(m_base_name) + ".index";

  // test for existing files
  if ( ! allow_overwrite)
  {
    if (vul_file::exists (m_archive_path)
        || vul_file::exists (m_meta_path)
        || vul_file::exists (m_index_path))
    {
      LOG_INFO ("File exists and not allowed to overrwrite");
      return (false);
    }
  }

  LOG_INFO("Writing archive to: " << m_archive_path );
  LOG_INFO("Writing meta to: " << m_meta_path  );
  LOG_INFO("Writing index to: " << m_index_path );

  // Make sure the directory exists
  if ( ! vul_file::make_directory_path (m_output_path ) )
  {
    LOG_ERROR ("Error creating output directory: " << m_output_path );
    return false;
  }

  m_archive_stream = new vcl_ofstream;
  m_meta_stream = new vcl_ofstream;
  m_index_stream = new vcl_ofstream;

  m_archive_stream->open(m_archive_path.c_str());
  if ( ! m_archive_stream )
  {
    LOG_ERROR ("Could not open archive file: " << m_archive_path);
    return false;
  }

  m_meta_stream->open(m_meta_path.c_str());
  if ( ! m_meta_stream)
  {
    LOG_ERROR ("Could not open archive file: " << m_meta_path);
    return false;
  }

  m_index_stream->open(m_index_path.c_str());
  if ( ! m_index_stream)
  {
    LOG_ERROR ("Could not open archive file: " << m_index_path);
    return false;
  }

  m_archive = new vsl_b_ostream(m_archive_stream);
  m_meta = new vsl_b_ostream(m_meta_stream);

  // Write out version information.
  *m_index_stream << "3\n" // version number?
                  << vul_file::basename(m_archive_path) << "\n"
                  << vul_file::basename(m_meta_path) << "\n"
                  << m_mission_id << "\n";

  vsl_b_write( *m_archive, static_cast< int >(KWA_FORMAT) );

  vsl_b_write( *m_meta, static_cast< int >(KWA_FORMAT) );

  return true;
}


// ----------------------------------------------------------------
/** Close all files.
 *
 * Close and delete all active output files. Once this is done, no
 * further output can be generated. This is done in preparation for
 * deleting this object.
 */
void kw_archive_writer::
close_files()
{
  delete m_archive;
  m_archive = 0;

  delete m_meta;
  m_meta = 0;

  if (m_archive_stream) m_archive_stream->close();
  delete m_archive_stream;
  m_archive_stream = 0;

  if (m_meta_stream) m_meta_stream->close();
  delete m_meta_stream;
  m_meta_stream = 0;

  if (m_index_stream) m_index_stream->close();
  delete m_index_stream;
  m_index_stream = 0;
}


// ----------------------------------------------------------------
/** Write all frame data for current frame.
 *
 *
 */
void kw_archive_writer::
write_frame_data(kw_archive_writer::data_set_homog const& data)
{
  corners_t corners;
  calculate_frame_corner_points (data, corners);

  write_frame_data (data, corners);
}


void kw_archive_writer::
write_frame_data(kw_archive_writer::data_set_corners const& data)
{
  write_frame_data (data, data.corners);
}


void kw_archive_writer::
write_frame_data(kw_archive_writer::data_set const& data, corners_t const& corners)
{
  const vcl_streampos pos ( this->m_archive_stream->tellp() );

  const vxl_int_64 uSeconds ( data.m_timestamp.time() );

  vgl_h_matrix_2d<double> homog = data.m_src_to_ref_homog.get_transform();
  vnl_matrix_fixed< double, 3, 3 > frame2ref;
  homog.get( &frame2ref );

  this->m_archive->clear_serialisation_records();
  vsl_b_write( *this->m_archive, uSeconds );
  vsl_b_write( *this->m_archive, data.m_image );
  vsl_b_write( *this->m_archive, frame2ref );

  vsl_b_write( *this->m_archive, corners );
  vsl_b_write( *this->m_archive, data.m_gsd );
  vsl_b_write( *this->m_archive, data.m_timestamp.frame_number() );
  vsl_b_write( *this->m_archive, data.m_src_to_ref_homog.get_dest_reference().frame_number() );
  vsl_b_write( *this->m_archive, data.m_image.ni() );
  vsl_b_write( *this->m_archive, data.m_image.nj() );

  this->m_meta->clear_serialisation_records();
  vsl_b_write( *this->m_meta, uSeconds );
  vsl_b_write( *this->m_meta, frame2ref );
  vsl_b_write( *this->m_meta, corners );
  vsl_b_write( *this->m_meta, data.m_gsd );
  vsl_b_write( *this->m_meta, data.m_timestamp.frame_number() );
  vsl_b_write( *this->m_meta, data.m_src_to_ref_homog.get_dest_reference().frame_number() );
  vsl_b_write( *this->m_meta, data.m_image.ni() );
  vsl_b_write( *this->m_meta, data.m_image.nj() );

  *this->m_index_stream << uSeconds << " " << vxl_int_64( pos ) << "\n";
}


// ----------------------------------------------------------------
/** Update the frame's corner lat/lon coordinates.
 *
 * Given image->UTM homography, convert image corners to UTM and
 * onward to lat/lons. If homography is invalid, instead
 * use the most recent valid world points, on the assumption that
 * a hiccup in place will look better than the track snapping back
 * to the reference and then back to the current location.
 *
 * @param[in] data - current data set
 * @param[out] corners - calculated corners lat / lon
 */
void kw_archive_writer::
calculate_frame_corner_points( kw_archive_writer::data_set_homog const& data,  corners_t & corners)
{

  // quick exits if this homography is invalid
  if ( ! data.m_src_to_utm_homog.is_valid() )
  {
    corners = prior_lat_lons;
    return;
  }

  // Set up the corner points as a 3x4
  vnl_matrix_fixed<double,3,4> src_pts, utm_pts;

  double img_w = data.m_image.ni();
  double img_h = data.m_image.nj();

  src_pts(0,0) = 0.0;    src_pts(1,0) = 0.0;    // source upper left
  src_pts(0,1) = img_w;  src_pts(1,1) = 0.0;    // source upper right
  src_pts(0,2) = img_w;  src_pts(1,2) = img_h;  // source lower right
  src_pts(0,3) = 0.0;    src_pts(1,3) = img_h;  // source lower left;
  for (unsigned i = 0; i < 4; ++i)
  {
    src_pts(2,i) = 1.0;
  }

  // map points to UTM
  utm_pts = data.m_src_to_utm_homog.get_transform().get_matrix() * src_pts;

  for (unsigned i = 0; i < 4; ++i)
  {
    if ( utm_pts(2,i) != 0.0 )
    {
      utm_pts(0,i) /= utm_pts(2,i);    // easting
      utm_pts(1,i) /= utm_pts(2,i);    // northing
    }
    else
    {
      // we should probably just reload with prior values and return if we get here?
      utm_pts(0,i) = vidtk::video_metadata::lat_lon_t::INVALID;
      utm_pts(1,i) = vidtk::video_metadata::lat_lon_t::INVALID;
    }
  }

  // convert UTM to lat / lon, apply,  and cache
  vidtk::utm_zone_t utm_zone = data.m_src_to_utm_homog.get_dest_reference();
  for (unsigned i = 0; i < 4; ++i)
  {
    vnl_vector_fixed< double, 2 > latlon;

    vidtk::geographic::geo_coords geo_pt( utm_zone.zone(), utm_zone.is_north(), utm_pts(0, i), utm_pts(1, i) );

    latlon[0] = geo_pt.latitude();
    latlon[1] = geo_pt.longitude();

    prior_lat_lons.push_back (latlon);
  }

  corners = prior_lat_lons;

  // all done!
}

} // end namespace
