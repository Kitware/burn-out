/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "base_io_process.h"

#include <utilities/unchecked_return_value.h>
#include <logger/logger.h>

namespace vidtk
{

  VIDTK_LOGGER("base_io_process");


base_io_process::
  base_io_process(vcl_string const& name,  vcl_string const& type)
    : process(name, type),
      m_appendFile(false),
      m_enabled(false)
{
  m_configBlock.add_parameter(
    "enabled",
    "false",
    "Do not do anything. Will read or write the file when set to true");

  m_configBlock.add_parameter(
    "filename",
    "Name for data file");

  m_configBlock.add_parameter(
    "append", "false", "Append to existing file if true; overwrite if false");
}


base_io_process::
~base_io_process()
{
  if ( this->file_stream().is_open() )
  {
    file_stream().close();
  }
}


// ----------------------------------------------------------------
/** Return required configuration parameters.
 *
 *
 */
config_block base_io_process::
  params() const
{
  return m_configBlock;
}


// ----------------------------------------------------------------
/** Handle configured values.
 *
 * This method reads the configuration and sets our internal values
 * based on what is found.
 */
bool base_io_process::
  set_params(config_block const& blk)
{
  try
  {
    blk.get("enabled", this->m_enabled);
    if ( this->m_enabled )
    {
      blk.get("filename", this->m_filename);
      blk.get("append", this->m_appendFile);
    }
  }
  catch (const unchecked_return_value & e)
  {
    LOG_ERROR(name() << ": couldn't set parameters: " << e.what());
    return false;
  }

  // update our local values to those passed in.
  m_configBlock.update(blk);
  return true;
}


// ----------------------------------------------------------------
/** Initialize process.
 *
 * This method is called after all parameters have been extracted by
 * this object.
 */
bool base_io_process::
initialize()
{
  if ( this->m_enabled && !this->m_filename.empty() )
  {

    this->open_file();
    if ( this->file_stream().fail() )
    {
      LOG_ERROR("Could not open file: " + this->m_filename );
      return false;
    }
  }

  // init most derived classes
  return  initialize (INTERNAL);
}


// ----------------------------------------------------------------
/** Main process step.
 *
 * @retval true - continue
 * @retval false - terminate process

 */
bool base_io_process::
  step()
{
  if ( ! this->m_enabled )
  {
    // we are happy to do nothing.
    return true;
  }

  if ( ! this->file_stream().is_open() )
  {
    return false;
  }

  int status(0);

  // In order to support the various protocols needed by the actual
  // process in the pipeline, we provide the pre/post hooks.
  status = pre_step_hook();
  if (0 != status)
  {
    return false;
  }

  // delegate further processing to derived class
  status = do_file_step();
  if (status != 0)
  {
    LOG_DEBUG ("base_io_process: file step returned " << status );
  }

  post_step_hook();

  return (0 == status);
}


// ----------------------------------------------------------------
/** Return our output stream.
 *
 *
 */
vcl_fstream & base_io_process::
file_stream()
{
  return this->m_fileStream;
}


// ----------------------------------------------------------------
/** Return config block.
 *
 *
 */
vidtk::config_block& base_io_process::
get_config()
{
  return m_configBlock;
}


// ----------------------------------------------------------------
/** Get reader writer group object.
 *
 *
 */
group_data_reader_writer * base_io_process::
get_group()
{
  return &this->m_dataGroup;
}


// ----------------------------------------------------------------
/** Add data reader/writer.
 *
 *
 */
base_reader_writer * base_io_process::
add_data_io (base_reader_writer const& obj)
{
  return this->m_dataGroup.add_data_reader_writer(obj);
}


// ----------------------------------------------------------------
/** Write all registered objects to stream
 *
 *
 */
void base_io_process::
write_all_objects(vcl_ostream& str)
{
  this->m_dataGroup.write_object (str);
}

// Read inputs
int base_io_process::
read_all_objects(vcl_istream& str)
{
  return this->m_dataGroup.read_object (str);
}

// Write headers
void base_io_process::
write_all_headers(vcl_ostream& str)
{
  this->m_dataGroup.write_header (str);
}


vcl_string const& base_io_process::
get_file_name() const
{
  return m_filename;
}


// ----------------------------------------------------------------
/** Pre step hook.
 *
 * This method is called just before the do_file_step() method to
 * allow the most derived class an option of doing something.
 *
 * This in conjunction with the post_step_hook() can be used by the
 * most derived class to implement any state changes needed to comply
 * with the larger pipeline protocol.
 *
 * @retval 0 - continue with the step
 * @retval != 0 - abort the step
 */
int base_io_process::
pre_step_hook()
{
  return 0;
}


// ----------------------------------------------------------------
/** Post step hook.
 *
 * This method is called jsut after the do_file_step() method to allow
 * the most derived class an option of doing something.
 *
 * This in conjunction with the pre_step_hook() can be used by the
 * most derived class to implement any state changes needed to comply
 * with the larger pipeline protocol.
 */
void base_io_process::
post_step_hook()
{

}



} // end namespace vidtk
