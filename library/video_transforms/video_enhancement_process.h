/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_enhancement_process_h_
#define vidtk_video_enhancement_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <video_transforms/automatic_white_balancing.h>
#include <video_transforms/illumination_normalization.h>

#include <boost/scoped_ptr.hpp>

//template <typename PixType>
//class vil_image_view;

namespace vidtk
{

/// Contains a few different methods for performing simple color correction
/// and various other operations for enchancing video
///
/// Includes toggleable smoothing, contrast enhancement, automatic white
/// balancing and brightness normalization
template <typename PixType>
class video_enhancement_process
  : public process
{

public:

  typedef video_enhancement_process self_type;
  typedef vil_image_view<PixType> input_type;
  typedef vil_image_view<bool> mask_type;
  typedef boost::scoped_ptr< illumination_normalization<PixType> > normalizer_sptr;

  video_enhancement_process( std::string const& name );
  virtual ~video_enhancement_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_mask_image( vil_image_view<bool> const& mask );
  VIDTK_INPUT_PORT( set_mask_image, vil_image_view<bool> const& );

  void set_enable_flag( bool flag );
  VIDTK_INPUT_PORT( set_enable_flag, bool );

  void set_reset_flag( bool flag );
  VIDTK_INPUT_PORT( set_reset_flag, bool );

  vil_image_view<PixType> output_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, output_image );

  vil_image_view<PixType> copied_output_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, copied_output_image );

private:

  // Inputs
  /// @todo Remove pointers to input data
  vil_image_view<PixType> const* src_image_;
  vil_image_view<bool> const* mask_image_;
  bool reset_flag_;
  bool enable_flag_;

  // Possible Outputs
  vil_image_view<PixType> output_image_;
  mutable vil_image_view<PixType> copied_output_image_;

  // Internal parameters/settings
  config_block config_;
  bool process_disabled_;

  // Masking Options
  bool masking_enabled_;

  // Simple Smoothing Options
  bool smoothing_enabled_;
  double std_dev_;
  unsigned half_width_;

  // Inversion settings
  bool inversion_enabled_;

  // Auto-white balancing Params
  bool awb_enabled_;
  auto_white_balancer_settings awb_settings_;
  auto_white_balancer<PixType> awb_engine_;

  // Illumination Norm Options
  enum { MEAN, MEDIAN, NONE, INVALID } illumination_mode_;
  normalizer_sptr illum_function_;
};


} // end namespace vidtk


#endif // vidtk_video_enhancement_process_h_
