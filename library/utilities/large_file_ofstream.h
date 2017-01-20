/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _LARGE_FILE_OFSTREAM_H_
#define _LARGE_FILE_OFSTREAM_H_

#include <vxl_config.h>
#include <fstream>

namespace vidtk
{

/// \brief A file stream that supports files > 2GB (needed for Windows).
///
/// This class implements a file stream (ofstream) such that files
/// larger than 2 GiB can be written. The trouble is on Windows, where
/// std::streampos is a 32-bit integer (even on 64-bit Windows), which
/// means we cannot directly seek or tell() to a position beyond 2 GiB using
/// std::ofstream. The workaround, implemented by this class, is to
/// use a std::filebuf wrapped around a FILE*, and use the C API that
/// supports large files to seek the FILE*.
///
/// On non-Windows platforms, this class is the same as std::ofstream.
///
/// For documentation on the methods, look at a reference for
/// std::ofstream.
class large_file_ofstream
  : public std::ofstream
{
public:
  /// \brief Look at the documentation for std::ofstream.
  large_file_ofstream(const char* filename, std::ios_base::openmode mode = std::ios_base::out);

  /// \brief Look at the documentation for std::ofstream.
  large_file_ofstream();

  /// \brief Look at the documentation for std::ofstream.
  ~large_file_ofstream();

  /// \brief Look at the documentation for std::ofstream.
  void open(const char* filename, std::ios_base::openmode mode = std::ios_base::out);

  /// \brief Look at the documentation for std::ofstream.
  large_file_ofstream& seekp(vxl_int_64 pos);

  /// \brief Look at the documentation for std::ofstream.
  vxl_int_64 tellp();

private:
  struct private_data;
  private_data* p_;
};


} // end namespace vidtk


#endif /* _LARGE_FILE_OFSTREAM_H_ */
