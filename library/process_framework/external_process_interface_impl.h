/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
  \file
   \brief
   This file contains the implementation details for creating an external
   dynamicly loadable vidtk process.

   To create an external process, you will need the following:

   1.  A shared library built with EXTERNAL_PROCESS_EXPORT_SYMBOL defined
   2.  A class that inherits from external_base_process,
       Ex: class my_super_cool_process : public external_base_process
   3.  An instance file in the shared library containing only the following

   #include <path/to/my_super_cool_process.h>

   #define EXTERNAL_PROCESS_TYPE super_cool::my_super_cool_process
   #define EXTERNAL_PROCESS_NAME "proc1" // Name of the process node
   #include <process_framework/external_process_interface_impl.h>

   This will create all the appropriate bindings on both sides to be able
   dynamically load and bind to your process.
*/

#include <vil/vil_image_view.h>
#include <utilities/log.h>
#include <process_framework/external_process_interface.h>

#ifndef EXTERNAL_PROCESS_TYPE 
#error "EXTERNAL_PROCESS_TYPE must be defined"
#endif
#ifndef EXTERNAL_PROCESS_NAME
#error "EXTERNAL_PROCESS_NAME must be defined"
#endif

#define TO_STR_( X ) TO_STR( X )
#define TO_STR( X ) #X

#ifdef __cplusplus
extern "C" {
#endif

// Constructor returning a pointer to the tracker object
EXTERNAL_PROCESS_API
vidtk::external_base_process* 
construct(void)
{
  return new (EXTERNAL_PROCESS_TYPE)( TO_STR(EXTERNAL_PROCESS_NAME) );
}


// Set configuration parameters
EXTERNAL_PROCESS_API
bool
set_params(vidtk::external_base_process* obj, const vidtk::config_block &blk)
{
  if( !obj )
  {
    log_error( TO_STR(EXTERNAL_PROCESS_NAME)
               << "_wrapper: Invalid pointer passed for object"
               << vcl_endl );
    return false;
  }

  vidtk::config_block obj_blk = obj->params();
  obj_blk.update(blk);

  return obj->set_params(obj_blk);
}


// Initialize 
EXTERNAL_PROCESS_API 
bool 
initialize(vidtk::external_base_process* obj)
{
  if( !obj )
  {
    log_error( TO_STR(EXTERNAL_PROCESS_NAME)
               << "_wrapper: Invalid pointer passed for object"
               << vcl_endl );
    return false;
  }

  return obj->initialize();
}


// Set tracker inputs
EXTERNAL_PROCESS_API
bool
set_inputs(vidtk::external_base_process* obj, const vidtk::external_base_process::data_map_type &inputs)
{
  if( !obj )
  {
    log_error( TO_STR(EXTERNAL_PROCESS_NAME)
               << "_wrapper: Invalid pointer passed for object"
               << vcl_endl );
    return false;
  }

  obj->set_inputs(inputs);
  return true;
}


// Execute the tracking algorithm for 1 step
EXTERNAL_PROCESS_API
bool
step(vidtk::external_base_process* obj)
{
  if( !obj )
  {
    log_error( TO_STR(EXTERNAL_PROCESS_NAME)
               << "_wrapper: Invalid pointer passed for object"
               << vcl_endl );
    return false;
  }

  return obj->step();
}


// Retrieve tracker output
EXTERNAL_PROCESS_API
bool
get_outputs(vidtk::external_base_process* obj, vidtk::external_base_process::data_map_type *& outputs)
{
  if( !obj )
  {
    log_error( TO_STR(EXTERNAL_PROCESS_NAME)
               << "_wrapper: Invalid pointer passed for object"
               << vcl_endl );
    return false;
  }

  outputs = &obj->outputs();
  return true;
}


// Destroy the tracker object
EXTERNAL_PROCESS_API
void 
destruct(vidtk::external_base_process *obj)
{
  if( !obj )
  {
    log_error( TO_STR(EXTERNAL_PROCESS_NAME)
               << "_wrapper: Invalid pointer passed for object"
              << vcl_endl );
    return;
  }

  delete obj;
}


#ifdef __cplusplus
}
#endif
