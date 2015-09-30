/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_LIST_WRITER_PROCESS_H_
#define _VIDTK_LIST_WRITER_PROCESS_H_

#include <vcl_string.h>
#include <vcl_ostream.h>
#include <vcl_vector.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Writes a vector of elements to a file in ASCII
template<typename T>
class list_writer_process : public process
{
public:
  typedef list_writer_process self_type;

  list_writer_process( const vcl_string &name );

  ~list_writer_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_stream( vcl_ostream &stream );
  VIDTK_INPUT_PORT( set_stream, vcl_ostream& );

  void set_data( const vcl_vector<T> &data );
  VIDTK_INPUT_PORT( set_data, const vcl_vector<T>& );

private:
  config_block config_;

  bool disabled_;
  vcl_ostream *stream_;
  const vcl_vector<T> *data_;
};

}  // namespace vidtk

#endif

