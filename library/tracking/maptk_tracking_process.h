/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_MAPTK_TRACKING_PROCESS_H_
#define VIDTK_MAPTK_TRACKING_PROCESS_H_

#include <string>

// VidTK includes
#include <kwklt/klt_track.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <tracking_data/shot_break_flags.h>
#include <utilities/config_block.h>
#include <utilities/homography.h>
#include <utilities/timestamp.h>

#include <vil/vil_image_view.h>


namespace vidtk
{


/// MAP-Tk stabilization tracking process
/**
 * Given successive frames, this process tracks feature points and computes
 * homography transformations from the current frame to a reference frame.
 */
class maptk_tracking_process
  : public process
{
public:
  typedef maptk_tracking_process self_type;

  maptk_tracking_process( std::string const &name );
  virtual ~maptk_tracking_process();

  // Inheritance overrides
  config_block params() const;
  bool set_params( config_block const& blk );
  bool initialize();
  bool reset();
  process::step_status step2();

  // Input Ports -------------------------------------------------------------

  /// Set input timestamp
  void set_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  /// Set the input image
  void set_image( vil_image_view<vxl_byte> const& img);
  VIDTK_INPUT_PORT( set_image, vil_image_view<vxl_byte> const& );

  /// Optionally set input image mask
  void set_mask( vil_image_view<bool> const& mask);
  VIDTK_OPTIONAL_INPUT_PORT( set_mask, vil_image_view<bool> const& );

  // Output Ports ------------------------------------------------------------

  /// The output homography from source (current) to reference image.
  image_to_image_homography src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, src_to_ref_homography );

  /// The output homography from reference to source (current) image.
  image_to_image_homography ref_to_src_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, ref_to_src_homography );

  /// This frame's active tracks
  std::vector< klt_track_ptr > active_tracks() const;
  VIDTK_OUTPUT_PORT( std::vector< klt_track_ptr >, active_tracks );

  /// Shot break flags for the current frame
  vidtk::shot_break_flags get_output_shot_break_flags ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::shot_break_flags, get_output_shot_break_flags );

private:
  // Private implementation
  class pimpl;
  pimpl *impl_;
};


}


#endif // VIDTK_MAPTK_TRACKING_PROCESS_H_
