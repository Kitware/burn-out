/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_diff_buffer_process_h_
#define vidtk_diff_buffer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/ring_buffer.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>

#include <vil/vil_image_view.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

#include <tracking_data/gui_frame_info.h>
#include <tracking_data/shot_break_flags.h>

#include <vector>


namespace vidtk
{

template <class PixType>
class diff_buffer_process
  : public process
{
public:
  typedef diff_buffer_process self_type;
  typedef vil_image_view< PixType > img_type;
  typedef vil_image_view< bool > mask_type;
  typedef vgl_h_matrix_2d< double > homog_type;
  typedef ring_buffer< img_type > img_buffer_type;
  typedef ring_buffer< mask_type > mask_buffer_type;
  typedef ring_buffer< homog_type > homog_buffer_type;
  typedef gui_frame_info::recommended_action gui_action_type;

  diff_buffer_process( std::string const& name );
  virtual ~diff_buffer_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual process::step_status step2();

  /// Set the next img item to be inserted into the buffer
  void set_source_img( vil_image_view<PixType> const& item );
  VIDTK_INPUT_PORT( set_source_img, vil_image_view<PixType> const& );

  /// Set the next mask item to be inserted into the buffer
  void set_source_mask( vil_image_view<bool> const& item );
  VIDTK_INPUT_PORT( set_source_mask, vil_image_view<bool> const& );

  /// Set the next homog item to be inserted into the buffer
  void set_source_homog( vgl_h_matrix_2d<double> const& item );
  VIDTK_INPUT_PORT( set_source_homog, vgl_h_matrix_2d<double> const& );

  /// Set the next homog item to be inserted into the buffer
  void set_source_vidtk_homog( image_to_image_homography const& item );
  VIDTK_INPUT_PORT( set_source_vidtk_homog, image_to_image_homography const& );

  /// Set any gui feedback for the current frame
  void add_source_gui_feedback( gui_frame_info const& fb );
  VIDTK_INPUT_PORT( add_source_gui_feedback, gui_frame_info const& );

  /// Set any gui feedback for the current frame
  void add_shot_break_flags( shot_break_flags const& sbf );
  VIDTK_INPUT_PORT( add_shot_break_flags, shot_break_flags const& );

  /// Set source GSD
  void add_gsd( double gsd );
  VIDTK_INPUT_PORT( add_gsd, double );

  /// Get 1st image used for diff process
  virtual vil_image_view<PixType> get_first_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, get_first_image );

  /// Get 2nd diff image
  virtual vil_image_view<PixType> get_second_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, get_second_image );

  /// Get 3rd diff image
  virtual vil_image_view<PixType> get_third_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, get_third_image );

  /// Get 1st mask used for diff process
  virtual vil_image_view<bool> get_first_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, get_first_mask );

  /// Get 2nd diff mask
  virtual vil_image_view<bool> get_second_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, get_second_mask );

  /// Get 3rd diff mask
  virtual vil_image_view<bool> get_third_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, get_third_mask );

  /// Get 3rd to 1st image homog
  virtual vgl_h_matrix_2d<double> get_third_to_first_homog() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, get_third_to_first_homog );

  /// Get 3rd to 2nd image homog
  virtual vgl_h_matrix_2d<double> get_third_to_second_homog() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, get_third_to_second_homog );

  /// Flag indicating whether or not differencing should even be performed
  virtual bool get_differencing_flag() const;
  VIDTK_OUTPUT_PORT( bool, get_differencing_flag );

protected:

  // Configuration
  config_block config_;
  unsigned spacing_;
  bool masking_enabled_;
  unsigned current_end_idx_;
  bool disable_capacity_error_;
  bool reset_on_next_pass_;
  bool check_shot_break_flags_;
  double min_overlap_;
  bool enable_min_overlap_;
  unsigned min_adaptive_thresh_;
  double min_operating_gsd_;
  double max_operating_gsd_;

  // Internal Buffers
  img_buffer_type img_buffer_;
  mask_buffer_type mask_buffer_;
  homog_buffer_type homog_buffer_;

  // Inputs and Outputs
  vil_image_view<PixType> src_img_;
  vil_image_view<bool> src_mask_;
  vgl_h_matrix_2d<double> src_homog_;
  double input_gsd_;

  vgl_h_matrix_2d<double> first_to_ref_;
  vgl_h_matrix_2d<double> second_to_ref_;
  vgl_h_matrix_2d<double> third_to_ref_;
  vgl_h_matrix_2d<double> third_to_first_;
  vgl_h_matrix_2d<double> third_to_second_;
  vgl_h_matrix_2d<double> first_to_third_;
  bool differencing_flag_;

  // Optional Inputs
  gui_action_type requested_action_;
  shot_break_flags shot_break_flags_;

  // Helper function
  void reset_buffer_to_last_entry();

  // Compute image overlap
  double percent_overlap( const unsigned ni, const unsigned nj, vgl_h_matrix_2d<double> homog );

  // Double compute
  double po_per_index( const unsigned index );
  void compute_index();
  unsigned min_allowed_;

};

} // end namespace vidtk


#endif // vidtk_ring_buffer_process_h_
