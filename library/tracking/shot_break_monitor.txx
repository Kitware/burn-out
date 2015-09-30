/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "shot_break_monitor.h"

#include <utilities/unchecked_return_value.h>
#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("shot_break_monitor");


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
template< class PixType >
shot_break_monitor< PixType >
::shot_break_monitor( vcl_string const & name )
  : process( name, "shot_break_monitor" ),
    last_modality_(vidtk::VIDTK_INVALID),
    disabled_(true),
    fov_change_threshold_(0),
    end_of_shot_(false),
    modality_change_(false),
    out_video_modality_(vidtk::VIDTK_INVALID),
    first_frame_only_(false)
{
  this->config_.add_parameter( "disabled", "true",
    "If enabled, will reset on video modality change and shot breaks. "
    "If disaled, will pass all inputs to outputs." );

  this->config_.add_parameter( "fov_change_threshold", "0.125",
    "Threshold (in meters-per-pixel) that decides between Medium Field of "
    "View (MFOV) and Narrow Field of View (NFOV), i.e. MFOV above and NFOV "
    "at or below the threshold." );

  this->config_.add_parameter( "first_frame_only", "false",
    "If set, only inspect the first frame for EO/IR classification.");

  // incorporate eo-ir parameters into ours.
  this->config_.add_subblock( eo_ir_oracle.params(), "eo_ir_detect");

  reset_save_area();
}


template< class PixType >
shot_break_monitor< PixType >
::~shot_break_monitor()
{

}


// ----------------------------------------------------------------
/** Reset internal values.
 *
 * This method is called before restarting the pipeine after a
 * failure.
 */
template< class PixType >
bool
shot_break_monitor< PixType >
::reset()
{
  LOG_INFO (name() << ": reset() called");
  this->modality_change_ = false;
  this->end_of_shot_ = false;

  return true;
}


// ----------------------------------------------------------------
/** Collect parameters from this process.
 *
 * This method returns a bonfig block that contains all the
 * parameters needed by this process.
 *
 * @return Config block with all parameters.
 */
template< class PixType >
config_block
shot_break_monitor< PixType >
::params() const
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
template< class PixType >
bool shot_break_monitor< PixType >
::set_params(config_block const& blk)
{
  try {
    disabled_ = blk.get< bool >("disabled");

    if( !disabled_ )
    {
      blk.get("fov_change_threshold", fov_change_threshold_);
      blk.get("first_frame_only", first_frame_only_);

      // Pass parameters to eo-ir oracle
      if( !eo_ir_oracle.set_params( blk.subblock("eo_ir_detect") ) )
        return ( false );
    }
  }
  catch(unchecked_return_value & e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );

    return ( false );
  }

  return ( true );
}


// ----------------------------------------------------------------
/** Initialize this process.
 *
 * This initialize method is called once after the parameters
 * have been supplied. It is our duty to initialize this process
 * and make it ready to operate.
 */
template< class PixType >
bool
shot_break_monitor< PixType >
::initialize()
{
  return (true);
}


// ----------------------------------------------------------------
/** Main processing method.
 *
 * This method is called once per iteration of the pipeline.
 */
