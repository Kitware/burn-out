/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_transform_timestamp_process_h_
#define vidtk_transform_timestamp_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/timestamp.h>
#include <utilities/timestamp_generator.h>
#include <utilities/video_metadata.h>
#include <utilities/config_block.h>

#include <vil/vil_image_view.h>

namespace vidtk
{


/// A process to set the time in a timestamp.
///
/// This process is capable of using various sources to determine the
/// correct time for a given frame number.  The source and algorithm
/// used is governed by the \c method configuration parameter.
///
/// One method that this class can use is to parse a time code
/// embedded in the image itself.  For these cases, it can also output
/// the image with the time code stripped out.
template <class PixType>
class transform_timestamp_process
  : public process
{
public:
  typedef transform_timestamp_process self_type;

  transform_timestamp_process( std::string const& name );
  ~transform_timestamp_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  /// This is used to obtain the frame number of interest.
  void set_source_timestamp( vidtk::timestamp const& ts );
  VIDTK_INPUT_PORT( set_source_timestamp, vidtk::timestamp const& );

  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_source_metadata( video_metadata const& );
  VIDTK_INPUT_PORT( set_source_metadata, video_metadata const& );

  vidtk::timestamp timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

  vil_image_view<PixType> image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image );

  video_metadata metadata() const;
  VIDTK_OUTPUT_PORT( video_metadata, metadata );

  /// \brief The input metadata is optionally passed along as a vector.
  ///
  /// The reason we are using vector is that, we could be skipping frames
  /// inside this process. For practical reasons we don't want to skip the
  /// metadata packet and will pass along all of them. The process using
  /// this vector would know about this miss-alignment. If the consumer
  /// process doesn't care, then they should just pick the first one.
  std::vector< video_metadata > metadata_vec() const;
  VIDTK_OUTPUT_PORT( std::vector< video_metadata >, metadata_vec );

private:

  // I/O data
  vidtk::timestamp ts_;
  vil_image_view< PixType > img_;
  video_metadata output_md_;
  std::vector< video_metadata > output_md_vect_;
  vidtk::timestamp input_ts_;
  video_metadata const * input_md_;

  vidtk::timestamp last_ts_;
  std::vector< video_metadata > working_md_vect_;

  // Parameters
  config_block config_;
  bool use_safety_checks_;

  // Timestamp generator
  timestamp_generator generator_;
};


} // end namespace vidtk


#endif // vidtk_transform_timestamp_process_h_
