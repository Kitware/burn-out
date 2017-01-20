/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_FILE_STREAM_PROCESS_H_
#define _VIDTK_FILE_STREAM_PROCESS_H_

#include <string>
#include <fstream>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// @todo: This warning is being suppressed because, at the moment, there's nothing that can be done.
/// iostream will not allow a copy. This class is not used anywhere though and can likely get deleted.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/// Open a file output stream
class file_stream_process : public process
{
public:
  typedef file_stream_process self_type;

  file_stream_process( const std::string &name, bool in = true, bool out = true);

  virtual ~file_stream_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block &);

  virtual bool initialize();

  virtual bool step();

  std::iostream& stream(void);
  VIDTK_OUTPUT_PORT(std::iostream&, stream);

private:
  config_block config_;

  bool disabled_;
  bool mode_binary_;
  bool mode_append_;
  bool mode_input_;
  bool mode_output_;
  std::string filename_;
  std::fstream stream_;
};

}  // namespace vidtk

#endif

