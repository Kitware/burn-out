/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "tagged_data_writer_process.h"

#include <utilities/gsd_reader_writer.h>
#include <utilities/timestamp_reader_writer.h>
#include <utilities/homography_reader_writer.h>
#include <utilities/video_metadata_reader_writer.h>
#include <utilities/video_metadata_vector_reader_writer.h>
#include <utilities/video_modality_reader_writer.h>
#include <utilities/shot_break_flags_reader_writer.h>
#include <utilities/image_view_reader_writer.h>
#include <logger/logger.h>


VIDTK_LOGGER("tagged_data_writer_process");

namespace vidtk
{

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
tagged_data_writer_process::
tagged_data_writer_process(std::string const& _name)
  : base_writer_process(_name, "tagged_data_writer_process"),

#define MDRW_INPUT(N,T,W,I)                     \
    m_ ## N ## _enabled (false),                \
    m_ ## N ## _force_enabled (false),          \
    m_ ## N ## _connected (false),

  MDRW_SET  // apply macro over inputs
#undef MDRW_INPUT

    last(0)
{
  vidtk::config_block& config(get_config());


  // Fill in the config block
  // parameter name is N (name from table)
#define MDRW_INPUT(N,T,W,I)                                            \
  config.add_parameter ( #N, "false", "Write " #N " value to output");

  MDRW_SET  // apply macro over inputs
#undef MDRW_INPUT
}


tagged_data_writer_process::
~tagged_data_writer_process()
{

}


bool tagged_data_writer_process::
set_params(config_block const& blk)
{
  // Get parameters from the base class first
  if (base_writer_process::set_params (blk) != true)
  {
    return (false);
  }

  // Get config values
#define MDRW_INPUT(N,T,W,I)                                     \
  m_ ## N ## _enabled = blk.get<bool>( #N );                    \
  if (m_ ## N ## _enabled && ! m_ ## N ## _connected)           \
  {                                                             \
    LOG_WARN( #N " is enabled by config but not connected");   \
    m_ ## N ## _enabled = false;                                \
  }

  MDRW_SET  // apply macro over inputs
#undef MDRW_INPUT

  return (true);
}


bool tagged_data_writer_process::
initialize_internal()
{
  if (! m_enabled)
  {
    return (true); // to consume our inputs
  }


  // add writers
#define MDRW_INPUT(N,T,W,I)                                             \
  if (m_ ## N ## _enabled || m_ ## N ## _force_enabled)                 \
  {                                                                     \
    vidtk::base_reader_writer * writer = add_data_io (vidtk::W (&m_ ## N) ); \
    if (I != 0)  writer->add_instance_name (I);                         \
  }

  MDRW_SET_ONE  // apply macro over inputs
#undef MDRW_INPUT

  // add image writers -- this is not ideal. Need to supply an
  // additional file name.
#define MDRW_INPUT(N,T,W,I)                                             \
  if (m_ ## N ## _enabled || m_ ## N ## _force_enabled)                 \
  {                                                                     \
    vidtk::base_reader_writer * writer = add_data_io (vidtk::W (&m_ ## N) ); \
    writer->add_instance_name (I);                                      \
    W * iw = dynamic_cast< W * >(writer);                               \
    iw->set_image_file_name( get_file_name());                          \
  }

  MDRW_SET_IMAGE  // apply macro over inputs
#undef MDRW_INPUT

  // initialize the base class
  if (base_writer_process::initialize_internal() != true)
  {
    return (false);
  }

  return (true);
}

} // end name
