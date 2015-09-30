/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_DESERIALIZE_PROCESS_H_
#define _VIDTK_DESERIALIZE_PROCESS_H_

#include <vcl_string.h>
#include <vcl_istream.h>
#include <vsl/vsl_fwd.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// VSL deserialization process
template<typename T>
class deserialize_process : public process
{
public:
  typedef deserialize_process self_type;

  deserialize_process( const vcl_string &name );

  ~deserialize_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_stream( vcl_istream &stream );
  VIDTK_INPUT_PORT( set_stream, vcl_istream& );

  T& data(void);
  VIDTK_OUTPUT_PORT( T&, data);

private:
  config_block config_;

  bool disabled_;
  bool first_step_;
  vcl_istream *stream_;
  vsl_b_istream *bstream_;
  T *data_;
};

}  // namespace vidtk

#endif

