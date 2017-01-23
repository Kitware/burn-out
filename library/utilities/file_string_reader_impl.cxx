/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "file_string_reader_impl.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <utilities/shell_comments_filter.h>
#include <utilities/blank_line_filter.h>


#include <logger/logger.h>
VIDTK_LOGGER("file_string_reader_impl");

namespace vidtk {
namespace string_reader_impl {


// ----------------------------------------------------------------
file_string_reader_impl
::file_string_reader_impl( std::string const& _name )
  : string_reader_impl_base( _name )
{
  this->config_.add_parameter( "filename", "Each line contains a string we want to pass on" );
}


// ----------------------------------------------------------------
file_string_reader_impl
::~file_string_reader_impl( void )
{
}


// ----------------------------------------------------------------
bool file_string_reader_impl
::set_params( config_block const& blk )
{
  try
  {
    this->fname_ = blk.get< std::string > ( "filename" );
  }
  catch ( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


// ----------------------------------------------------------------
bool file_string_reader_impl
::initialize()
{
  if ( fname_.empty() )
  {
    LOG_ERROR( name() << " Need an input string" );
    return false;
  }

  if ( fin_ || in_stream_ )
  {
    fin_.close();
    fin_.clear();

    in_stream_.reset();
  }

  fin_.open( fname_.c_str() );

  if ( ! fin_ )
  {
    fin_.clear();
    LOG_ERROR( name() << " cannot open " << fname_ );
    return false;
  }

  in_stream_.push (vidtk::blank_line_filter());
  in_stream_.push (vidtk::shell_comments_filter());
  in_stream_.push (fin_);

  return true;
}


// ----------------------------------------------------------------
bool file_string_reader_impl
::step()
{
  if ( ! fin_ )
  {
    return false;
  }

  str_ = "";

  while ( std::getline( in_stream_, str_ ) && str_.empty() ) //skip
  {  }

  return str_ != "";
}


} // end namespace string_reader_impl
} // end namespace vidtk
