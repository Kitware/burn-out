/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _KW_ARCHIVE_INDEX_WRITER_H_
#define _KW_ARCHIVE_INDEX_WRITER_H_

#include <vxl_config.h>

#include <map>
#include <fstream>
#include <vector>

#include <vil/vil_image_view.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_metadata.h>
#include <utilities/large_file_ofstream.h>


//
// Partial types.
//
class vsl_b_ostream;

namespace vidtk
{


/// KW Archive format writer for a single index file
///
/// This class can write a single KWA file (i.e. a single .index file
/// and associated .data and .meta files).
template<class PixType>
class kw_archive_index_writer
{
public:
  struct open_parameters
  {
    // documentation in .cxx
    open_parameters();

    // This macro declares the set_X methods and the X_ variables
    #define DO_PARAMETER(NAME, TYPE)                                    \
       open_parameters& set_ ## NAME( TYPE const& value );              \
       TYPE NAME ## _;

    DO_PARAMETER(base_filename, std::string)
    DO_PARAMETER(separate_meta, bool)
    DO_PARAMETER(overwrite, bool)
    DO_PARAMETER(mission_id, std::string)
    DO_PARAMETER(stream_id, std::string)
    DO_PARAMETER(compress_image, bool)

    #undef DO_PARAMETER
  };

public:
  /// Constructor.
  kw_archive_index_writer();

  /// Destructor.
  ~kw_archive_index_writer();

  /// Open the archive.
  ///
  /// Refer to open_parameters::open_parameters for the documentation of
  /// the parameters.
  ///
  /// @return Returns \c true if the archive was successfully open,
  /// and \c false otherwise. If this returns \c false, the behavior
  /// of subsequent write operations are undefined.
  bool open(open_parameters const& param);

  /// Close the archive files.
  void close();

  /// Write a data frame.
  ///
  /// @returns \c true on success and \c false on failure.
  bool write_frame(vidtk::timestamp const& ts,
                   vidtk::video_metadata const& corner_points,
                   vil_image_view<PixType> const& image,
                   vidtk::image_to_image_homography const& frame_to_ref,
                   double gsd);

private:
  std::ofstream* index_stream_;
  vidtk::large_file_ofstream* meta_stream_;
  vsl_b_ostream* meta_bstream_;
  vidtk::large_file_ofstream* data_stream_;
  vsl_b_ostream* data_bstream_;

  int data_version_;
  bool compress_image_;

  std::vector<char> image_write_cache_;

  void convert_byte_view(vil_image_view<vxl_byte> const& image,
                         vil_image_view<vxl_byte> & byte_view);

  void convert_byte_view(vil_image_view<vxl_uint_16> const& image,
                         vil_image_view<vxl_byte> & byte_view);

  void write_frame_data(vsl_b_ostream& stream,
                        bool write_image,
                        vidtk::timestamp const& ts,
                        vidtk::video_metadata const& corner_points,
                        vil_image_view<PixType> const& image,
                        vidtk::image_to_image_homography const& frame_to_ref,
                        double gsd);

}; // end class kw_archive_index_writer

} // end namespace vidtk

#endif /* _KW_ARCHIVE_INDEX_WRITER_H_ */
