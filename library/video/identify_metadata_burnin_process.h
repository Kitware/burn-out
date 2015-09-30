/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_identify_metadata_burnin_process_h_
#define vidtk_identify_metadata_burnin_process_h_

/// \file

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vcl_set.h>
#include <vcl_vector.h>

namespace vidtk
{


class identify_metadata_burnin_process_impl
{
public:
  static config_block params();
  virtual bool set_params( config_block const& );

  virtual bool initialize();
  virtual bool step();

  virtual void set_input_image( vil_image_view< vxl_byte > const& img );

  bool disabled_;
  bool verbose_;
  bool write_intermediates_;

  // Detection parameters
  double cross_threshold_;
  double bracket_threshold_;
  double rectangle_threshold_;
  double text_threshold_;

  vcl_vector< vcl_pair< vcl_string, vcl_string > > templates_;

  vcl_string edge_template_;

  // Possible parameters to be detected later; currently softcoded into the config file
  double line_width_;
  double draw_line_width_;
  double min_roi_ratio_;
  double roi_ratio_;
  double roi_aspect_;
  int off_center_x_;
  int off_center_y_;
  int cross_gap_x_;
  int cross_gap_y_;
  int cross_length_x_;
  int cross_length_y_;
  int bracket_length_x_;
  int bracket_length_y_;
  double max_text_dist_;

  // Text-detection parameters
  double text_repeat_threshold_;
  size_t text_memory_;
  double text_consistency_threshold_;
  size_t text_range_;

  // Search parameters
  vxl_uint_8 dark_pixel_threshold_;
  int off_center_jitter_;
  int bracket_aspect_jitter_;

  vcl_string name_;

  // Input
  vil_image_view<vxl_byte> input_image_;

  // Output
  vil_image_view<bool> mask_;
};

class identify_metadata_burnin_process
  : public process
{
public:
  typedef identify_metadata_burnin_process self_type;

  identify_metadata_burnin_process( vcl_string const& name );

  ~identify_metadata_burnin_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<vxl_byte> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  /// \brief The output mask.
  vil_image_view<bool> const& metadata_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool> const&, metadata_mask );

private:

  identify_metadata_burnin_process_impl* impl_;
};


} // end namespace vidtk


#endif // vidtk_identify_metadata_burnin_process_h_
