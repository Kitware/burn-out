/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef filter_video_metadata_process_h_
#define filter_video_metadata_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/gsd_file_source.h>
#include <vil/vil_image_view.h>
#include <vector>

namespace vidtk
{


// ----------------------------------------------------------------
/** Filter video metadata process.
 *
 * This process filters the incoming stream of metadata and passes the
 * inputs when the following criteria are met.
 *
 * (1) It must also have a valid time stamp.
 *
 * (2) Its time must not be less than the last packet time seen. That
 *     is, all packets must have increasing timestamps.
 *
 * (3) Horizontal field of view value, if non-zero, should not be
 *     unrealistically large.
 *
 * (4) Platform altitude, if non-zero, should not change drastically
 *     between two consecutive metadata packets.
 *
 * (5) Any of the lat/lon location should not change drastically between
 *     two consecutive metadata packets.
 *
 * All the inputs, except the video metadata, are passed through on
 * the same cycle. The video moeadata is only passed through if it is
 * good.
 */
class filter_video_metadata_process
  : public process
{
public:
  typedef filter_video_metadata_process self_type;
  typedef vil_image_view < vxl_byte > image_view_t;


  filter_video_metadata_process( std::string const& name );
  virtual ~filter_video_metadata_process();

  // process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  // Input ports
  void set_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  void set_source_metadata( video_metadata const& );
  VIDTK_INPUT_PORT( set_source_metadata, video_metadata const& );

  void set_source_metadata_vector( std::vector< video_metadata > const& );
  VIDTK_INPUT_PORT( set_source_metadata_vector, std::vector< video_metadata > const& );

  // Output ports
  std::vector< video_metadata > output_metadata_vector() const;
  VIDTK_OUTPUT_PORT( std::vector< video_metadata >, output_metadata_vector );

  video_metadata output_metadata() const;
  VIDTK_OUTPUT_PORT( video_metadata, output_metadata );

protected:
  bool is_good(video_metadata * in);
  bool is_good_location( geo_coord::geo_lat_lon const& new_loc,
                                 geo_coord::geo_lat_lon & prev_loc ) const;

  config_block config_;

  // Configuration parameters
  bool disable_;
  bool allow_equal_time_;
  double max_hfov_;
  double max_alt_delta_;
  double max_lat_delta_;
  double max_lon_delta_;

  video_metadata const * input_single_;
  std::vector< video_metadata > const * input_multi_;
  video_metadata invalid_;
  std::vector< video_metadata > output_vector_;
  vxl_uint_64 previous_time_;
  video_metadata previous_packet_;
  double previous_alt_;
  geo_coord::geo_lat_lon prev_platform_loc_;
  geo_coord::geo_lat_lon prev_frame_center_;
  geo_coord::geo_lat_lon prev_corner_ul_;
  geo_coord::geo_lat_lon prev_corner_ur_;
  geo_coord::geo_lat_lon prev_corner_lr_;
  geo_coord::geo_lat_lon prev_corner_ll_;

  timestamp ts_;
  gsd_file_source_t gsd_file_source_;
};

} // end namespace vidtk

#endif // filter_video_metadata_process_h_
