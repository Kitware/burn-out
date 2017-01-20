/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_super_process_h_
#define vidtk_homography_super_process_h_

#include <kwklt/klt_track.h>

#include <pipeline_framework/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vil/vil_pyramid_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>

#include <tracking_data/shot_break_flags.h>

namespace vidtk
{

class homography_super_process_impl;
class shot_break_flags;

class homography_super_process
  : public super_process
{
public:
  typedef homography_super_process self_type;

  homography_super_process( std::string const& name );

  virtual ~homography_super_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual process::step_status step2();

  void set_timestamp( timestamp const& );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  /// Set the input image
  /// \note This port should be connected only if using the GPU tracker type
  void set_image( vil_image_view<vxl_byte> const& );
  VIDTK_INPUT_PORT( set_image, vil_image_view<vxl_byte> const& );

  /// Set the input image pyramid
  /// \note This port should be connected only if using the KLT tracker type
  void set_image_pyramid( vil_pyramid_image_view<float> const& );
  VIDTK_INPUT_PORT( set_image_pyramid, vil_pyramid_image_view<float> const& );

  /// Set the input image pyramid of horizontal gradients
  /// \note This port should be connected only if using the KLT tracker type
  void set_image_pyramid_gradx( vil_pyramid_image_view<float> const& );
  VIDTK_INPUT_PORT( set_image_pyramid_gradx, vil_pyramid_image_view<float> const& );

  /// Set the input image pyramid of vertical gradients
  /// \note This port should be connected only if using the KLT tracker type
  void set_image_pyramid_grady( vil_pyramid_image_view<float> const& );
  VIDTK_INPUT_PORT( set_image_pyramid_grady, vil_pyramid_image_view<float> const& );

  /// The initial frameN0 -> frameN1 homography estimate
  void set_homog_predict( vgl_h_matrix_2d<double> const& f2f );
  VIDTK_OPTIONAL_INPUT_PORT( set_homog_predict, vgl_h_matrix_2d<double> const&);

  void set_mask( vil_image_view<bool> const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_mask, vil_image_view<bool> const& );

  /// The output homography from source (current) to reference image.
  image_to_image_homography src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, src_to_ref_homography );

  image_to_image_homography ref_to_src_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, ref_to_src_homography );

  std::vector< klt_track_ptr > active_tracks() const;
  VIDTK_OUTPUT_PORT( std::vector< klt_track_ptr >, active_tracks );

  vidtk::shot_break_flags get_output_shot_break_flags ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::shot_break_flags, get_output_shot_break_flags );

private:
  homography_super_process_impl * impl_;
};

} // end namespace vidtk

#endif // vidtk_homography_super_process_h_
