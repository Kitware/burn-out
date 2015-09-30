/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */



#include "kw_archive_writer_process.h"
#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER("kw_archive_writer_process");

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
  kw_archive_writer_process::
  kw_archive_writer_process(vcl_string const& name)
    : process(name, "kw_archive_writer_process"),
      m_enable(true),
      m_allow_overwrite(false),
      m_archive_writer(0)
  {
    m_config.add("disabled", "true");
    m_config.add("file_path", ".");
    m_config.add("file_name", "kw_archive");
    m_config.add_parameter( "overwrite_existing", "false",
                            "Weather or not a file can be overwriten." );
  }


  kw_archive_writer_process::
  ~kw_archive_writer_process()
  {
    delete m_archive_writer;
  }


// ----------------------------------------------------------------
/** Collect parameters from this process.
 *
 * This method returns a bonfig block that contains all the
 * parameters needed by this process.
 *
 * @return Config block with all parameters.
 */
  config_block kw_archive_writer_process::
  params() const
  {
    return (m_config);
  }


// ----------------------------------------------------------------
/** Set values for parameters.
 *
 * This method is called with an updated config block that
 * contains the most current configuration values. We will
 * query our parameters from this block.
 *
 * @param[in] blk - updated config block
 */
  bool kw_archive_writer_process::
  set_params( config_block const& blk)
  {
    blk.get("enabled", m_enable);
    blk.get("file_path", m_file_path);
    blk.get("file_name", m_file_name);
    blk.get( "overwrite_existing", m_allow_overwrite );

    return (true);
  }


// ----------------------------------------------------------------
/** Initialize this process.
 *
 * This initialize method is called once after the parameters
 * have been supplied. It is our duty to initialize this process
 * and make it ready to operate.
 */
  bool kw_archive_writer_process::
  initialize()
  {
    if ( ! m_enable)
    {
      return (true);
    }

    if (m_file_name.empty())
    {
      LOG_ERROR("Required base file name is missing");
      return (false);
    }

    // Create and initialize the real writer
    this->m_archive_writer = new vidtk::kw_archive_writer();

    bool status = this->m_archive_writer->set_up_files (m_file_path, m_file_name, m_allow_overwrite);
    if ( ! status)
    {
      LOG_ERROR ("Could not set up output files.");
    }

    return status;
  }


// ----------------------------------------------------------------
/** Main processing method.
 *
 * This method is called once per iteration of the pipeline.
 */
  bool kw_archive_writer_process::
  step()
  {
    // Need to return true in case we are in an async pipeline.
    // Need to consume inputs so source node will not block.
    if ( ! m_enable )
    {
      return (true);
    }

    // Test for invalid input - means we have reached end of video
    if (! m_data_set.m_timestamp.is_valid() )
    {
      LOG_DEBUG(name() << ": invalid time stamp seen - assume EOF"); // TEMP
      return (false);
    }

    if ( ! m_data_set.m_timestamp.has_time())
    {
      LOG_WARN ("Must have valid time in timestamp input.");
      return (true);
    }

    // check for valid src -> utm homography
    if (! !m_data_set.m_src_to_utm_homog.is_valid())
    {
      // src2utm not available, then try to make one from wld2utm and src2wld
      if ( m_wld_to_utm_h.is_valid() && m_src_to_wld_h.is_valid() )
      {
        this->m_data_set.m_src_to_utm_homog = m_wld_to_utm_h * m_src_to_wld_h;
      }
      else
      {
        LOG_WARN ("Must have either src_to_utm or (src_to_wld and wld_to_utm)");
        return (true);
      }
    }

    // Write out one frame of data
    this->m_archive_writer->write_frame_data (this->m_data_set);

    // Set to known invalid state.
    m_data_set.clear();
    return (true);
  }


// ================================================================
// -- inputs --
void kw_archive_writer_process::
set_input_timestamp ( vidtk::timestamp const& val)
{
  m_data_set.m_timestamp = val;
}


void kw_archive_writer_process::
set_input_src_to_ref_homography ( vidtk::image_to_image_homography const& val)
{
  m_data_set.m_src_to_ref_homog = val;
}


void kw_archive_writer_process::
set_input_src_to_utm_homography ( vidtk::image_to_utm_homography const& val)
{
  m_data_set.m_src_to_utm_homog = val;
}


void kw_archive_writer_process::
set_input_src_to_wld_homography ( vidtk::image_to_plane_homography const& val)
{
  m_src_to_wld_h = val;
}


void kw_archive_writer_process::
set_input_wld_to_utm_homography ( vidtk::plane_to_utm_homography const& val)
{
  m_wld_to_utm_h = val;
}

void kw_archive_writer_process::
set_input_world_units_per_pixel ( double val )
{
  m_data_set.m_gsd = val;
}


void kw_archive_writer_process::
set_input_image ( vil_image_view < vxl_byte > const& val)
{
  m_data_set.m_image = val;
}


} // end namespace
