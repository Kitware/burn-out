/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
  \file
   \brief
   This file contains the implementation for a proxy class which can
   dynamically load and attach to an externally defined process in a shared
   library.  The only configuration parameter needed is "path" which contains
   the shared object to load.  It can be the full path to the library or just   the library name.  In the case of the former, the existing runtime library
   search path will be used.  An additional configuration subblock "external"
   will be parsed and all configuration parameters passed to the attached
   process' set_params function.

   If a more complex or detailed process interface with explicit input and
   output ports is desired, you can subclass this proxy object and implement
   the ports and step functions in terms of the property map
 */
#ifndef VIDTK_UTILITIES_EXTERNAL_PROXY_PROCESS_H_
#define VIDTK_UTILITIES_EXTERNAL_PROXY_PROCESS_H_

#include <string>
#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <utilities/config_block.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

class external_base_process;

class external_proxy_process : public process
{
public:
  typedef external_proxy_process self_type;
  typedef std::map< std::string, boost::any > data_map_type;

  external_proxy_process(const std::string &name);
  virtual ~external_proxy_process(void);

  // set_params has a bit of a "chicken and egg" problem.  Since remote
  // functions cannot be called until after initialization, only the path
  // parameter will be explicitly set here.  The "external" subblock
  // will be retrieved but the remote set_params function will not be called
  // until inside initialize()
  virtual bool set_params(const config_block &blk);
  static config_block static_params(void);
  virtual config_block params(void) const;

  // Attach to the external process shared library and bind to the appropriate
  // function signatures.  Once binding is complete, instantiate, set
  // parameters, and initialize the external process.
  virtual bool initialize(void);

  // Pass all inputs in a property map
  virtual void set_inputs( const data_map_type& inputs );
  VIDTK_INPUT_PORT( set_inputs, const data_map_type& );

  // Execute the step function
  virtual bool step(void);

  // Retrieve all outputs in a property map
  virtual data_map_type outputs( void );
  VIDTK_OUTPUT_PORT( data_map_type, outputs );

  struct library_bindings;

private:
  config_block config_;

  boost::mutex instance_lock;

  std::string path_;
  config_block lib_config_;

  static library_bindings *impl_;
  external_base_process* external_obj_;
  data_map_type *outputs_;
};

}  // end namespace

#endif //VIDTK_UTILITIES_external_proxy_process_H_
