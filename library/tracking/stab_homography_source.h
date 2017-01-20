/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_STAB_HOMOGRAPHY_SOURCE_H_
#define _VIDTK_STAB_HOMOGRAPHY_SOURCE_H_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>

#include <utilities/homography.h>
#include <utilities/timestamp.h>


namespace vidtk
{

// ----------------------------------------------------------------
/** Create new identity homography.
 *
 * This process creates a stream of valid src_to_ref homographies that
 * have the identity transform. The dest reference is set to the first
 * timestamp seen and the src is set to the current timestamp.
 *
 * It seems that there should be a better way to create this type of
 * light weight process, but all the alternatives involved more
 * overhead than benefit.
 *
 * - Base class process and derived processes to create homographies
 *   with desired properties.
 *
 * - Factory class with delegate to create homographies with desired
 *   properties.
 *
 * The only hope is to wait for a croup of homography generators to
 * form and then refactor into the form that provides most benefit.
 */
class stab_homography_source
  : public process
{
public:
  typedef stab_homography_source self_type;

  stab_homography_source(std::string const& name );
  virtual ~stab_homography_source();

  // -- process interface --
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  // -- process inputs --
  VIDTK_INPUT_PORT ( input_timestamp, vidtk::timestamp const& );
  void input_timestamp( vidtk::timestamp const& ts );

  // -- process outputs --
  VIDTK_OUTPUT_PORT ( vidtk::image_to_image_homography, output_src_to_ref_homography );
  vidtk::image_to_image_homography output_src_to_ref_homography();

private:
  config_block config_;

  vidtk::timestamp first_ts_;

  // -- inputs --
  vidtk::timestamp input_ts_;

  // -- outputs --
  vidtk::image_to_image_homography out_src_to_ref_;


}; // end class stab_homography_source

} // end namespace

#endif /* _VIDTK_STAB_HOMOGRAPHY_SOURCE_H_ */
