/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "external_base_process.h"

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_external_base_process_cxx__
VIDTK_LOGGER("external_base_process_cxx");


namespace vidtk
{

external_base_process
::external_base_process(const std::string &proc_name,
                        const std::string &child_name)
: process(proc_name, child_name),
  disabled_(false),
  inputs_(NULL)
{
  this->config_.add_parameter("disabled",
                              "false",
                              "Whether or not the process is disabled");
}


external_base_process
::~external_base_process(void)
{
}


bool
external_base_process
::set_params(const config_block &blk)
{
  try
  {
    this->disabled_ = blk.get<bool>("disabled");
  }
  catch(const config_block_parse_error &e)
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: "
               << e.what() );
    return false;
  }

  this->config_.update(blk);
  return true;
}


config_block
external_base_process
::params(void) const
{
  return this->config_;
}


void
external_base_process
::set_inputs(const external_base_process::data_map_type &inputs)
{
  this->inputs_ = &inputs;
}

external_base_process::data_map_type &
external_base_process
::outputs(void)
{
  return this->outputs_;
}

} // end namespace
