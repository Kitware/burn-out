/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "large_file_ifstream.h"

#include <iostream>

#include <logger/logger.h>

#ifdef VCL_WIN32
  #include <cstdio>
#endif

VIDTK_LOGGER("large_file_ifstream");


#ifdef VCL_WIN32
namespace // anonymous
{

// This sole purpose of this class is to circumvent the protected
// status of the _Init function in std::filebuf for MSVC.
struct remove_protection_from_filebuf
  : std::filebuf
{
  using std::filebuf::_Init;
};

} // end namespace anonymous
#endif

namespace vidtk
{

struct large_file_ifstream::private_data
{
#ifdef VCL_WIN32
  private_data()
    : file_(NULL)
  {
  }

  FILE* file_;
#endif
};

large_file_ifstream
::large_file_ifstream( const char* filename, std::ios_base::openmode mode )
  : p_(new private_data)
{
  this->open(filename, mode);
}


large_file_ifstream
::large_file_ifstream()
  : p_(new private_data)
{
}


large_file_ifstream
::~large_file_ifstream()
{
  this->close();
  delete this->p_;
}


void
large_file_ifstream
::open(const char* filename, std::ios_base::openmode mode)
{
#ifdef VCL_WIN32
  char const* cmode;
  if (mode & std::ios_base::binary)
  {
    cmode = "rb";
  }
  else
  {
    cmode = "r";
  }
  if (fopen_s(&this->p_->file_, filename, cmode)!=0)
  {
    this->setstate( std::ios::failbit );
    return;
  }
  static_cast<remove_protection_from_filebuf*>(this->rdbuf())->
                          _Init(this->p_->file_, std::filebuf::_Openfl);
#else
  std::ifstream::open(filename, mode);
#endif
}


large_file_ifstream&
large_file_ifstream
::seekg(vxl_int_64 pos)
{
#ifdef VCL_WIN32
  // Don't seek if not necessary, to avoid flushing data that has already been
  // read into the buffer
  if (_ftelli64(this->p_->file_) == pos)
  {
    return *this;
  }

  if (_fseeki64(this->p_->file_, pos, SEEK_SET) == 0)
  {
    this->rdbuf()->pubsync();
  }
  else
  {
    this->setstate(std::ios::failbit);
  }
#else // if not VCL_WIN32
  std::ifstream::seekg(pos);
#endif // VCL_WIN32
  return *this;
}


vxl_int_64
large_file_ifstream
::tellg()
{
#ifdef VCL_WIN32
  return _ftelli64(this->p_->file_);
#else
  return std::ifstream::tellg();
#endif
}


} // end namespace vidtk
