/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kmeans_segmentation_process_h_
#define vidtk_kmeans_segmentation_process_h_


#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <video_properties/border_detection.h>

namespace vidtk
{


/// Segments an image according to kmeans, returning an image containing
/// the cluster ID for each pixel in the input image.
template <typename PixType, typename LabelType = unsigned>
class kmeans_segmentation_process
  : public process
{

public:

  typedef kmeans_segmentation_process self_type;

  kmeans_segmentation_process( std::string const& name );
  virtual ~kmeans_segmentation_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// \brief An optional image border.
  void set_source_border( image_border const& border );
  VIDTK_INPUT_PORT( set_source_border, image_border const& );

  /// \brief The output image.
  vil_image_view<LabelType> segmented_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<LabelType>, segmented_image );

private:

  // Inputs
  /// @todo Remove pointers to input data
  vil_image_view<PixType> const* src_image_;
  image_border const* src_border_;

  // Possible Outputs
  vil_image_view<LabelType> dst_image_;

  // Parameters
  config_block config_;
  unsigned clusters_;
  unsigned sampling_points_;
};


} // end namespace vidtk


#endif // vidtk_kmeans_segmentation_process_h_
