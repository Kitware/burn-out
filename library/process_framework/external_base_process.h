/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
  \file
   \brief
   This file contains the base class that all dynamically loadable processes
   must inherit from.  It is expected to be subclassed in an external shared
   library.  In order to accomodate the variety of desired input and output
   ports, external processes are implemnted with only a single input port
   and a single output port.  Each of these passes a data_map_type which can
   contain multiple named parameters.  When subclassing this process, do not
   add additional input ports or output ports as they will not get called
   properly.  A more detailed process interface can be created on the callers
   side by subclassing the external_proxy_process.
 */

#ifndef VIDTK_UTILITIES_EXTERNAL_BASE_PROCESS_H_
#define VIDTK_UTILITIES_EXTERNAL_BASE_PROCESS_H_

#include <string>
#include <vector>

#include <utilities/config_block.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <boost/any.hpp>

namespace vidtk
{

/// @todo: This warning is being suppressed because, at the moment, there's nothing that can be done.
/// The using code ( matlab plugin ) will crash if the return on the output_port is not a ref.
/// It's likely that the code is trying to use it as a temporary for which it assumes maintained scope.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

class external_base_process : public process
{
public:
  typedef external_base_process self_type;
  typedef std::map< std::string, boost::any > data_map_type;

  virtual ~external_base_process();

  // Pass configuration parameters
  virtual bool set_params(const config_block& blk);

  // Retrieve config block
  virtual config_block params(void) const;

  // Initialization
  virtual bool initialize(void) = 0;

  // Pass all inputs in a property map
  virtual void set_inputs( const data_map_type& inputs );
  VIDTK_INPUT_PORT( set_inputs, const data_map_type& );

  // Input ports, output ports, and step function will be defined in subclass
  bool step(void) = 0;

  // Retrieve all outputs in a property map
  //NOTE:  This is potentially a bad idea, but it is needed for the external matlab stuff
  //       used by cete.  Please talk to others (possibly Juda) before chaning this to
  //       something other than a reference.
  virtual data_map_type & outputs( void );
  VIDTK_OUTPUT_PORT( data_map_type & , outputs );

protected:

  // Protected constructor to be called by subclasses
  external_base_process(const std::string &proc_name,
                        const std::string &class_name);

  config_block config_;

  // Whether or not the process is disabled
  bool disabled_;

  // Collection of input valies
  const data_map_type *inputs_;

  // Collection of oputput values
  data_map_type outputs_;
};

} // end namespace

#endif // VIDTK_UTILITIES_EXTERNAL_BNASE_PROCESS_H_
