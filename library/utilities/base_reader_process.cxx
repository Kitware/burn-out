/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "base_reader_process.h"

#include <logger/logger.h>

namespace vidtk
{


  VIDTK_LOGGER("base_reader_process");

base_reader_process::
  base_reader_process(std::string const& _name,  std::string const& type)
    : base_io_process(_name, type)
{
}


base_reader_process::~base_reader_process()
{
}


// ----------------------------------------------------------------
/** Initialize process.
 *
 *
 */
bool base_reader_process::
initialize_internal()
{
  return true;
}


// ----------------------------------------------------------------
/** Open output file.
 *
 * This method handles opening the output file. We must allocate a new
 * file object of the correct type.
 */
void base_reader_process::
  open_file()
{
  file_stream().open(this->m_filename.c_str(), std::ofstream::in);
}


// ----------------------------------------------------------------
/** Main process step.
 *
 *
 */
int base_reader_process::
  do_file_step()
{
  int status = m_dataGroup.read_object (file_stream());
  if (status != 0)
  {
    LOG_DEBUG ("base_reader_process: read group returned " << status );
  }

  // test for error/eof
  if (-1 == status)
  {
    // let derived class handle the error
    status = error_hook (status);
  }

  return status;
}


// ----------------------------------------------------------------
/** Error handler hook
 *
 * This method is called when an error is detected in reading the
 * input file. The actual type of the error can be determined by
 * examining the state of the stream by calling file_stream().
 *
 * @param[in] status - error code
 *
 * @retval 0 - ignore error
 * @retval != 0 - honor the error and terminate process
 */
int base_reader_process::
error_hook( int status)
{
  return status;
}

} // end namespace vidtk

