/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/string_reader_process.h>

#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>

#include <boost/thread/thread.hpp>

#include <string>
#include <iostream>

namespace
{

std::string test1("Test1\n");
std::string test2("Test 2\n");

void tcp_tread_function(vidtk::string_reader_process * tcp)
{
  TEST("can step 1: ", tcp->step(), true);
  std::cout << "Message: " << tcp->str() << std::endl;
  TEST("string is what is expected: ", tcp->str(), "Test1");
  TEST("can step 2: ", tcp->step(), true);
  std::cout << "Message: " << tcp->str() << std::endl;
  TEST("string is what is expected: ", tcp->str(), "Test 2");
  TEST("can step 3: ", tcp->step(), false);
}

void write_handler(const boost::system::error_code &/*ec*/, std::size_t /*bytes_transferred*/)
{
}

} // end namespace

int test_tcp_string_reader_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "tcp_string_reader_process" );

  {
    {
      vidtk::string_reader_process tcp("tcp_reader_test");
      vidtk::config_block block =  tcp.params();
      block.set("type", "tcp");
      block.set("tcp:port", "BOB");
      TEST("test non number port", tcp.set_params(block), false);
      block.set("tcp:port", "10001");
      TEST("test number port", tcp.set_params(block), true);
      TEST("test non init step1", tcp.step(), false);
    }
    {
      vidtk::string_reader_process tcp("tcp_reader_test");
      vidtk::string_reader_process tcp1("tcp_reader_test1");
      vidtk::config_block block =  tcp.params();
      block.set("type", "tcp");
      block.set("tcp:port", "10001");
      TEST("set config", tcp.set_params(block), true);
      TEST("set config", tcp1.set_params(block), true);
      TEST("can init: ", tcp.initialize(), true);
      TEST("can init: ", tcp1.initialize(), false);
    }
    {
      vidtk::string_reader_process tcp("tcp_reader_test");
      vidtk::config_block block =  tcp.params();
      block.set("type", "tcp");
      block.set("tcp:port", "10002");
      TEST("set config", tcp.set_params(block), true);
      TEST("can init: ", tcp.initialize(), true);
      boost::thread worker(&tcp_tread_function, &tcp);
      boost::this_thread::sleep( boost::posix_time::seconds(2) );
      boost::asio::io_service io_service;
      boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string("127.0.0.1"), 10002);
      boost::asio::ip::tcp::socket sock(io_service);
      sock.connect(endpoint);
      boost::asio::async_write(sock, boost::asio::buffer(test1), write_handler);
      boost::asio::async_write(sock, boost::asio::buffer(test2), write_handler);
      sock.close();
      worker.join();
    }
  }

  return testlib_test_summary();
}
