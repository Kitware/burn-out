/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
  \file
   \brief
   This header represents an external interface for a dynamically loadable
   vidtk process.  Input and output ports are implemented as property maps.
   This file is internal to vidtk and should not need to be directly consumed
   by any user code.
 */

#ifndef VIDTK_PROCESS_FRAMEWORK_EXTERNAL_PROCESS_INTERFACE_H_
#define VIDTK_PROCESS_FRAMEWORK_EXTERNAL_PROCESS_INTERFACE_H_

#ifdef _WIN32
#  ifdef EXTERNAL_PROCESS_EXPORT_SYMBOL
#    define EXTERNAL_PROCESS_API __declspec( dllexport )
#  else
#    define EXTERNAL_PROCESS_API __declspec( dllimport )
#  endif
#else
#  define EXTERNAL_PROCESS_API
#endif

#include <utilities/config_block.h>
#include <process_framework/external_base_process.h>

extern "C"
{

// Constructor returning a pointer to the tracker object
EXTERNAL_PROCESS_API
vidtk::external_base_process*
construct(void);

// Set configuration parameters
EXTERNAL_PROCESS_API
bool
set_params(vidtk::external_base_process* obj, const vidtk::config_block &blk);

// Initialize
EXTERNAL_PROCESS_API
bool
initialize(vidtk::external_base_process* obj);

// Set process inputs
EXTERNAL_PROCESS_API
bool
set_inputs(vidtk::external_base_process* obj, const vidtk::external_base_process::data_map_type &inputs);

// Execute the tracking algorithm for 1 step
EXTERNAL_PROCESS_API
bool
step(vidtk::external_base_process* obj);

// Retrieve processs outputs
EXTERNAL_PROCESS_API
bool
get_outputs(vidtk::external_base_process* obj, vidtk::external_base_process::data_map_type *&outputs);

// Destroy the tracker object
EXTERNAL_PROCESS_API
void
destruct(vidtk::external_base_process *obj);

} // extern "C"

// Define the funtion pointers
typedef vidtk::external_base_process* (*external_process_fun_construct)();
typedef bool (*external_process_fun_set_params)(vidtk::external_base_process*, const vidtk::config_block&);
typedef bool (*external_process_fun_initialize)(vidtk::external_base_process*);
typedef bool (*external_process_fun_set_inputs)(vidtk::external_base_process*,
              const vidtk::external_base_process::data_map_type&);
typedef bool (*external_process_fun_step)(vidtk::external_base_process*);
typedef bool (*external_process_fun_get_outputs)(vidtk::external_base_process*,
                                                 vidtk::external_base_process::data_map_type*&);
typedef bool (*external_process_fun_destruct)(vidtk::external_base_process*);

#endif //VIDTK_PROCESS_FRAMEWORK_EXTERNAL_PROCESS_WRAPPER_H_
