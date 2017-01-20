/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_SPLIT_STRING_PROCESS_H_
#define _VIDTK_SPLIT_STRING_PROCESS_H_

#include <string>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Seperate a string into one of three categories based off of a delimiter
class split_string_process : public process
{
public:
  typedef split_string_process self_type;

  split_string_process(const std::string &name);

  ~split_string_process();

  virtual config_block params() const;

  virtual bool set_params(const config_block &blk);

  virtual bool initialize();

  virtual process::step_status step2();

  virtual bool step()
  {
    return this->step2() == process::SUCCESS;
  }

  void set_input(const std::string &in);
  VIDTK_INPUT_PORT(set_input, const std::string&);

  std::string group1(void) const;
  VIDTK_OUTPUT_PORT(std::string, group1);

  std::string group2(void) const;
  VIDTK_OUTPUT_PORT(std::string, group2);

private:
  config_block config_;

  struct impl;
  impl *p_;
};

}  // namespace vidtk

#endif
