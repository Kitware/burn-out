/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vidtksys/DynamicLoader.hxx>
#include <logger/logger.h>
#include <process_framework/external_process_interface.h>
#include "external_proxy_process.h"

#include <cassert>

typedef vidtksys::DynamicLoader DL;

namespace vidtk
{

VIDTK_LOGGER ("external_proxy_process");


struct external_proxy_process::library_bindings
{
  library_bindings(void)
  :  count_(0),
     lib_handle_(NULL)
  {
  }

  ~library_bindings()
  {
    lib_handle_ = NULL;
  }

  boost::mutex mutex;
  void inc()
  {
    count_++;
  }
  void dec()
  {
    count_--;
  }

  unsigned int count_;
  vidtksys::DynamicLoader::LibraryHandle lib_handle_;
  boost::function<external_base_process* ()> lib_construct_;
  boost::function<bool (external_base_process*, const vidtk::config_block&)> lib_set_params_;
  boost::function<bool (external_base_process*)> lib_initialize_;
  boost::function<bool (external_base_process*, const vidtk::external_proxy_process::data_map_type&)> lib_set_inputs_;
  boost::function<bool (external_base_process*)> lib_step_;
  boost::function<bool (external_base_process*, vidtk::external_proxy_process::data_map_type*&)> lib_get_outputs_;
  boost::function<bool (external_base_process*)> lib_destruct_;
};

external_proxy_process::library_bindings * external_proxy_process::impl_ = NULL;

external_proxy_process
::external_proxy_process(const std::string &_name)
: process(_name, "external_proxy_process"),
  external_obj_(NULL),
  outputs_(NULL)
{
  this->config_.add_parameter("path", "Path to the shared library to load");
}


external_proxy_process
::~external_proxy_process(void)
{
  boost::unique_lock< boost::mutex > lock( instance_lock );
  assert(this->impl_ != NULL);
  if( external_obj_ && this->impl_->lib_destruct_ )
  {
    this->impl_->lib_destruct_(this->external_obj_);
  }
  this->impl_->dec();
  if( this->impl_->count_ == 0 && this->impl_->lib_handle_ )
  {
    DL::CloseLibrary(this->impl_->lib_handle_);
    delete this->impl_;
    this->impl_ = NULL;
  }
}

bool
external_proxy_process
::set_params(const config_block &blk)
{
  try
  {
    // Retrieve the shared library path
    this->path_ = blk.get<std::string>("path");
    if( this->path_.empty() )
    {
      throw config_block_parse_error("path cannot be empty");
    }

    // Retrieve the extra parameters to pass
    this->lib_config_ = blk.subblock("external");
    this->config_ = blk;
  }
  catch(const config_block_parse_error& ex)
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << ex.what() );
    return false;
  }
  return true;
}


config_block external_proxy_process
::static_params(void)
{
  config_block result;
  result.add_parameter("path", "Path to the shared library to load");
  return result;
}

config_block
external_proxy_process
::params() const
{
  return this->config_;
}


