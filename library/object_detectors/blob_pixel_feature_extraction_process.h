/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_blob_pixel_feature_extractor_process_h_
#define vidtk_blob_pixel_feature_extractor_process_h_

#include "blob_pixel_feature_extraction.h"

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

namespace vidtk
{


/// \brief A specialized process which takes as input a binary mask, from which multiple
/// pixel level features are computed relating to each blob in the image.
///
/// In more detail, it formulates an output image containing (# of features enabled)
/// output planes for each input plane of the binary mask. These features can include:
///
///  (a) A relative measure of the size of each blob in relation to the image size
///  (b) A relative measure of the density of the blob compared against its surrounding
///  (c) A relative measure of the aspect ratio of a bbox drawn around each blob
template <typename FeatureType>
class blob_pixel_feature_extraction_process
  : public process
{
public:

  typedef blob_pixel_feature_extraction_process self_type;

  blob_pixel_feature_extraction_process( std::string const& name );
  virtual ~blob_pixel_feature_extraction_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( vil_image_view<bool> const& src );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<bool> const& );

  vil_image_view<FeatureType> feature_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<FeatureType>, feature_image );

private:

  config_block config_;

  vil_image_view<bool> const* src_image_;
  vil_image_view<FeatureType> output_image_;

  blob_pixel_features_settings settings_;
};


} // end namespace vidtk


#endif // vidtk_blob_pixel_feature_extraction_process_h_
