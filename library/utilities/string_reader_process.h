/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_string_reader_process_h_
#define vidtk_string_reader_process_h_

#include <string>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <boost/scoped_ptr.hpp>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Read strings from a file.
 *
 * This process supplies strings on the "str" port for downstream
 * processes to use. Typically these strings would be file names, but
 * in some cases the strings also contain the crop strings.
 *
 * Currently strings can be sourced from a TCP socket, file contents,
 * or names in a directory. Other sources can be easily added as needed.
 */

class string_reader_process
  : public vidtk::process
{
public:
  typedef string_reader_process self_type;

  string_reader_process( std::string const& name );
  virtual ~string_reader_process(void);

  virtual config_block params() const;
  virtual bool set_params( config_block const& blk);
  virtual bool initialize();
  virtual bool step();

  // -- OUTPUTS --
  std::string str(void) const;
  VIDTK_OUTPUT_PORT( std::string, str );

private:

  class priv;
  boost::scoped_ptr<priv> d;
};


} // end namespace vidtk

#endif // vidtk_tcp_string_reader_process_h_
