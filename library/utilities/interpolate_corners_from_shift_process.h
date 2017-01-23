/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_interpolate_corners_from_shift_process_h_
#define vidtk_interpolate_corners_from_shift_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>

#include "interpolate_corners_from_shift.h"

namespace vidtk
{

template< class PixType >
class interpolate_corners_from_shift_process
  : public process
{
public:
  typedef interpolate_corners_from_shift_process self_type;

  interpolate_corners_from_shift_process( std::string const& name );
  virtual ~interpolate_corners_from_shift_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_input_image( vil_image_view<PixType> const& );
  VIDTK_INPUT_PORT( set_input_image, vil_image_view<PixType> const& );

  void set_input_timestamp( timestamp const& );
  VIDTK_INPUT_PORT( set_input_timestamp, timestamp const& );

  void set_input_metadata_vector( video_metadata::vector_t const& );
  VIDTK_INPUT_PORT( set_input_metadata_vector, video_metadata::vector_t const& );

  void set_input_homography( image_to_image_homography const & );
  VIDTK_INPUT_PORT( set_input_homography, image_to_image_homography const & );

  video_metadata::vector_t output_metadata_vector() const;
  VIDTK_OUTPUT_PORT( video_metadata::vector_t, output_metadata_vector );

private:

  // Parameters
  bool disabled_;
  config_block config_;

  // Inputs
  vil_image_view<PixType> image_;
  timestamp ts_;
  video_metadata::vector_t in_meta_vec_;
  image_to_image_homography homog_;

  // Algorithm
  interpolate_corners_from_shift algorithm_;

  // Outputs
  video_metadata::vector_t out_meta_vec_;
};

} // end namespace vidtk


#endif // vidtk_interpolate_corners_from_shift_process_h_
