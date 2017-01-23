/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _LARGE_FILE_IFSTREAM_H_
#define _LARGE_FILE_IFSTREAM_H_

#include <vxl_config.h>
#include <fstream>

namespace vidtk
{

/// \brief A file stream that supports files > 2GB (needed for Windows).
///
/// This class implements a file stream (ifstream) such that files
/// larger than 2 GiB can be read. The trouble is on Windows, where
/// std::streampos is a 32-bit integer (even on 64-bit Windows), which
/// means we cannot directly seek to a position beyond 2 GiB using
/// std::ifstream. The workaround, implemented by this class, is to
/// use a std::filebuf wrapped around a FILE*, and use the C API that
/// supports large files to seek the FILE*.
///
/// On non-Windows platforms, this class is the same as std::ifstream.
///
/// For documentation on the methods, look at a reference for
/// std::ifstream.
class large_file_ifstream
  : public std::ifstream
{
public:
  /// \brief Look at the documentation for std::ifstream.
  large_file_ifstream(const char* filename, std::ios_base::openmode mode = std::ios_base::in);

  /// \brief Look at the documentation for std::ifstream.
  large_file_ifstream();

  /// \brief Look at the documentation for std::ifstream.
  ~large_file_ifstream();

  /// \brief Look at the documentation for std::ifstream.
  void open(const char* filename, std::ios_base::openmode mode = std::ios_base::in);

  /// \brief Look at the documentation for std::ifstream.
  large_file_ifstream& seekg(vxl_int_64 pos);

  /// \brief Look at the documentation for std::ifstream.
  vxl_int_64 tellg();

private:
  struct private_data;
  private_data* p_;
};


} // end namespace vidtk


#endif /* _LARGE_FILE_IFSTREAM_H_ */
