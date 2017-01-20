/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pixel_feature_writer_h_
#define vidtk_pixel_feature_writer_h_

#include <vil/vil_image_view.h>
#include <vil/algo/vil_blob.h>

#include <vector>
#include <map>
#include <string>

#include <video_properties/border_detection.h>

#include "pixel_annotation_loader.h"

namespace vidtk
{


/// \brief Settings for the pixel_feature_writer class
struct pixel_feature_writer_settings
{
  /// Output file location
  std::string output_file;

  /// Groundtruth loader settings
  pixel_annotation_loader_settings gt_loader;

  /// Should we output a validation image showing groundtruth pixels
  /// drawn on top of the images we have groundtruth for?
  bool output_validation_image;

  /// Number of pixels near the outside image pixel to ignore
  unsigned border_pixels_to_ignore;

  /// Percent of pixels in a blob required for it to be that category
  double blob_pixel_percentage;

  /// Default settings
  pixel_feature_writer_settings()
  : output_file( "training_data.dat" ),
    output_validation_image( true ),
    border_pixels_to_ignore( 5 ),
    blob_pixel_percentage( 0.25 )
  {}
};


/// \brief A class for assisting with writing pixel-level features to a
/// text file, meant for use for extracting training data for classifiers
///
/// This class functions as a state machine, and should be used by first
/// specifying all settings for loading groundtruth and output settings at
/// the start of the video, followed by calling the step function for every
/// new frame.
template< typename PixType >
class pixel_feature_writer
{
public:

  typedef pixel_feature_writer_settings settings_t;
  typedef vil_image_view< PixType > image_t;
  typedef std::vector< image_t > feature_array_t;

  pixel_feature_writer();
  virtual ~pixel_feature_writer() {}

  /// Set any options for the pixel feature writer.
  virtual bool configure( const settings_t& settings );

  /// For each frame, if there is available groundtruth for said frame,
  /// in a list print out the gt label for each pixel followed by the
  /// list of features for the pixel.
  virtual bool step( const feature_array_t& features,
                     const image_border border = image_border() );

  /// For each frame, if there is available groundtruth for said frame,
  /// for all input blobs write out the most likely label for each blob
  /// with the corresponding supplied input feature vectors.
  virtual bool step( const std::vector< vil_blob_pixel_list >& blobs,
                     const std::vector< std::vector< double > >& features,
                     const image_border border = image_border() );

private:

  unsigned frame_counter_;
  unsigned border_pixels_to_ignore_;
  bool output_validation_image_;
  pixel_annotation_loader gt_loader_;
  std::string output_file_;
  double blob_pixel_percentage_;
};


/// \brief Print out a vector of pixel-feature images
///
/// Requires a string prefix and postfix (which are seperated by a numerically
/// increasing integral value corresponding to each feature image). Can optionally
/// scale values to range [0,255] when outputting.
template< typename PixType >
bool write_feature_images( const std::vector< vil_image_view< PixType> >& features,
                           const std::string prefix = "feature",
                           const std::string postfix = ".png",
                           const bool scale_features = true );


} // end namespace vidtk

#endif // vidtk_pixel_feature_writer_h_
