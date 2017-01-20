/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_deep_copy_image_process_h_
#define vidtk_deep_copy_image_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

namespace vidtk
{


/// Deep copy the source image.
///
/// This will create a deep copy of the source image.  One use for
/// this is typically as a step between a frame_process and a
/// ring_buffer, since the output of the frame_process may not be a
/// new memory location at each step: it may, for example, return a
/// wrapper around an internal buffer.  If one did not deep copy the
/// image before inserting the output image_view into a ring buffer,
/// all the entries would effectively point to the same image.
///
template <class PixType>
class deep_copy_image_process
  : public process
{
public:
  typedef deep_copy_image_process self_type;

  deep_copy_image_process( std::string const& name );

  virtual ~deep_copy_image_process();

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

  bool deep_copy_;
  bool force_component_order_;

  /// @todo Remove pointers to input data
  vil_image_view<PixType> const* in_img_;

  vil_image_view<PixType> out_img_;
};


} // end namespace vidtk


#endif // vidtk_deep_copy_image_process_h_
