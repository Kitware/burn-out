/*ckwg +5
 * Copyright 2012,2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "large_file_ofstream.h"

#include <iostream>
#include <string>

#include <logger/logger.h>

#ifdef VCL_WIN32
  #include <cstdio>
#endif

VIDTK_LOGGER( "large_file_ofstream" );


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

struct large_file_ofstream::private_data
{
#ifdef VCL_WIN32
  private_data()
    : file_( NULL )
  {
  }


  FILE* file_;
#endif
};

large_file_ofstream
::large_file_ofstream( const char* filename, std::ios_base::openmode mode )
  : p_( new private_data )
{
  this->open( filename, mode );
}


large_file_ofstream
::large_file_ofstream()
  : p_( new private_data )
{
}


large_file_ofstream
::~large_file_ofstream()
{
  this->close();
  delete this->p_;
}


void
large_file_ofstream
::open( const char* filename, std::ios_base::openmode mode )
{
#ifdef VCL_WIN32
  if ( (mode & std::ios_base::trunc) && ( mode & std::ios_base::app ) )
  {
    LOG_ERROR( "Simultaneous trunc and app is not implemented" );
    this->setstate( std::ios::failbit );
    return;
  }
  if ( ( (mode & std::ios_base::app) == 0 ) && ( mode & std::ios_base::ate ) )
  {
    LOG_ERROR( "Simultaneous ate without app is not implemented" );
    this->setstate( std::ios::failbit );
    return;
  }

  // Note that even without trunc, ofstream truncates
  // http://stdcxx.apache.org/doc/stdlibug/30-3.html#Table 33
  //
  std::string cmode = (mode & std::ios_base::app) ? "a" : "w";
  cmode += (mode & std::ios_base::binary) ? 'b' : 't';

  if ( fopen_s( &this->p_->file_, filename, cmode.c_str() ) != 0 )
  {
    this->setstate( std::ios::failbit );
    return;
  }
  static_cast< remove_protection_from_filebuf* > ( this->rdbuf() )->
    _Init( this->p_->file_, std::filebuf::_Openfl );
#else
  std::ofstream::open( filename, mode );
#endif
}


large_file_ofstream&
large_file_ofstream
::seekp( vxl_int_64 pos )
{
#ifdef VCL_WIN32
  // Don't seek if not necessary, to avoid flushing data unnecessarily
  if ( _ftelli64( this->p_->file_ ) == pos )
  {
    return *this;
  }

  if ( _fseeki64( this->p_->file_, pos, SEEK_SET ) == 0 )
  {
    this->rdbuf()->pubsync();
  }
  else
  {
    this->setstate( std::ios::failbit );
  }
#else // if not VCL_WIN32
  std::ofstream::seekp( pos );
#endif // VCL_WIN32
  return *this;
}


vxl_int_64
large_file_ofstream
::tellp()
{
#ifdef VCL_WIN32
  return _ftelli64( this->p_->file_ );
#else
  return std::ofstream::tellp();
#endif
}


} // end namespace vidtk