template< class PixType >
process::step_status
shot_break_monitor< PixType >
::step2()
{
  if( !this->timestamp_data.is_valid() )
  {
    LOG_WARN( this->name() << ": Did not receive valid input data, failing." );
    return process::FAILURE;
  }

  LOG_INFO (name() << ": input - " << this->shot_break_flags_data << " "
            << this->timestamp_data
            << (disabled_ ? " (disabled)" : " (enabled)" ) );

  if ( disabled_)
  {
    return process::SUCCESS;
  }

  process::step_status retcode = process::SUCCESS;

  // If we are to process only the first frame, clear the enable so
  // the next time we will just return the last calculated modality.
  if (first_frame_only_)
  {
    disabled_ = true;
  }

  // If there is valid data in the saved slot, flush out.
  if (timestamp_data_save.is_valid() )
  {
    // The video modality has already been updated to the new mode
    // that caused the restart.
    swap_inputs();
    LOG_INFO (name() << ": restoring saved frame "<< timestamp_data);
    push_output (process::SUCCESS);
    swap_inputs();
    reset_save_area(); // make sure we do this only once
  }

  // Determine eo/ir attribute
  if ((src_to_ref_homography_data.is_valid() && src_to_ref_homography_data.is_new_reference())
    || first_frame_only_)
  {
    // use GSD to determine full modality
    this->out_video_modality_ = compute_modality( world_units_per_pixel_data);

    // Check to see if we have changed modes
    if ( (this->out_video_modality_ != vidtk::VIDTK_INVALID )
         && ( this->last_modality_ != this->out_video_modality_ ) )
    {
      LOG_INFO (name() << ": Modality change, causing restart, "
                << this->last_modality_ << " -> "
                << this->out_video_modality_ << "  "
                << this->timestamp_data);

      this->modality_change_ = true;

      // Save current frame data so we can output it after reset.  The
      // new video modality will persist over the restart
      save_inputs();

      // Need to reset based on new video mode encountered.
      // Do this before processing first frame of new mode.
      retcode = process::FAILURE;
    }
    else
    {
      this->modality_change_ = false;
    }
  }

  // update on every frame
  this->last_modality_ = this->out_video_modality_;

  //
  // Test for end of shot
  //
  end_of_shot_ = shot_break_flags_data.is_shot_end();
  if (end_of_shot_)
  {
    LOG_INFO (name() << ": End of shot detected, causing restart "
              << this->timestamp_data);
    retcode = process::FAILURE; // cause pipeline reset
  }

  return retcode;
}


// ----------------------------------------------------------------
/** Compute video modality.
 *
 *
 * @param[in] new_gsd - gsd scaling value
 *
 *  @return New Video Modality value.
 */
template< class PixType >
video_modality shot_break_monitor< PixType >
::compute_modality( double new_gsd )
{
  video_modality mode;
  bool is_eo = eo_ir_oracle.is_eo( this->image_data );

  if( new_gsd > fov_change_threshold_ && is_eo )
  {
    mode = VIDTK_EO_M;
  }
  else if( new_gsd <= fov_change_threshold_ && is_eo )
  {
    mode = VIDTK_EO_N;
  }
  else if( new_gsd > fov_change_threshold_ && !is_eo )
  {
    mode = VIDTK_IR_M;
  }
  else if( new_gsd <= fov_change_threshold_ && !is_eo )
  {
    mode = VIDTK_IR_N;
  }
  else
  {
    // Should never happen. Only using to fix a compiler warning.
    mode = VIDTK_INVALID;
  }

  return mode;
} // compute_modality()


// ----------------------------------------------------------------
// -- special access methods --
template< class PixType >
bool shot_break_monitor< PixType >
::is_end_of_shot()
{
  return this->end_of_shot_;
}


template< class PixType >
bool shot_break_monitor < PixType >
::is_modality_change()
{
  return this->modality_change_;
}


template< class PixType >
vidtk::video_modality shot_break_monitor < PixType >
::current_modality() const
{
  return this->out_video_modality_;
}


// ----------------------------------------------------------------
/** Save inputs in our save slot.
 *
 *
 */
template< class PixType >
void shot_break_monitor < PixType >
::save_inputs()
{
#define CONNECTION_DEF(TYPE, BASENAME)          \
  BASENAME ## _data_save =  BASENAME ## _data;

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace

}


// ----------------------------------------------------------------
/** Swap input and saved data areas.
 *
 *
 */
template< class PixType >
void shot_break_monitor < PixType >
::swap_inputs()
{
#define CONNECTION_DEF(TYPE, BASENAME)          \
  TYPE BASENAME ## _temp = BASENAME ## _data;   \
  BASENAME ## _data =  BASENAME ## _data_save;  \
  BASENAME ## _data_save =  BASENAME ## _temp;

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace

}


template< class PixType >
void shot_break_monitor < PixType >
::reset_save_area()
{
  timestamp_data_save = timestamp();
}


} // end namespace
