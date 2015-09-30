#include <vidtksys/DynamicLoader.hxx>
#include <utilities/log.h>
#include <process_framework/external_process_interface.h>
#include "external_proxy_process.h"

typedef vidtksys::DynamicLoader DL;

namespace vidtk
{

struct external_proxy_process::library_bindings
{ 
  library_bindings(void)
  : obj_(NULL),
    lib_handle_(NULL)
  {
  }

  external_base_process* obj_;
  vidtksys::DynamicLoader::LibraryHandle lib_handle_;
  boost::function<external_base_process* ()> lib_construct_;
  boost::function<bool (external_base_process*, const vidtk::config_block&)> lib_set_params_;
  boost::function<bool (external_base_process*)> lib_initialize_;
  boost::function<bool (external_base_process*, const vidtk::external_proxy_process::data_map_type&)> lib_set_inputs_;
  boost::function<bool (external_base_process*)> lib_step_;
  boost::function<bool (external_base_process*, vidtk::external_proxy_process::data_map_type*&)> lib_get_outputs_;
  boost::function<bool (external_base_process*)> lib_destruct_;
};


external_proxy_process
::external_proxy_process(const std::string &name)
: process(name, "external_proxy_process"),
  impl_(new library_bindings),
  outputs_(NULL)
{
  this->config_.add_parameter("path", "Path to the shared library to load");
}


external_proxy_process
::~external_proxy_process(void)
{
  if( this->impl_->obj_ && this->impl_->lib_destruct_ )
  {
    this->impl_->lib_destruct_(this->impl_->obj_);
  }
  if( this->impl_->lib_handle_ )
  {
    DL::CloseLibrary(this->impl_->lib_handle_);
  }
  delete this->impl_;
}

bool
external_proxy_process
::set_params(const config_block &blk)
{
  try
  {
    // Retrieve the shared library path
    this->path_ = blk.get<vcl_string>("path");
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
    log_error( this->name() << ": Unable to set parameters: "
               << ex.what() << vcl_endl );
    return false;
  }
  return true;
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

  // Step 1: Load the shared library

  this->impl_->lib_handle_ = DL::OpenLibrary(this->path_.c_str());
  if( !this->impl_->lib_handle_ )
  {
    log_error( this->name() << ": Unable to load external library: " 
               << DL::LastError() << vcl_endl );
    return false;
  }

  // Step 2: Bind to the appropriate functions in th eshared library
  
  DL::SymbolPointer ptr_construct = 
    DL::GetSymbolAddress(this->impl_->lib_handle_, "construct");
  if( !ptr_construct )
  {
    log_error( this->name() << ": Unable to bind to construct function: " 
               << DL::LastError() << vcl_endl );
    return false;
  }
  this->impl_->lib_construct_ = 
    reinterpret_cast<external_process_fun_construct>(ptr_construct);

  DL::SymbolPointer ptr_set_params = 
    DL::GetSymbolAddress(this->impl_->lib_handle_, "set_params");
  if( !ptr_construct )
  {
    log_error( this->name() << ": Unable to bind to set_params function: " 
               << DL::LastError() << vcl_endl );
    return false;
  }
  this->impl_->lib_set_params_ = 
    reinterpret_cast<external_process_fun_set_params>(ptr_set_params);

  DL::SymbolPointer ptr_initialize = 
    DL::GetSymbolAddress(this->impl_->lib_handle_, "initialize");
  if( !ptr_initialize )
  {
    log_error( this->name() << ": Unable to bind to initialize function: " 
               << DL::LastError() << vcl_endl );
    return false;
  }
  this->impl_->lib_initialize_ = 
    reinterpret_cast<external_process_fun_initialize>(ptr_initialize);

  DL::SymbolPointer ptr_set_inputs = 
    DL::GetSymbolAddress(this->impl_->lib_handle_, "set_inputs");
  if( !ptr_set_inputs )
  {
    log_error( this->name() << ": Unable to bind to set_inputs function: " 
               << DL::LastError() << vcl_endl );
    return false;
  }
  this->impl_->lib_set_inputs_ = 
    reinterpret_cast<external_process_fun_set_inputs>(ptr_set_inputs);

  DL::SymbolPointer ptr_step = 
    DL::GetSymbolAddress(this->impl_->lib_handle_, "step");
  if( !ptr_step )
  {
    log_error( this->name() << ": Unable to bind to step function: " 
               << DL::LastError() << vcl_endl );
    return false;
  }
  this->impl_->lib_step_ = 
    reinterpret_cast<external_process_fun_step>(ptr_step);

  DL::SymbolPointer ptr_get_outputs = 
    DL::GetSymbolAddress(this->impl_->lib_handle_, "get_outputs");
  if( !ptr_get_outputs )
  {
    log_error( this->name() << ": Unable to bind to get_outputs function: " 
               << DL::LastError() << vcl_endl );
    return false;
  }
  this->impl_->lib_get_outputs_ = 
    reinterpret_cast<external_process_fun_get_outputs>(ptr_get_outputs);

  DL::SymbolPointer ptr_destruct = 
    DL::GetSymbolAddress(this->impl_->lib_handle_, "destruct");
  if( !ptr_destruct )
  {
    log_error( this->name() << ": Unable to bind to destruct function: " 
               << DL::LastError() << vcl_endl );
    return false;
  }
  this->impl_->lib_destruct_ = 
    reinterpret_cast<external_process_fun_destruct>(ptr_destruct);

  // Step 3: Now that the DLL is loaded, begin the initialization process
  
  this->impl_->obj_ = this->impl_->lib_construct_();
  
  if(! this->impl_->lib_set_params_(this->impl_->obj_, this->lib_config_) )
  {
    log_error( this->name() << ": external set_params() failed" << vcl_endl );
    return false;
  }

  if(! this->impl_->lib_initialize_(this->impl_->obj_) )
  {
    log_error( this->name() << ": external initialize() failed" << vcl_endl );
    return false;
  }

  return true;
}


void
external_proxy_process
::set_inputs(const data_map_type &inputs)
{
  if( !this->impl_->lib_set_inputs_(this->impl_->obj_, inputs) )
  {
    log_error( this->name() << ": Error setting inputs" << vcl_endl );
  }
}


bool
external_proxy_process
::step(void)
{
  return this->impl_->lib_step_(this->impl_->obj_);
}


external_proxy_process::data_map_type&
external_proxy_process
::outputs(void)
{
  if( !this->impl_->lib_get_outputs_(this->impl_->obj_, this->outputs_) )
  {
    log_error( this->name() << ": Error retrieving outputs" << vcl_endl ) ;
  }
  return *this->outputs_;
}

}
