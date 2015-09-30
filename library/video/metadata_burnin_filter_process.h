/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_metadata_burnin_filter_process_h_
#define vidtk_metadata_burnin_filter_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

namespace vidtk
{

/// \brief Fast 1D (horizontal) box filter smoothing
/// \param width The width of the box filter
void box_filter_1d(const vil_image_view<vxl_byte>& src,
                   vil_image_view<vxl_byte>& dst,
                   unsigned width);


/// \brief Filter images to detect black or white metadata burnin
/// This process produces a 3-plane image where planes 1 and 2 are
/// responses to horizontally and vertically oriented filters and
/// plane 3 is the response to a bidirectional filter.
/// The process tries to detect black or white metadata and returns
/// the result with the highest response.  The metadata_is_black
/// port is true if the metadata is black and false if white.
class metadata_burnin_filter_process
  : public process
{
public:
  typedef metadata_burnin_filter_process self_type;

  metadata_burnin_filter_process( vcl_string const& name );

  ~metadata_burnin_filter_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<vxl_byte> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  /// \brief The output image.
  vil_image_view<vxl_byte> const& metadata_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte> const&, metadata_image );

  /// \brief The the output is true if metadata burnin is black.
  bool metadata_is_black() const;
  VIDTK_OUTPUT_PORT( bool, metadata_is_black );

private:

  /// \brief detect metadata burnin
  unsigned long detect_burnin(const vil_image_view<vxl_byte>& image,
                             const vil_image_view<vxl_byte>& smooth,
                             vil_image_view<vxl_byte>& weight,
                             bool black);
  // Parameters
  config_block config_;

  int kernel_width_;
  int kernel_height_;
  bool interlaced_;
  int adapt_thresh_high_;
  int abs_thresh_low_;
  int abs_thresh_high_;

  vil_image_view<vxl_byte> image_;
  vil_image_view<vxl_byte> filtered_image_;
  bool is_black_;

};


} // end namespace vidtk


#endif // vidtk_metadata_burnin_filter_process_h_
