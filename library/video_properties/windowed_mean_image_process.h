/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_properties_windowed_mean_image_process_h_
#define vidtk_video_properties_windowed_mean_image_process_h_

#include <vil/vil_image_view.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>


namespace vidtk
{


/// This process computes the local mean and variance at each pixel.
///
/// It computes the mean and variance using a square window centered
/// around each pixel.  The pixels at the boundaries are computed
/// using the best fitting window of the same size, and hence the
/// resulting values repeat themselves near the border of the image.
/// (E.g., for a 5x5 window, the result at (0,8) is computed using the
/// same window as at (1,8): the window from (0,6)-(4,9).)
///
/// The mean and variance is computed independently on each channel of
/// a multi-channel image.
template<typename SrcPixelType, typename MeanPixelType = double>
class windowed_mean_image_process
  : public process
{
public:
  typedef windowed_mean_image_process self_type;

  typedef SrcPixelType src_pixel_type;
  typedef MeanPixelType mean_pixel_type;

  windowed_mean_image_process( std::string const& name );

  ~windowed_mean_image_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_source_image( vil_image_view<src_pixel_type> const& img );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<src_pixel_type> const& );

  /// An image the same size as the source image, with each pixel
  /// representing the local mean.
  vil_image_view<double> mean_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<mean_pixel_type>, mean_image );

  /// An image the same size as the source image, with each pixel
  /// representing the local variance.
  vil_image_view<double> variance_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<mean_pixel_type>, variance_image );

  /// This is the square root of the variance image.
  ///
  /// Of course, it requires O(ni*nj) square root operations, so this
  /// is a somewhat expensive image to request.
  vil_image_view<double> stddev_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<mean_pixel_type>, stddev_image );

private:
  config_block config_;

  unsigned radius_;

  vil_image_view<src_pixel_type> const* in_img_;

  vil_image_view<mean_pixel_type> mean_img_;
  vil_image_view<mean_pixel_type> var_img_;
};


} // end namespace vidtk


#endif // vidtk_windowed_mean_image_process_h_
