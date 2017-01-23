/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_mask_image_process_h_
#define vidtk_mask_image_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <string>

namespace vidtk
{

class mask_image_process
  : public process
{
public:
  typedef mask_image_process self_type;

  mask_image_process( std::string const& name );

  virtual ~mask_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// The image to be masked.
  void set_source_image( vil_image_view<bool> const& img );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<bool> const& );

  /// The mask image.
  void set_mask_image( vil_image_view<bool> const& img );

  VIDTK_OPTIONAL_INPUT_PORT( set_mask_image, vil_image_view<bool> const& );

  /// The copy of the image.
  vil_image_view<bool> image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, image );

  /// The copy of the image.
  vil_image_view<bool> copied_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, copied_image );

private:
  config_block config_;

  // Masked pixels should be on. *on/true* pixels in the mask_img_ will be ignored.
  /// @todo Remove pointers to input data
  vil_image_view<bool> const* mask_img_;

  // This value will be written out in the 'masked pixels'.
  bool masked_value_;

  vil_image_view<bool> const* in_img_;

  vil_image_view<bool> out_img_;

  mutable vil_image_view<bool> copied_out_img_;
};


} // end namespace vidtk

#endif // vidtk_mask_image_process_h_
