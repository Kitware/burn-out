/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_timestamp_reader_process_h_
#define vidtk_timestamp_reader_process_h_

#include <vil/vil_image_view.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>
#include <utilities/oc_timestamp_hack.h>
#include <utilities/video_metadata.h>

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
class timestamp_reader_process
  : public process
{
public:
  typedef timestamp_reader_process self_type;

  timestamp_reader_process( vcl_string const& name );
  ~timestamp_reader_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();
  virtual process::step_status step2();

  /// This is used to obtain the frame number of interest.
  void set_source_timestamp( vidtk::timestamp const& ts );
  VIDTK_INPUT_PORT( set_source_timestamp, vidtk::timestamp const& );

  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_source_metadata( video_metadata const& );
  VIDTK_INPUT_PORT( set_source_metadata, video_metadata const& );

  vidtk::timestamp const& timestamp() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp const&, timestamp );

  vil_image_view<PixType> const& image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType> const&, image );

  /// \brief The input metadata is passed along.
  ///
  /// The reason we are using vector is that, we could be skipping frames
  /// inside this process. For practical reasons we don't want to skip the
  /// metadata packet and will pass along all of them. The process using
  /// this vector would know about this miss-alignment. If the consumer
  /// process doesn't care, then they should just pick the first one.
  vcl_vector< video_metadata > const& metadata() const;
  VIDTK_OUTPUT_PORT( vcl_vector< video_metadata > const&, metadata );

  vidtk::timestamp::vector_t const& timestamp_vector() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp::vector_t const&, timestamp_vector );

private:
  bool initialize_ocean_city();
  bool parse_ocean_city();

  // Use the manual_frame_rate_ to provide timestamp in secs.
  // NOTE: produces timestamp in MICRO seconds, this is what
  // timestamp class expects.
  bool get_full_timestamp();

  enum method_types { null, ocean_city, start_at_frame0 };

  config_block config_;

  /// \internal Which parsing method should we use?
  method_types method_;

  /// \internal If the source timestamp already has a time field,
  /// should we replace it?
  bool override_time_;

  /// \internal A filename, if it is required by the parsing method.
  vcl_string filename_;

  /// \internal Used to generate timestamp in secs.
  // This should be replaced by the real frame_rate info from the
  // input stream.
  double manual_frame_rate_;

  vidtk::timestamp input_ts_; // process input

  // Counter used to force manual (0-based) frame numbers
  unsigned frame_num_0based_;

  /// \internal Absolute starting time of the source image sequence/video,
  /// used in case when the timestamp stream does not capture this
  /// information.
  double base_time_;

  video_metadata const * input_md_; // process input

  unsigned skip_frames_;

  unsigned start_frame_;

  unsigned end_frame_;

  unsigned frame_counter_;

  vidtk::timestamp last_ts_;

  vidtk::timestamp::vector_t  working_ts_vect_;
  vcl_vector< video_metadata > working_md_vect_;

  // cached output - process outputs
  vidtk::timestamp ts_;
  vil_image_view< PixType > img_;
  vcl_vector< video_metadata > output_md_vect_;
  vidtk::timestamp::vector_t  output_ts_vect_;

  // parse helpers
  oc_timestamp_hack oc_timestamp_parser_;
};


} // end namespace vidtk


#endif // vidtk_timestamp_reader_process_h_
