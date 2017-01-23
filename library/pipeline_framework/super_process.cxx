/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline_framework/super_process.h>

#include <pipeline_framework/async_pipeline_node.h>
#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <vul/vul_file.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER("super_process");

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
super_process
::super_process( std::string const& _name, std::string const& _class_name )
  : process( _name, _class_name ),
    pipeline_( NULL )
{
}


/// Returns the current configuration parameters.
config_block
super_process
::params() const
{
  if ( 0 == pipeline_ )
  {
    return config_block();
  }

  return pipeline_->params();
}


/// Allows the user to set the configuration parameters.
bool
super_process
::set_params( config_block const& block )
{
  if ( 0 == pipeline_ )
  {
    return false;
  }

  return pipeline_->set_params( block );
}


/// Initializes the module.
bool
super_process
::initialize()
{
  if ( 0 == pipeline_ )
  {
    return false;
  }

  return pipeline_->initialize();
}


bool
super_process
::reset()
{
  if ( 0 == pipeline_ )
  {
    return false;
  }

  return pipeline_->reset();
}


bool
super_process
::cancel()
{
  LOG_INFO (name() << ": super_process: cancel() called" );

  if ( 0 == pipeline_ )
  {
    return false;
  }

  return pipeline_->cancel();
}


bool
super_process
::run(async_pipeline_node* parent)
{
  if( async_pipeline* ap = dynamic_cast<async_pipeline*>(this->pipeline_.ptr()) )
  {
    // Run this function as long as it returns true.
    // The true return value indicates recovery from internal failure
    // which should trigger the process to run again.
    while( ap->run_with_pads(parent) ) ;
    return true;
  }
  else if( dynamic_cast<sync_pipeline*>(this->pipeline_.ptr()) )
  {
    // run the parent node synchronously to handle input and output edges
    // in the parent pipeline.
    parent->run();
    return true;
  }
  return false;
}


pipeline_sptr
super_process
::get_pipeline() const
{
  return pipeline_;
}


void
super_process
::set_print_detailed_report( bool print )
{
  pipeline_->set_print_detailed_report( print );
}



} // end namespace vidtk
