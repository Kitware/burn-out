/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_moving_burnin_detector_process_h_
#define vidtk_moving_burnin_detector_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <set>
#include <vector>

#include <object_detectors/scene_obstruction_detector.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{


class moving_burnin_detector_impl
{
public:
  virtual ~moving_burnin_detector_impl() {};

  static config_block params();
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  virtual void set_input_image( vil_image_view< vxl_byte > const& img );
  virtual void scale_params_for_image( vil_image_view< vxl_byte > const& img );

  bool disabled_;
  bool verbose_;
  bool write_intermediates_;

  // Detection parameters
  double cross_threshold_;
  double bracket_threshold_;
  double rectangle_threshold_;
  double text_threshold_;
  bool highest_score_only_;

  std::vector< std::pair< std::string, std::string > > templates_;
  std::string edge_template_;

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
  float cross_ends_ratio_;

  // Text-detection parameters
  double text_repeat_threshold_;
  size_t text_memory_;
  double text_consistency_threshold_;
  size_t text_range_;

  // Resolution these parameters were generated for, if known
  unsigned target_resolution_x_;
  unsigned target_resolution_y_;

  // Search parameters
  vxl_uint_8 dark_pixel_threshold_;
  int off_center_jitter_;
  int bracket_aspect_jitter_;

  std::string name_;

  // Inputs
  vil_image_view<vxl_byte> input_image_;

  // Outputs
  vil_image_view<bool> mask_;
  std::vector<unsigned> target_widths_;
};

class moving_burnin_detector_process
  : public process
{
public:
  typedef moving_burnin_detector_process self_type;

  moving_burnin_detector_process( std::string const& name );
  virtual ~moving_burnin_detector_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<vxl_byte> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<vxl_byte> const& );

  /// \brief A path to a new set of parameters to use.
  void set_parameter_file( std::string const& file );
  VIDTK_INPUT_PORT( set_parameter_file, std::string const& );

  /// \brief The output mask.
  vil_image_view<bool> metadata_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, metadata_mask );

  /// \brief Detected bracket widths.
  std::vector<unsigned> target_pixel_widths() const;
  VIDTK_OUTPUT_PORT( std::vector<unsigned>, target_pixel_widths );

private:

  // Pointer to selected implementation
  boost::scoped_ptr< moving_burnin_detector_impl > impl_;

  // Optional parameter file input, in case parameters need to change
  std::string parameter_file_;

  // The last configuration file used, if set_parameter file is ever called
  std::string last_parameter_file_;

  // The last input image
  vil_image_view< vxl_byte > input_image_;

  // Whether or not to force this process to be disabled
  bool force_disable_;
};


} // end namespace vidtk


#endif // vidtk_moving_burnin_detector_process_h_
