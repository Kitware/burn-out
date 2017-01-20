/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_aligned_edge_detection_process_h_
#define vidtk_aligned_edge_detection_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <tracking_data/image_border.h>

#include <video_transforms/threaded_image_transform.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{

/// \brief This process, used for burnin detection in addition to salient
/// object detection, generates information pertaining to possible
/// vertical and horizontally aligned edges within an image.
///
/// The first output is a 2-channel image containing aligned (vertical
/// and horizontal) edges given a grayscale image, where the first channel
/// of the image contains non-maximum suppressed horizontal edges, and the
/// second contains the vertical equivalent. Non-edges are given a value
/// of 0, and edges retain their respective edge magnitude from the gs image.
///
/// The second [optional] output contains 1 channel, a smoothed version
/// of the sum of both channels of the first output, which is more or
/// less a representation of the local density of NMS edges for each pixel.
template <typename PixType>
class aligned_edge_detection_process
  : public process
{

public:

  typedef aligned_edge_detection_process self_type;
  typedef vil_image_view< PixType > input_image_t;
  typedef vil_image_view< float > float_image_t;
  typedef threaded_image_transform< input_image_t, input_image_t,
    input_image_t, float_image_t, float_image_t > thread_sys_t;
  typedef boost::scoped_ptr< thread_sys_t > thread_sys_sptr_t;

  aligned_edge_detection_process( std::string const& name );
  virtual ~aligned_edge_detection_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( input_image_t const& src );
  VIDTK_INPUT_PORT( set_source_image, input_image_t const& );

  void set_source_border( image_border const& border );
  VIDTK_INPUT_PORT( set_source_border, image_border const& );

  input_image_t aligned_edge_image() const;
  VIDTK_OUTPUT_PORT( input_image_t, aligned_edge_image );

  input_image_t combined_edge_image() const;
  VIDTK_OUTPUT_PORT( input_image_t, combined_edge_image );

private:

  // Inputs
  input_image_t source_image_;
  image_border border_;

  // Possible Outputs
  input_image_t aligned_edge_output_;
  input_image_t combined_edge_output_;

  // Internal buffers
  float_image_t grad_i_;
  float_image_t grad_j_;
  thread_sys_sptr_t threads_;

  // Internal parameters/settings
  config_block config_;
  float threshold_;
  bool produce_joint_output_;
  double smoothing_sigma_;
  unsigned smoothing_half_step_;

  // Internal helper function
  void process_region( const input_image_t& input_image,
                       input_image_t& aligned_edges,
                       input_image_t& combined_edges,
                       float_image_t& grad_i_buffer,
                       float_image_t& grad_j_buffer );
};


} // end namespace vidtk


#endif // vidtk_aligned_edge_detection_process_h_
