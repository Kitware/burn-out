/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "stab_homography_source.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER("stab_homography_source");

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
stab_homography_source::
stab_homography_source ( std::string const& n )
  : process ( n, "stab_homography_source" )
{
  out_src_to_ref_.set_valid( true );
}


stab_homography_source::
~stab_homography_source ()
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
config_block stab_homography_source::
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
bool stab_homography_source::
set_params( config_block const& blk )
{
  (void) blk;
  return true;
}


// ----------------------------------------------------------------
/** Initialize this process.
 *
 * This initialize method is called once after the parameters
 * have been supplied. It is our duty to initialize this process
 * and make it ready to operate.
 */
bool stab_homography_source::
initialize()
{
  this->first_ts_ = timestamp();
  return true;
}


// ----------------------------------------------------------------
/** Main processing method.
 *
 * This method is called once per iteration of the pipeline.
 */
process::step_status
stab_homography_source::
step2()
{
  // If no new timestamp presented, fail
  if ( ! input_ts_.is_valid() )
  {
    return vidtk::process::FAILURE;
  }

  out_src_to_ref_.set_new_reference( false );

  // do step processing here
  if ( ! first_ts_.is_valid() )
  {
    // save first ts
    first_ts_ = input_ts_;
    out_src_to_ref_.set_new_reference( true );
  }

  out_src_to_ref_.set_dest_reference( first_ts_ )
    .set_source_reference( input_ts_ )
    .set_identity( true );

  input_ts_ = vidtk::timestamp(); // reset input

  return vidtk::process::SUCCESS;
}


// ================================================================
// Input methods
void
stab_homography_source::
input_timestamp (vidtk::timestamp const& ts )
{
  this->input_ts_ = ts;
}


// ================================================================
// Output methods
vidtk::image_to_image_homography
stab_homography_source::
output_src_to_ref_homography()
{
  return this->out_src_to_ref_;
}

} // end namespace
