/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_stabilization_super_process_h_
#define vidtk_stabilization_super_process_h_

#include <kwklt/klt_track.h>

#include <pipeline_framework/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>

#include <tracking_data/shot_break_flags.h>

namespace vidtk
{

template< class PixType >
class stabilization_super_process_impl;
class shot_break_flags;

template< class PixType >
class stabilization_super_process
  : public super_process
{
public:
  typedef stabilization_super_process self_type;

  stabilization_super_process( std::string const& name );
  virtual ~stabilization_super_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  // -- process inputs --
  /// Set the detected foreground image.
  void set_source_image( vil_image_view< PixType > const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_source_mask( vil_image_view< bool > const& img );
  VIDTK_INPUT_PORT( set_source_mask, vil_image_view< bool > const& );

  void set_source_timestamp( timestamp const& );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_source_metadata( video_metadata const& );
  VIDTK_INPUT_PORT( set_source_metadata, video_metadata const& );

  // -- process outputs --
  timestamp source_timestamp() const;
  VIDTK_OUTPUT_PORT( timestamp, source_timestamp );

  vil_image_view< PixType > source_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType >, source_image );

  /// \brief The input metadata is passed along.
  ///
  /// The reason we are using vector is that, we could be skipping frames
  /// inside this process. For practical reasons we don't want to skip the
  /// metadata packet and will pass along all of them. The process using
  /// this vector would know about this miss-alignment. If the consumer
  /// process doesn't care, then they should just pick the first one.
  video_metadata::vector_t output_metadata_vector() const;
  VIDTK_OUTPUT_PORT(video_metadata::vector_t, output_metadata_vector );

  /// The output homography from source (current) to reference image.
  image_to_image_homography src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, src_to_ref_homography );

  image_to_image_homography ref_to_src_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, ref_to_src_homography );

  std::vector< klt_track_ptr > active_tracks() const;
  VIDTK_OUTPUT_PORT( std::vector< klt_track_ptr >, active_tracks );

  vidtk::shot_break_flags get_output_shot_break_flags() const;
  VIDTK_OUTPUT_PORT( vidtk::shot_break_flags, get_output_shot_break_flags );

  vil_image_view< bool > get_mask() const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool >, get_mask );


private:
  stabilization_super_process_impl<PixType> * impl_;
};

} // end namespace vidtk

#endif // vidtk_stabilization_super_process_h_
