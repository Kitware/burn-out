/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_demultiplexer_process.h"

#include <logger/logger.h>

using namespace vidtk;

namespace vidtk {


VIDTK_LOGGER("track_demultiplexer_process");


// set up to use the list of linker ports
#define PASS_THRU_PORTS(C) DEMUX_PASS_THRU_DATA_LIST(C)

#include <utilities/pass_thru_helper.h>

class track_demultiplexer_process::impl
  : public pass_thru_helper
{
};

// Define pass thru methods for process class
#define DEFINE_PASS_THRU_METHODS(T,N,D)                                 \
  void track_demultiplexer_process::input_ ## N( T const& val) { this->impl_->in_ ## N(val); } \
  T track_demultiplexer_process::output_ ## N() const { return this->impl_->output_slice_.N; }

  PASS_THRU_PORTS(DEFINE_PASS_THRU_METHODS)

#undef DEFINE_PASS_THRU_METHODS

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
track_demultiplexer_process::
track_demultiplexer_process ( std::string const& _name )
  : process ( _name, "track_demultiplexer_process" ),
    impl_( new impl )
{
  // currently no config entries

  initialize();
}


track_demultiplexer_process::
~track_demultiplexer_process ()
{

}


// ----------------------------------------------------------------
/** Collect parameters from this process.
 *
 * This method returns a config block that contains all the
 * parameters needed by this process.
 *
 * @return Config block with all parameters.
 */
config_block track_demultiplexer_process::
params() const
{
  return this->config_;
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
bool track_demultiplexer_process::
set_params( config_block const& blk )
{
  (void) blk;
  return true;
}


// ----------------------------------------------------------------
/** Reset process.
 *
 *
 */
bool track_demultiplexer_process::
reset()
{
  return this->initialize();
}


// ----------------------------------------------------------------
/** Initialize this process.
 *
 * This initialize method is called once after the parameters
 * have been supplied. It is our duty to initialize this process
 * and make it ready to operate.
 */
bool track_demultiplexer_process::
initialize()
{
  state_ = 0; // reset to operating state.
  return true;
}


// ----------------------------------------------------------------
/** Main processing method.
 *
 * This method is called once per iteration of the pipeline.
 */
process::step_status
track_demultiplexer_process::
step2()
{

  switch (this->state_)
  {
  case 0: // normal operating mode
  {
    // Test for lack of inputs.  This indicates that the inputs failed,
    // we are at end of video and need to flush all remaining tracks.
    if ( ! valid_inputs())
    {
      LOG_DEBUG( name() << ": End of inputs at: " << impl_->last_pass_thru_timestamp() );

      state_ = 1; // switch to pending terminate

      // See if the pass-thru queue is empty
      assert( impl_->pass_thru_helper_empty() );

      if( this->demux_.flush_tracks() )
      {
        LOG_DEBUG( name() << ": Active tracks flushed to terminated tracks" );
        // No need to pop_pass_thru_output because we are generating
        // an additional output cycle with no input data.
        this->output_timestamp_ = impl_->last_pass_thru_timestamp();
        impl_->last_pass_thru_output(); // repeat the last pass-thru output
        return process::SUCCESS;
      }
      else
      {
        LOG_DEBUG( name() << ": No active tracks to flush, terminating" );
        // There wasn't anything to flush, hence fail now and avoid the
        // extra step
        return process::FAILURE;
      }
    }


    impl_->push_pass_thru_input( input_timestamp_ ); // save pass through data

    reset_outputs();

    // cycle demux
    this->demux_.add_tracks_for_frame(input_tracks_, input_timestamp_);
    impl_->pop_pass_thru_output( input_timestamp_ );
    this->output_timestamp_ = this->input_timestamp_;

    reset_inputs();

    return process::SUCCESS;
  } // end state 0

  default:
  case 1: // last cycle we flushed remaining tracks. Need to terminate.
    LOG_DEBUG( name() << ": Terminate after flushing tracks on last step" );
    reset_outputs();
    return process::FAILURE;

  } // end switch
}

// ----------------------------------------------------------------
/** Reset process outputs.
 *
 * This method resets outputs to a state indicating that there are no
 * real outputs.
 */
void
track_demultiplexer_process::
reset_outputs()
{
  // Reset outputs
  this->demux_.reset_outputs();
  this->output_timestamp_ = vidtk::timestamp();
}


// ----------------------------------------------------------------
/** Reset all process inputs.
 *
 *
 */
void
track_demultiplexer_process::
reset_inputs()
{
  // reset inputs
  this->input_timestamp_ = vidtk::timestamp();
  this->input_tracks_.clear();
}


// ----------------------------------------------------------------
/** Are inputs valid
 *
 * @retval true - valid inputs
 * @retval false - invalid inputs
 */
bool
track_demultiplexer_process::
valid_inputs()
{
  return this->input_timestamp_.is_valid();
}


// ================================================================
// Input methods
void track_demultiplexer_process::
input_timestamp (vidtk::timestamp const&  value)
{
  this->input_timestamp_ = value;
}


void track_demultiplexer_process::
input_tracks (vidtk::track::vector_t const&  tracks)
{
  this->input_tracks_ = tracks;
}


// ================================================================
// Output methods
vidtk::timestamp
track_demultiplexer_process::
output_timestamp() const
{
  return this->output_timestamp_;
}


vidtk::track::vector_t
track_demultiplexer_process::
output_new_tracks() const
{
  return this->demux_.get_new_tracks();
}


vidtk::track::vector_t
track_demultiplexer_process::
output_updated_tracks() const
{
  return this->demux_.get_updated_tracks();
}


vidtk::track::vector_t
track_demultiplexer_process::
output_not_updated_tracks() const
{
  return this->demux_.get_not_updated_tracks();
}


vidtk::track::vector_t
track_demultiplexer_process::
output_terminated_tracks() const
{
  return this->demux_.get_terminated_tracks();
}


vidtk::track::vector_t
track_demultiplexer_process::
output_active_tracks() const
{
  return this->demux_.get_active_tracks();
}

} // end namespace
