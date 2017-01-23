/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_properties_border_detection_process_h_
#define vidtk_video_properties_border_detection_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/ring_buffer.h>

#include <video_properties/border_detection.h>

#include <vil/vil_image_view.h>

namespace vidtk
{


/// \brief A process which contains a few different methods and options for
/// detecting borders within videos.
///
/// Standard operating modes include: white border detection, black border
/// detection, automatic mode, black-and-white mode, and fixed border mode.
template <typename PixType>
class border_detection_process
  : public process
{

public:

  typedef border_detection_process self_type;
  typedef vil_image_view<PixType> input_type;

  border_detection_process( std::string const& name );
  virtual ~border_detection_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  void set_source_color_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_source_color_image, vil_image_view<PixType> const& );

  void set_source_gray_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_source_gray_image, vil_image_view<PixType> const& );

  void set_reset_flag( bool flag );
  VIDTK_INPUT_PORT( set_reset_flag, bool );

  image_border_mask border_mask() const;
  VIDTK_OUTPUT_PORT( image_border_mask, border_mask );

  image_border border() const;
  VIDTK_OUTPUT_PORT( image_border, border );

private:

  // Numerical definitions of operating mode of filter - detect_solid_rect
  // indicates we are detect solid coloured rectangular borders, detect_bw
  // indicates we're going after pure black and white borders, which don't
  // have to be rectilinear.
  enum{ FIXED_MODE, DETECT_SOLID_RECT, DETECT_BW } mode_;

  // Inputs
  vil_image_view<PixType> color_image_;
  vil_image_view<PixType> gray_image_;
  bool reset_flag_;

  // Possible Outputs
  image_border detected_border_;
  vil_image_view<bool> output_mask_image_;

  // Internal parameters/settings
  bool disabled_;
  border_detection_settings< PixType > algorithm_settings_;
  config_block config_;
  unsigned min_border_dim_[4]; // { left, top, right, bottom }
  unsigned max_border_dim_[4];
  bool use_history_;
  unsigned history_length_;
  bool fix_border_;
  unsigned initial_hold_count_;

  // Internal buffers allocated as necessary
  ring_buffer< image_border > history_;

  bool frame_hold_;
  unsigned hold_count_;

  // Internal helper functions
  void historic_smooth( image_border& border );
  void set_square_mask( const image_border& borders );
};


} // end namespace vidtk


#endif // vidtk_border_detection_process_h_
