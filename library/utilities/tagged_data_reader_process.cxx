/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "tagged_data_reader_process.h"

#include <utilities/gsd_reader_writer.h>
#include <utilities/timestamp_reader_writer.h>
#include <utilities/homography_reader_writer.h>
#include <utilities/video_metadata_reader_writer.h>
#include <utilities/video_metadata_vector_reader_writer.h>
#include <utilities/video_modality_reader_writer.h>
#include <utilities/shot_break_flags_reader_writer.h>
#include <utilities/image_view_reader_writer.h>
#include <logger/logger.h>


namespace vidtk
{
VIDTK_LOGGER("tagged_data_reader_process");


// ----------------------------------------------------------------
/** Local reader policy class.
 *
 * Our policy is to allow unlimited unrecognised input lines and
 * unlimited missing inputs.
 */
class reader_policy
  : public group_data_reader_writer::mismatch_policy
{
public:
  reader_policy() {}
  virtual ~reader_policy() {}

  virtual bool unrecognized_input (const char * line)
  {
    LOG_INFO ("Unrecognised input: " << line );
    return true; // allow extra input
  }


  virtual int missing_input (group_data_reader_writer::rw_vector_t const& objs)
  {
    LOG_INFO ("Input not found for " << objs.size() << " data types" );
    return 0;  // Allow all missing input
  }

}; // end class




// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
tagged_data_reader_process::
tagged_data_reader_process(vcl_string const& name)
  : base_reader_process(name, "tagged_data_reader_process"),
    m_state(0)
{
  VIDTK_DEFAULT_LOGGER->set_level (vidtk_logger::LEVEL_TRACE);
  this->get_group()->set_policy(new reader_policy);
}


tagged_data_reader_process::
~tagged_data_reader_process()
{

}


bool tagged_data_reader_process::
set_params(config_block const& blk)
{
  // Get parameters from the base class first
  if (base_reader_process::set_params (blk) != true)
  {
    return (false);
  }

  return (true);
}


bool tagged_data_reader_process::
initialize(base_io_process::internal_t)
{
  if (! m_enabled)
  {
    return (true); // to consume our inputs
  }

  LOG_INFO (name() << ": Reading from " << get_file_name() );

  // add readers
#define MDRW_INPUT(N,T,W,I)                                             \
  {                                                                     \
    vidtk::base_reader_writer * reader =  add_data_io (vidtk::W (&m_ ## N) ); \
    if (I != 0)  reader->add_instance_name (I);                         \
  }

  MDRW_SET_ONE  // apply macro over inputs
#undef MDRW_INPUT

// Now do the images -- This is not ideal
  // add readers
#define MDRW_INPUT(N,T,W,I)                                             \
 {                                                                      \
    vidtk::base_reader_writer * reader =  add_data_io (vidtk::W (&m_ ## N) ); \
    reader->add_instance_name (I);                                      \
    W * iw = dynamic_cast< W * >(reader);                               \
    iw->set_image_file_name( get_file_name() );                         \
}

  MDRW_SET_IMAGE  // apply macro over inputs
#undef MDRW_INPUT


  // initialize the base class first
  if (base_reader_process::initialize(INTERNAL) != true)
  {
    return (false);
  }

  return (true);
}


//
// We need a simple state machine to finish outputting all data
// and then emit an invalid timestamp to signal end of video EOV
//
// The following table shows the state transitions. Note that the
// hooks are called in order: pre, err, post. err is skipped if no
// error.

//      | s(0)      |    s(1)  |    s(2)     |    s(3)   |
// --------------------------------------------------------
// pre  |           |          |             | terminate |
// --------------------------------------------------------
// err  |  s = s(1) |          |             |           |
// --------------------------------------------------------
// post |           | s = s(2) |  force t/s  |           |
//      |           |          |  s = s(3)   |           |
// --------------------------------------------------------




// ----------------------------------------------------------------
/** Error handler hook.
 *
 * This is called when a file based error is detected, such as EOF.
 * Status values are positive if there is a data tag error, negative
 * if i/o error and zero if there is no error. (We should not see a
 * zero.)
 *
 * @param[in] status - error code
 */
int tagged_data_reader_process::
error_hook( int /*status*/)
{

  if (0 == m_state)
  {
    m_state = 1; // move to next state, initiating termination sequence
  }

  // proceed normally
  return 0;
}


int tagged_data_reader_process::
pre_step_hook()
{
  if (3 == m_state)
  {
    return 1; // terminate process
  }

  return (0);
}


void tagged_data_reader_process::
post_step_hook()
{
  if (1 == m_state)
  {
    m_state++;
  }
  else if (2 == m_state)
  {
      LOG_DEBUG ("tagged_data_reader_process: setting invalid timestamp for EOF signal");
      // set to uninitialized values
      m_timestamp = vidtk::timestamp();

      m_state++;
  }
}


// ==== OUTPUT METHODS ======
// ----------------------------------------------------------------
/** Timestamp vector.
 *
 * Since we don't really handle the timestamp vector, we will create
 * one here from the last timestamp we have.
 */
vidtk::timestamp::vector_t
tagged_data_reader_process::
get_output_timestamp_vector() const
{
  vidtk::timestamp::vector_t tsv;

  tsv.push_back (m_timestamp);

  return tsv;
}



} // end namespace
