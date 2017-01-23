/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//clamp_image_pixel_values_process.h
#ifndef vidtk_clamp_image_pixel_values_process_h_
#define vidtk_clamp_image_pixel_values_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

template <class PixType>
class vil_image_view;

namespace vidtk
{

///This process exists to fix a bug where null pixel value existed in the image
///thus causing us to miss detections.
template <class PixType>
class clamp_image_pixel_values_process
  : public process
{
public:
  typedef clamp_image_pixel_values_process self_type;

  clamp_image_pixel_values_process( std::string const& name );

  virtual ~clamp_image_pixel_values_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// The image to be copied.
  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// The copy of the image.
  vil_image_view<PixType> image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image );

private:
  config_block config_;

  bool disable_;

  PixType min_;
  PixType max_;

  /// @todo Remove pointers to input data
  vil_image_view<PixType> const* in_img_;

  vil_image_view<PixType> out_img_;
};

}

#endif
