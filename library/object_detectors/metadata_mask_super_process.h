/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_metadata_mask_super_process_h_
#define vidtk_metadata_mask_super_process_h_

#include <kwklt/klt_track.h>

#include <pipeline_framework/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>
#include <utilities/video_metadata.h>

#include <tracking_data/shot_break_flags.h>
#include <tracking_data/image_border.h>

namespace vidtk
{

template< class PixType >
class metadata_mask_super_process_impl;
class shot_break_flags;

template< class PixType >
class metadata_mask_super_process
  : public super_process
{
public:

  typedef metadata_mask_super_process self_type;

  metadata_mask_super_process( std::string const& name );
  virtual ~metadata_mask_super_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  void set_image( vil_image_view< PixType > const& );
  VIDTK_INPUT_PORT( set_image, vil_image_view< PixType > const& );

  void set_timestamp( vidtk::timestamp const& );
  VIDTK_INPUT_PORT( set_timestamp, vidtk::timestamp const& );

  void set_metadata( vidtk::video_metadata const& );
  VIDTK_INPUT_PORT( set_metadata, vidtk::video_metadata const& );

  void set_metadata_vector( vidtk::video_metadata::vector_t const& );
  VIDTK_INPUT_PORT( set_metadata_vector, vidtk::video_metadata::vector_t const& );

  vil_image_view< PixType > image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType >, image );

  vil_image_view< bool > mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool >, mask );

  vil_image_view< bool > borderless_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool >, borderless_mask );

  image_border border() const;
  VIDTK_OUTPUT_PORT( image_border, border );

  std::string detected_type() const;
  VIDTK_OUTPUT_PORT( std::string, detected_type );

  vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  video_metadata metadata() const;
  VIDTK_OUTPUT_PORT( video_metadata, metadata );

  double world_units_per_pixel() const;
  VIDTK_OUTPUT_PORT( double, world_units_per_pixel );

  video_metadata::vector_t metadata_vector() const;
  VIDTK_OUTPUT_PORT( video_metadata::vector_t, metadata_vector );

private:

  metadata_mask_super_process_impl<PixType>* impl_;
};

} // end namespace vidtk

#endif // vidtk_metadata_mask_super_process_h_
