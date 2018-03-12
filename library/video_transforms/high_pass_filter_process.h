/*ckwg +5
 * Copyright 2011-2017 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_high_pass_filter_process_h_
#define vidtk_high_pass_filter_process_h_


#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <video_properties/border_detection.h>
#include <video_transforms/threaded_image_transform.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{

/// \brief Performs high-pass filtering by one of several methods either
/// in the spatial or frequency domain.
///
/// Some of these filters are not true high-pass filters and are more similar
/// to band-pass filters, but all of them can be configured to isolate certain
/// high frequency trends within the source imagery.
///
/// Description of implemented filters below:
///
/// BOX - Computes 3 output images for the given input. The first image
/// contains the difference between each pixel and a regional average in
/// the horizontal direction, the second the same only for a verticle average,
/// and the third a 2D regional average of some specified size.
///
/// BIDIR - Similar to a box filter, only instead of performing the difference
/// between some pixel and a regional average centered on the pixel, computes
/// the minimum difference between the pixel and 4 regional averages computed
/// above, below, to the left, and to the right of the pixel.
///
/// BOX_WORLD - Performs the same operation as a BOX filter, only instead of
/// having a radius based in pixels, a radius in world coordinates is specified.
template <typename PixType>
class high_pass_filter_process
  : public process
{

public:

  typedef high_pass_filter_process self_type;
  typedef vil_image_view< PixType > image_t;
  typedef threaded_image_transform< image_t, image_t > thread_sys_t;
  typedef boost::scoped_ptr< thread_sys_t > thread_sys_sptr_t;

  high_pass_filter_process( std::string const& _name );
  virtual ~high_pass_filter_process();
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief The input image.
  void set_source_image( image_t const& img );
  VIDTK_INPUT_PORT( set_source_image, image_t const& );

  /// \brief The source GSD.
  void set_source_gsd( double gsd );
  VIDTK_INPUT_PORT( set_source_gsd, double );

  /// \brief An optional image border.
  void set_source_border( image_border const& border );
  VIDTK_INPUT_PORT( set_source_border, image_border const& );

  /// \brief The output image.
  image_t filtered_image() const;
  VIDTK_OUTPUT_PORT( image_t, filtered_image );

private:

  // Inputs
  image_t const* source_image_;
  image_border const* src_border_;
  double gsd_;

  // Possible Outputs
  image_t output_image_;

  // Parameters
  config_block config_;

  enum { DISABLED, BOX, BIDIR, BOX_WORLD } mode_;

  unsigned box_kernel_width_;
  unsigned box_kernel_height_;
  bool box_interlaced_;

  double box_kernel_dim_world_;
  unsigned min_kernel_dim_pixels_;
  unsigned max_kernel_dim_pixels_;
  double last_valid_gsd_;
  bool output_net_only_;

  thread_sys_sptr_t threads_;

  void process_region( const image_t input, image_t output );
};


} // end namespace vidtk


#endif // vidtk_high_pass_filter_process_h_