bool
external_proxy_process
::initialize(void)
{
  boost::unique_lock< boost::mutex > lock( instance_lock );

  // Step 0: create the library bindings if they do not exist
  if(this->impl_ == NULL)
  {
    this->impl_ = new library_bindings;
  }
  this->impl_->inc();

  // Step 1: Load the shared library
  if(this->impl_->lib_handle_ == NULL)
  {
    this->impl_->lib_handle_ = DL::OpenLibrary(this->path_.c_str());
    if( !this->impl_->lib_handle_ )
    {
      LOG_ERROR( this->name() << ": Unable to load external library: " << DL::LastError());
      return false;
    }

    // Step 2: Bind to the appropriate functions in th eshared library

    DL::SymbolPointer ptr_construct =
      DL::GetSymbolAddress(this->impl_->lib_handle_, "construct");
    if( !ptr_construct )
    {
      LOG_ERROR( this->name() << ": Unable to bind to construct function: " << DL::LastError());
      return false;
    }
    this->impl_->lib_construct_ =
      reinterpret_cast<external_process_fun_construct>(ptr_construct);

    DL::SymbolPointer ptr_set_params =
      DL::GetSymbolAddress(this->impl_->lib_handle_, "set_params");
    if( !ptr_construct )
    {
      LOG_ERROR( this->name() << ": Unable to bind to set_params function: " << DL::LastError());
      return false;
    }
    this->impl_->lib_set_params_ =
      reinterpret_cast<external_process_fun_set_params>(ptr_set_params);

    DL::SymbolPointer ptr_initialize =
      DL::GetSymbolAddress(this->impl_->lib_handle_, "initialize");
    if( !ptr_initialize )
    {
      LOG_ERROR( this->name() << ": Unable to bind to initialize function: " << DL::LastError());
      return false;
    }
    this->impl_->lib_initialize_ =
      reinterpret_cast<external_process_fun_initialize>(ptr_initialize);

    DL::SymbolPointer ptr_set_inputs =
      DL::GetSymbolAddress(this->impl_->lib_handle_, "set_inputs");
    if( !ptr_set_inputs )
    {
      LOG_ERROR( this->name() << ": Unable to bind to set_inputs function: " << DL::LastError());
      return false;
    }
    this->impl_->lib_set_inputs_ =
      reinterpret_cast<external_process_fun_set_inputs>(ptr_set_inputs);

    DL::SymbolPointer ptr_step =
      DL::GetSymbolAddress(this->impl_->lib_handle_, "step");
    if( !ptr_step )
    {
      LOG_ERROR( this->name() << ": Unable to bind to step function: " << DL::LastError());
      return false;
    }
    this->impl_->lib_step_ =
      reinterpret_cast<external_process_fun_step>(ptr_step);

    DL::SymbolPointer ptr_get_outputs =
      DL::GetSymbolAddress(this->impl_->lib_handle_, "get_outputs");
    if( !ptr_get_outputs )
    {
      LOG_ERROR( this->name() << ": Unable to bind to get_outputs function: " << DL::LastError());
      return false;
    }
    this->impl_->lib_get_outputs_ =
      reinterpret_cast<external_process_fun_get_outputs>(ptr_get_outputs);

    DL::SymbolPointer ptr_destruct =
      DL::GetSymbolAddress(this->impl_->lib_handle_, "destruct");
    if( !ptr_destruct )
    {
      LOG_ERROR( this->name() << ": Unable to bind to destruct function: " << DL::LastError());
      return false;
    }
    this->impl_->lib_destruct_ =
      reinterpret_cast<external_process_fun_destruct>(ptr_destruct);
  }

  // step 2.5: Create the external data object
  if(this->external_obj_ == NULL)
  {
    this->external_obj_ = this->impl_->lib_construct_();
    if(this->external_obj_ == NULL)
    {
      LOG_ERROR( this->name() << ": Could not construct the lib");
      return false;
    }
  }

  // Step 3: Now that the DLL is loaded, begin the initialization process

  if(! this->impl_->lib_set_params_(this->external_obj_, this->lib_config_) )
  {
    LOG_ERROR( this->name() << ": external set_params() failed");
    return false;
  }

  if(! this->impl_->lib_initialize_(this->external_obj_) )
  {
    LOG_ERROR( this->name() << ": external initialize() failed");
    return false;
  }

  return true;
}


void
external_proxy_process
::set_inputs(const data_map_type &inputs)
{
  boost::unique_lock< boost::mutex > lock( instance_lock );
  assert(this->impl_ != NULL);
  assert(this->external_obj_ != NULL);
  if( !this->impl_->lib_set_inputs_(this->external_obj_, inputs) )
  {
    LOG_ERROR( this->name() << ": Error setting inputs");
  }
}


bool
external_proxy_process
::step(void)
{
  boost::unique_lock< boost::mutex > lock( instance_lock );
  return this->impl_->lib_step_(this->external_obj_);
}


external_proxy_process::data_map_type
external_proxy_process
::outputs(void)
{
  boost::unique_lock< boost::mutex > lock( instance_lock );
  if( !this->impl_->lib_get_outputs_(this->external_obj_, this->outputs_) )
  {
    LOG_ERROR( this->name() << ": Error retrieving outputs");
  }
  return *this->outputs_;
}

}
