/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_TCP_STRING_READER_IMPL_H_
#define _VIDTK_TCP_STRING_READER_IMPL_H_

#include <utilities/string_reader_impl_base.h>

#include <iostream>
#include <boost/asio.hpp>


namespace vidtk {
namespace string_reader_impl {

// ----------------------------------------------------------------
/** \brief Read string from TCP port.
 *
 * This class reads strings from the specified TCP port and passes
 * them down stream.
 */
class tcp_string_reader_impl
  : public string_reader_impl_base
{
public:
  tcp_string_reader_impl( std::string const& name );
  virtual ~tcp_string_reader_impl();

  virtual bool set_params( config_block const& blk );
  virtual bool initialize();
  virtual bool step();

private:
  unsigned int port_;
  boost::asio::io_service io_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::iostream stream_;

  bool first_step_;
}; // end class tcp_string_reader_impl

} // end namespace string_reader_impl
} // end namespace vidtk

#endif /* _VIDTK_TCP_STRING_READER_IMPL_H_ */
