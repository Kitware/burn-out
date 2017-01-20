/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_uncrop_image_process_h_
#define vidtk_uncrop_image_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

template <class PixType>
class vil_image_view;

namespace vidtk
{


/// Crop an input image with user-specified parameters.
///
/// The cropped image generally *not* a deep copy, and shares pixels
/// with the source image.
///
template <class PixType>
class uncrop_image_process
  : public process
{
public:
  typedef uncrop_image_process self_type;

  uncrop_image_process( std::string const& name );

  virtual ~uncrop_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// \brief The cropped output image.
  vil_image_view<PixType> uncropped_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, uncropped_image );

private:

  // Parameters
  config_block config_;

  bool disabled_;

  unsigned int horizontal_pad_, vertical_pad_;

  PixType bkgnd_value_;

  // Local state
  vil_image_view<PixType> cropped_view_;

  // Input
  vil_image_view<PixType> const* input_image_;

  // Output
  vil_image_view<PixType> uncropped_image_;
};


} // end namespace vidtk


#endif // vidtk_uncrop_image_process_h_
