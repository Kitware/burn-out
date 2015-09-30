/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_difference_process2_h_
#define vidtk_image_difference_process2_h_

#include <vcl_vector.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <tracking/fg_image_process.h>
#include <utilities/buffer.h>
#include <utilities/config_block.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

/// Find moving object by pixelwise comparison of registered images.
/// This process will allocate new memory for each output image on
/// every frame.
template < class PixType >
class image_difference_process2
  : public process
{
public:
  typedef image_difference_process2 self_type;

  image_difference_process2( vcl_string const& name );
  ~image_difference_process2();
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  void set_first_frame( vil_image_view<PixType> const& image );
  VIDTK_INPUT_PORT( set_first_frame, vil_image_view<PixType> const& );

  void set_second_frame( vil_image_view<PixType> const& image );
  VIDTK_INPUT_PORT( set_second_frame, vil_image_view<PixType> const& );

  void set_third_frame( vil_image_view<PixType> const& image );
  VIDTK_INPUT_PORT( set_third_frame, vil_image_view<PixType> const& );

  virtual vil_image_view<bool> const& fg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image );

  virtual vil_image_view<float> const& diff_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<float>, diff_image );

  virtual vil_image_view<bool> const& fg_image_zscore() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image_zscore );

  virtual vil_image_view<float> const& diff_image_zscore() const;
  VIDTK_OUTPUT_PORT( vil_image_view<float>, diff_image_zscore );

  /// See declaration of unprocessed_mask_ for comments. 
  virtual vil_image_view<bool> const& unprocessed_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, unprocessed_mask );

protected:
  bool validate_input();

  void compute_zscore() const;

  void compute_difference_image( vil_image_view<PixType> const& A,
                                 vil_image_view<PixType> const& B,
                                 vil_image_view<PixType> const& C );

  void suppress_nan_regions_from_diff( vil_image_view<PixType> const& A );

  config_block config_;

  /// current input images.
  vil_image_view<PixType> stab_frame_1_;
  vil_image_view<PixType> stab_frame_2_;
  vil_image_view<PixType> stab_frame_3_;

  /// current difference image.
  vil_image_view<float> diff_image_;

  /// Thresholded foreground image
  mutable bool fg_image_is_valid_;
  mutable vil_image_view<bool> fg_image_abs_;

  mutable bool fg_image_zscore_is_valid_;
  mutable vil_image_view<float> diff_zscore_;
  mutable vil_image_view<bool> fg_image_zscore_;

  /// A binary mask that records pixels (w/ true value) that are not supposed to 
  /// produce valid motion segmentation. This is mainly due to unmapped pixels 
  /// during image warping.
  vil_image_view<bool> unprocessed_mask_;
  bool compute_unprocessed_mask_;

  // Parameters
  unsigned spacing_;
  float threshold_;
  float z_threshold_;
  float z_radius_;
  int nan_value_;
  int conflict_value_;
  bool use_conflict_value_;
  unsigned jitter_delta_[2];

  enum {DIFF_ABSOLUTE, DIFF_ZSCORE, DIFF_ADAPTIVE_ZSCORE} diff_type_;

};

} // namespace vidtk

#endif
