/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "split_string_process.h"

#include <vul/vul_reg_exp.h>

#include <logger/logger.h>
VIDTK_LOGGER("split_string_process_cxx");


namespace vidtk
{

struct split_string_process::impl
{
  vul_reg_exp re_;
  std::string input_;
  std::string group1_;
  std::string group2_;
};


split_string_process
::split_string_process(const std::string &n)
: process(n, "split_string_process"), p_(new split_string_process::impl)
{
  this->config_.add_parameter("pattern", "([^:]*):(.*)", "Regular expression separating the input string into 2 groups");
}


split_string_process
::~split_string_process()
{
  delete this->p_;
}


config_block
split_string_process
::params() const
{
  return this->config_;
}


bool
split_string_process
::set_params(const config_block &blk)
{
  try
  {
    std::string p = blk.get<std::string>("pattern");
    this->p_->re_.compile(p.c_str());
  }
  catch(const config_block_parse_error &e)
  {
    LOG_ERROR(this->name() << e.what());
    return false;
  }
  this->config_ = blk;
  return true;
}


bool
split_string_process
::initialize()
{
  return true;
}


process::step_status
split_string_process
::step2()
{
  if(!this->p_->re_.find(this->p_->input_))
  {
    return process::FAILURE;
  }
  this->p_->group1_ = this->p_->re_.match(1);
  this->p_->group2_ = this->p_->re_.match(2);
  return process::SUCCESS;
}


void
split_string_process
::set_input(const std::string &in )
{
  this->p_->input_ = in;
}


std::string
split_string_process
::group1(void) const
{
  return this->p_->group1_;
}


std::string
split_string_process
::group2(void) const
{
  return this->p_->group2_;
}

} // end namespace
