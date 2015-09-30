#include <utilities/log.h>
#include "external_base_process.h"

namespace vidtk
{

external_base_process
::external_base_process(const vcl_string &proc_name,
                        const vcl_string &child_name)
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
    log_error( this->name() << ": Unable to set parameters: " 
               << e.what() << vcl_endl );
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

external_base_process::data_map_type&
external_base_process
::outputs(void)
{
  return this->outputs_;
}

} // end namespace