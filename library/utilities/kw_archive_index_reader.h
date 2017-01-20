/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _KW_ARCHIVE_INDEX_READER_H_
#define _KW_ARCHIVE_INDEX_READER_H_

#include <vxl_config.h>

#include <map>
#include <fstream>

#include <vil/vil_image_view.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_metadata.h>
#include <utilities/large_file_ifstream.h>


//
// Partial types.
//
class vsl_b_istream;

namespace vidtk
{


/// KW Archive format reader for a single index file
///
/// This class can read a single KWA file (i.e. a single .index file
/// and associated .data and .meta files).
class kw_archive_index_reader
{
public:
  /// Constructor.
  kw_archive_index_reader();

  /// Destructor.
  ~kw_archive_index_reader();

  /// Open the archive.
  ///
  /// @param[in] index_filename This should point to the ".index"
  /// file of the KW archive file set.
  ///
  /// @return Returns \c true if the archive was successfully open,
  /// and \c false otherwise. If this returns \c false, the behavior
  /// of subsequent read operations are undefined.
  bool open(std::string index_filename);

  std::string const& mission_id() const;

  std::string const& stream_id() const;

  /// Read the next frame in the stream.
  ///
  /// This reads the next data packet on the stream. Returns \c true
  /// on success, or \c false if the next frame could not be read
  /// (e.g. because of end-of-file). The latter condition can be
  /// checked by the eof() method.
  ///
  bool read_next_frame();

  /// Determine if the end of the archive was reached.
  ///
  /// End-of-file can usually only be determined after a read fails,
  /// and so calls to this function is usually meaningful only after a
  /// failed call to read_next_frame().
  ///
  /// \returns \c true if the last read failed because of end-of-file,
  /// \c false otherwise.
  bool eof() const;

  /// @defgroup KWAGetters
  /// \brief Getters for the data elements.
  ///
  /// This collection of functions retrieves the data elements that
  /// were last read from the archive. As such, they only make sense
  /// right after a successful read call (e.g. to
  /// read_next_frame()). Their behavior after an unsuccessful read
  /// call is undefined.
  ///
  /// @{

  /// The timestamp of the frame.
  vidtk::timestamp const& timestamp() const;

  /// The pixels.
  ///
  /// If the pixel data is not available, this may return a null image
  /// (i.e. vil_image_view()).
  vil_image_view<vxl_byte> const& image() const;

  /// \brief The geo-coordinates of the frame corner points.
  ///
  /// \note The current KWA format only stores the corner points, and
  /// so only those elements of the return structure will be valid.
  ///
  vidtk::video_metadata const& corner_points() const;

  /// The frame to reference homography.
  vidtk::image_to_image_homography const& frame_to_reference() const;

  /// The ground sample distance (GSD).
  double gsd() const;

  /// @}

private:
  // The map from timestamps to file indexes, for quick seeking
  std::map<vxl_int_64,vxl_int_64> index_;

  // This will point to the .data file if it exists, and to the .meta
  // file otherwise.
  vidtk::large_file_ifstream* info_stream_;
  vsl_b_istream* info_b_stream_;

  // True if we found the .data file (i.e. info_stream_ points to a
  // .data file), false otherwise (i.e. info_stream_ points to a .meta
  // file).
  bool have_data_file_;

  // True if we are expecting the image data to be compressed
  bool compressed_image_;

  std::string mission_id_;
  std::string stream_id_;

  // Cache the frame last read
  vidtk::timestamp ts_;
  vidtk::video_metadata points_;
  vil_image_view<vxl_byte> image_;
  vidtk::image_to_image_homography frame_to_ref_;
  double gsd_;

}; // end class kw_archive_index_reader

} // end namespace vidtk

#endif /* _KW_ARCHIVE_INDEX_READER_H_ */
