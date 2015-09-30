/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_FILE_STREAM_PROCESS_H_
#define _VIDTK_FILE_STREAM_PROCESS_H_

#include <vcl_string.h>
#include <vcl_fstream.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Open a file output stream
class file_stream_process : public process
{
public:
  typedef file_stream_process self_type;

  file_stream_process( const vcl_string &name, bool in = true, bool out = true);

  ~file_stream_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  vcl_iostream& stream(void);
  VIDTK_OUTPUT_PORT(vcl_iostream &, stream);

private:
  config_block config_;

  bool disabled_;
  bool mode_binary_;
  bool mode_append_;
  bool mode_input_;
  bool mode_output_;
  vcl_string filename_;
  vcl_fstream stream_;
};

}  // namespace vidtk

#endif

