/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_DIR_STRING_READER_IMPL_H_
#define _VIDTK_DIR_STRING_READER_IMPL_H_

#include <utilities/string_reader_impl_base.h>

#include <string>
#include <set>
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace vidtk {
namespace string_reader_impl {

// ----------------------------------------------------------------
/*! \brief Reads in a file where each line is a string to pass downstream.
 *
 * This process reads a file and passes each line to the output port.
 * Typically this process would be used to source image file names to
 * image reader processes that take file names.
 */
class dir_string_reader_impl
  : public string_reader_impl_base
{
public:
  dir_string_reader_impl( std::string const& name );
  virtual ~dir_string_reader_impl(void);

  virtual bool set_params( config_block const& blk);
  virtual bool initialize();
  virtual bool step();

private:
  std::string glob_pattern;
  unsigned int buffer_size;
  long timeout;
  bool monitor_mode;

  // store image time and filename. Time goes first so we can use default sort
  std::deque<std::pair<double, std::string> > filename_list;

  // to keep track of all files we've seen
  std::set< std::string > all_files;

  // keep track of when we get files
  boost::posix_time::ptime lastfile;
  boost::posix_time::ptime curtime;
};


} // end namespace string_reader_impl
} // end namespace vidtk

#endif // vidtk_dir_string_reader_impl_h_
