/*ckwg +5
 * Copyright 2010-11 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef filter_video_metadata_process_h_
#define filter_video_metadata_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/shot_break_flags.h>
#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>
#include <utilities/checked_bool.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>
#include <vcl_vector.h>

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


  filter_video_metadata_process( vcl_string const& name );
  ~filter_video_metadata_process();

  // process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  // Input ports
  void set_source_metadata( video_metadata const& );
  VIDTK_INPUT_PORT( set_source_metadata, video_metadata const& );

  void set_source_metadata_vector( vcl_vector< video_metadata > const& );
  VIDTK_INPUT_PORT( set_source_metadata_vector, vcl_vector< video_metadata > const& );

  // Output ports
  vcl_vector< video_metadata > const& output_metadata_vector() const;
  VIDTK_OUTPUT_PORT( vcl_vector< video_metadata > const&, output_metadata_vector );

  video_metadata const & output_metadata() const;
  VIDTK_OUTPUT_PORT( video_metadata const &, output_metadata );

protected:
  bool is_good(video_metadata const * in);
  checked_bool is_good_location( video_metadata::lat_lon_t const& new_loc,
                                 video_metadata::lat_lon_t & prev_loc ) const;

  config_block config_;

  // Configuration parameters
  bool disable_;
  double max_hfov_;
  double max_alt_delta_;
  double max_lat_delta_;
  double max_lon_delta_;

  video_metadata const * input_single_;
  vcl_vector< video_metadata > const * input_multi_;
  video_metadata invalid_;
  vcl_vector< video_metadata > output_vector_;
  vxl_uint_64 previous_time_;
  double previous_alt_;
  video_metadata::lat_lon_t prev_platform_loc_;
  video_metadata::lat_lon_t prev_frame_center_;
  video_metadata::lat_lon_t prev_corner_ul_;
  video_metadata::lat_lon_t prev_corner_ur_;
  video_metadata::lat_lon_t prev_corner_lr_;
  video_metadata::lat_lon_t prev_corner_ll_;
};

} // end namespace vidtk

#endif // filter_video_metadata_process_h_
