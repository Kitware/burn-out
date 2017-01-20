/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_color_commonality_filter_process_h_
#define vidtk_color_commonality_filter_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <video_transforms/color_commonality_filter.h>

namespace vidtk
{

/// Quickly filters an input RGB or grayscale image, highlighting colors
/// which are less common in the image (on a per-pixel) basis for the output
/// image. Input must be 8-bit at this time due to optimizations.
///
/// The output image is of the same type as the input, where lower values
/// correspond to colors which are less common in the input.
template< class InputType, class OutputType >
class color_commonality_filter_process
  : public process
{

public:

  typedef color_commonality_filter_process self_type;

  color_commonality_filter_process( std::string const& name );
  virtual ~color_commonality_filter_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( vil_image_view<InputType> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<InputType> const& );

  void set_source_border( vgl_box_2d<int> const& rect );
  VIDTK_INPUT_PORT( set_source_border, vgl_box_2d<int> const& );

  vil_image_view<OutputType> filtered_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<OutputType>, filtered_image );

private:

  // Inputs
  /// @todo Remove pointers to input data
  vil_image_view<InputType> const* input_;
  vgl_box_2d<int> const* border_;

  // Possible Outputs
  vil_image_view<OutputType> output_image_;

  // Internal parameters/settings
  color_commonality_filter_settings filter_settings_;
  config_block config_;
  unsigned color_resolution_;
  unsigned color_resolution_per_chan_;
  unsigned intensity_resolution_;
  bool smooth_image_;

  // Internal buffers
  std::vector<unsigned> color_histogram_;
  std::vector<unsigned> intensity_histogram_;
};


} // end namespace vidtk


#endif // vidtk_color_commonality_filter_process_h_
