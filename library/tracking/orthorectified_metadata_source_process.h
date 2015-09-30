/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_orthorectified_metadata_source_process_h_
#define vidtk_orthorectified_metadata_source_process_h_

/**
\file
\brief Contains the process that consumes the metadata corresponding to the
ortho-rectified image stream.

*/

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>
#include <utilities/homography.h>

namespace vidtk
{

template< class PixType >
class  orthorectified_metadata_source_impl;

/// \brief Process for consuming metadata from an ortho-rectified video
/// source.
///
/// This process feeds the metadata derived fields to the tracker pipeline.
/// Input is assumed to be four latitude and longitude pairs corresponding
/// to the image corners. The outputs of this process include: first frame
/// (ref) to tracking world plane (ref2wld) homography, and tracking world
/// plane (wld) to UTM coordinates (wld2utm) homography.
///
/// TODO:
/// - Adjust UTM zones if not same.
/// - Accept lat/long input parameters for the full frame as opposed to the
///   ones corresponding to the ROI.

template< class PixType >
class orthorectified_metadata_source_process
  : public process
{
public:
  typedef orthorectified_metadata_source_process self_type;

  orthorectified_metadata_source_process( vcl_string const& process_name );

  ~orthorectified_metadata_source_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  /// \brief Input image used only to extract dimensions.
  void set_source_image( vil_image_view< PixType > const& );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );

  /// \brief first frame (ref) to tracking world plane (ref2wld)
  /// homography
  image_to_plane_homography const & ref_to_wld_homography() const;
  VIDTK_OUTPUT_PORT( image_to_plane_homography const&,
                     ref_to_wld_homography );

  /// \brief tracking world plane to UTM coordinates (wld2utm)
  /// homography
  plane_to_utm_homography const & wld_to_utm_homography() const;
  VIDTK_OUTPUT_PORT( plane_to_utm_homography const&,
                     wld_to_utm_homography );

protected:
  orthorectified_metadata_source_impl<PixType> * impl_;

}; // class orthorectified_metadata_source_process

} // namespace vidtk

#endif // vidtk_orthorectified_metadata_source_process_h_
