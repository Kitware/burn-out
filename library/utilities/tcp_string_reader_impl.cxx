/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tcp_string_reader_impl.h"

#include <logger/logger.h>
VIDTK_LOGGER("tcp_string_reader_impl");

namespace vidtk {
namespace string_reader_impl {


// ----------------------------------------------------------------
tcp_string_reader_impl
::tcp_string_reader_impl( std::string const& _name )
  : string_reader_impl_base( _name ),
    acceptor_( io_ ),
    first_step_( true )
{
  this->config_.add_parameter( "port", "10000", "TCP Port to listen on" );
}


// ----------------------------------------------------------------
tcp_string_reader_impl
::~tcp_string_reader_impl( void )
{
}


// ----------------------------------------------------------------
bool tcp_string_reader_impl
::set_params( config_block const& blk )
{
  try
  {
    this->port_ = blk.get< unsigned int > ( "port" );
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
bool tcp_string_reader_impl
::initialize()
{
  try
  {
    boost::asio::ip::tcp::endpoint e( boost::asio::ip::tcp::v4(), this->port_ );
    this->acceptor_.open( e.protocol() );
    this->acceptor_.bind( e );
    this->acceptor_.listen();
  }
  catch ( const std::exception& e )
  {
    LOG_ERROR( this->name() << ':' << e.what() );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
bool tcp_string_reader_impl
::step()
{
  // we can't block waiting for the socket during initialize because
  // it prevents other nodes from initializing until after frames
  // start flowing. That approach causes a races condition Moving the
  // initialization here lets all other nodes start up and initialize
  // first.
  if ( first_step_ )
  {
    first_step_ = false;
    try
    {
      boost::system::error_code ec;
      this->acceptor_.accept( *this->stream_.rdbuf(), ec );
      if ( ec )
      {
        LOG_ERROR( this->name() << ": error code: " << ec );
        return false;
      }
    }
    catch ( const std::exception& e )
    {
      LOG_ERROR( this->name() << ':' << e.what() );
      return false;
    }
  }

  this->str_ = "";
  try
  {
    this->stream_.peek();
    if ( ! this->stream_ )
    {
      LOG_INFO( this->name() << " : stream closed" );
      return false;
    }

    if ( ! std::getline( this->stream_, this->str_ ) )
    {
      LOG_INFO( this->name() << " : failed to read line from stream" );
      return false;
    }
  }
  catch ( const std::exception& e )
  {
    LOG_ERROR( this->name() << ':' << e.what() );
    return false;
  }

  return true;
} // tcp_string_reader_impl::step


} // end namespace string_reader_impl
} // end namespace vidtk
