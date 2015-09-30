/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline/super_process.h>

#include <pipeline/async_pipeline_node.h>
#include <pipeline/async_pipeline.h>
#include <pipeline/sync_pipeline.h>

#include <vul/vul_file.h>


namespace vidtk
{

  // Storage for video file base name
  vcl_string super_process::s_video_basename;


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
super_process
::super_process( vcl_string const& name, vcl_string const& class_name )
  : process( name, class_name ),
    pipeline_( NULL )
{
}


/// Returns the current configuration parameters.
config_block
super_process
::params() const
{
  return pipeline_->params();
}


/// Allows the user to set the configuration parameters.
bool
super_process
::set_params( config_block const& block )
{
  return pipeline_->set_params( block );
}


/// Initializes the module.
bool
super_process
::initialize()
{
  return pipeline_->initialize();
}


bool
super_process
::reset()
{
  return pipeline_->reset();
}


bool
super_process
::cancel()
{
  log_info (name() << ": super_process: cancel() called\n"); // TEMP
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
    while( ap->run_with_pads(parent) );
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
delete_super_process_object( super_process* p )
{
  delete p;
}


void super_process
::set_videoname_prefix (vcl_string const& name)
{
  // extract video base name from full input path and file specification.
  s_video_basename = vul_file::strip_directory(
    vul_file::strip_extension( name ) );
}


// ----------------------------------------------------------------
/** Add video name to file name.
 *
 * This method adds the name of the current video being processed as a
 * prefix to the specified config item key.  The string in the config
 * block is modified in place by prefixing the video name.
 *
 * If the string from the config block is empty or the video name
 * prefix is empty, then the config entry is not modified.
 *
 * The main purpose for this operation is to generate unique file
 * names for each video, so the config string is processeed as a file
 * path.
 *
 * @param[in] block - config block to look in for the key
 * @paramin] key - config item name (key) to be modified
 *
 * @retval true - if key was found
 * @retval false - key not found
 */
checked_bool super_process
::add_videoname_prefix( config_block& block, vcl_string const& key )
{
  if( block.has( key ) )
  {
    vcl_string const& oldName = block.get<std::string>( key );
    if ( oldName.empty() || s_video_basename.empty() )
    {
      return (true);
    }

    vcl_string newName = vul_file::dirname(oldName) + "/"
      + this->s_video_basename + "_" + vul_file::strip_directory(oldName);
    block.set( key, newName );

    return (true);
  }
  else
  {
    return "Did not find config key \"" + key + "\" to prefix with video name.";
  }
} // bool add_videoname_prefix()


vcl_string const& super_process
::get_videoname_prefix() const
{
  return this->s_video_basename;
}

void
super_process
::set_print_detailed_report( bool print )
{
  pipeline_->set_print_detailed_report( print );
}



} // end namespace vidtk
