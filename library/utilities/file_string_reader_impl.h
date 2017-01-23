/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_file_string_reader_impl_h_
#define vidtk_file_string_reader_impl_h_

#include <utilities/string_reader_impl_base.h>

#include <string>
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>

namespace vidtk {
namespace string_reader_impl {

// ----------------------------------------------------------------
/*! \brief Reads in a file where each line is a string to pass downstream.
 *
 * This process reads a file and passes each line to the output port.
 * Typically this process would be used to source image file names to
 * image reader processes that take file names.
 */
class file_string_reader_impl
  : public string_reader_impl_base
{
public:
  file_string_reader_impl( std::string const& name );
  virtual ~file_string_reader_impl(void);

  virtual bool set_params( config_block const& blk);
  virtual bool initialize();
  virtual bool step();

private:
  std::string fname_;

  std::ifstream fin_;
  boost::iostreams::filtering_istream in_stream_;
};


} // end namespace string_reader_impl
} // end namespace vidtk

#endif // vidtk_file_string_reader_impl_h_
