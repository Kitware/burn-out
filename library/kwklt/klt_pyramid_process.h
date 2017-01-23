/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kwklt_pyramid_process_h_
#define vidtk_kwklt_pyramid_process_h_

#include <vil/vil_image_view.h>
#include <vil/vil_pyramid_image_view.h>

#include <process_framework/pipeline_aid.h>
#include <process_framework/process.h>
#include <utilities/config_block.h>

namespace vidtk
{

template< class PixType >
class klt_pyramid_process
  : public process
{
public:
  typedef klt_pyramid_process self_type;

  klt_pyramid_process(std::string const& name);

  virtual ~klt_pyramid_process();

  virtual config_block params() const;

  virtual bool set_params(config_block const&);

  virtual bool initialize();

  virtual bool step();

  virtual bool reset();

  // for the moment, we only allow a 8-bit image. If we want to handle
  // 16-bit image, we would need to extend the underlying KLT to do
  // that.  Then, we can handle 16-bit images by overloading set_image
  // with a 16-bit image version, instead of making the whole class
  // templated.

  /// Set the next image to process.
  void set_image(vil_image_view<PixType> const& img);

  VIDTK_INPUT_PORT(set_image, vil_image_view<PixType> const&);

  /// The image pyramid of the input image.
  vil_pyramid_image_view<float> image_pyramid() const;
  VIDTK_OUTPUT_PORT(vil_pyramid_image_view<float>, image_pyramid);

  /// The image pyramid of the x gradient.
  vil_pyramid_image_view<float> image_pyramid_gradx() const;
  VIDTK_OUTPUT_PORT(vil_pyramid_image_view<float>, image_pyramid_gradx);

  /// The image pyramid of the y gradient.
  vil_pyramid_image_view<float> image_pyramid_grady() const;
  VIDTK_OUTPUT_PORT(vil_pyramid_image_view<float>, image_pyramid_grady);

protected:
  vil_image_view<vxl_byte> img_; //This should be templated, but some of the klt code is assuming uchar images.
  vil_pyramid_image_view<float> pyramid_;
  vil_pyramid_image_view<float> pgradx_;
  vil_pyramid_image_view<float> pgrady_;

  // ----- Parameters

  config_block config_;

  bool disabled_;

  /// Subsampling factor to use.
  int subsampling_;

  /// Number of levels to use in the output pyramid.
  int levels_;

  /// Sigma factor for computation of the image pyramid.
  float init_sigma_;

  /// Sigma factor for computation of the image pyramid.
  float sigma_factor_;

  /// Sigma factor for computation of the image gradient pyramids.
  float grad_sigma_;
};

template<>
void klt_pyramid_process<vxl_uint_16>::set_image(vil_image_view<vxl_uint_16> const& img);
template<>
void klt_pyramid_process<vxl_byte>::set_image(vil_image_view<vxl_byte> const& img);


} // end namespace vidtk


#endif // vidtk_kwklt_pyramid_process_h_
