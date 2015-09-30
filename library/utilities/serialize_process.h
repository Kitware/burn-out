/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_SERIALIZE_PROCESS_H_
#define _VIDTK_SERIALIZE_PROCESS_H_

#include <vcl_string.h>
#include <vcl_ostream.h>
#include <vsl/vsl_fwd.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// VSL serialization process
template<typename T>
class serialize_process : public process
{
public:
  typedef serialize_process self_type;

  serialize_process( const vcl_string &name );

  ~serialize_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  void set_stream( vcl_ostream &stream );
  VIDTK_INPUT_PORT( set_stream, vcl_ostream& );

  void set_data( const T& d );
  VIDTK_INPUT_PORT( set_data, const T&);

private:
  config_block config_;

  bool disabled_;
  vcl_ostream *stream_;
  vsl_b_ostream *bstream_;
  const T *data_;
};

}  // namespace vidtk

#endif

