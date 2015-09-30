/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_identify_metadata_burnin_process_static_h_
#define vidtk_identify_metadata_burnin_process_static_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <vil/vil_image_view.h>

#include <opencv/cxcore.h>


namespace vidtk
{


class identify_metadata_burnin_process_static
  : public process
{
public:
  typedef identify_metadata_burnin_process_static self_type;

  identify_metadata_burnin_process_static( vcl_string const& name );

  ~identify_metadata_burnin_process_static();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<vxl_byte> const& img );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  /// \brief The cropped output image.
  vil_image_view<bool> const& metadata_mask() const;

  VIDTK_OUTPUT_PORT( vil_image_view<bool> const&, metadata_mask );

private:

  // Parameters
  config_block config_;

  bool disabled_;
  bool invalid_image_;

  double area_thresh_;
  double prob_thresh_;
  int histogram_detection_thresh_;

  int orientation_roi_[4];
  cv::Mat orientation_mask_;

  double thresh1_;
  double thresh2_;
  int aperture_;
  bool useL2grad_;

  typedef vcl_pair< int, int > point;
  struct segment
  {
    point start;
    point end;
    int thickness;
  };
  struct mask
  {
    vcl_vector< segment > segments;
  };
  vcl_vector< mask > masks_;
  vil_image_view<vxl_byte> byte_mask_;

  // Input
  vil_image_view<vxl_byte> input_image_;

  // Output
  vil_image_view<bool> mask_;
};


} // end namespace vidtk


#endif // vidtk_identify_metadata_burnin_process_static_h_
