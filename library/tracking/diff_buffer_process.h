/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_diff_buffer_process_h_
#define vidtk_diff_buffer_process_h_

#include <vcl_vector.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/ring_buffer.h>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

template <class PixType>
class diff_buffer_process
  : public process
{
public:
  typedef diff_buffer_process self_type;
  typedef ring_buffer< vil_image_view<PixType> > img_buffer_type;
  typedef ring_buffer< vgl_h_matrix_2d<double> > homog_buffer_type;

  diff_buffer_process( vcl_string const& name );
  ~diff_buffer_process();
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  /// Set the next img item to be inserted into the buffer
  void add_source_img( vil_image_view<PixType> const& item );
  VIDTK_INPUT_PORT( add_source_img, vil_image_view<PixType> const& );

  /// Set the next homog item to be inserted into the buffer
  void add_source_homog( vgl_h_matrix_2d<double> const& item );
  VIDTK_INPUT_PORT( add_source_homog, vgl_h_matrix_2d<double> const& );

  /// Get 1st image used for diff process
  virtual vil_image_view<PixType> const& get_first_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, get_first_image );

  /// Get 2nd diff image
  virtual vil_image_view<PixType> const& get_second_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, get_second_image );

  /// Get 3rd diff image
  virtual vil_image_view<PixType> const& get_third_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, get_third_image );

  /// Get 3rd to 1st image homog
  virtual vgl_h_matrix_2d<double> const& get_third_to_first_homog() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, get_third_to_first_homog );

  /// Get 3rd to 2nd image homog
  virtual vgl_h_matrix_2d<double> const& get_third_to_second_homog() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, get_third_to_second_homog );

protected:

  // Configuration
  config_block config_;
  unsigned spacing_;
  unsigned current_end_idx_;
  bool disable_capacity_error_;

  // Internal Buffers
  img_buffer_type img_buffer_;
  homog_buffer_type homog_buffer_;

  // Inputs and Outputs
  vgl_h_matrix_2d<double> first_to_ref_;
  vgl_h_matrix_2d<double> second_to_ref_;
  vgl_h_matrix_2d<double> third_to_ref_;
  vgl_h_matrix_2d<double> third_to_first_;
  vgl_h_matrix_2d<double> third_to_second_;
};

} // end namespace vidtk


#endif // vidtk_ring_buffer_process_h_
