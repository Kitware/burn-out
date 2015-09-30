/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _TAGGED_DATA_READER_PROCESS_H_
#define _TAGGED_DATA_READER_PROCESS_H_

#include "base_reader_process.h"

#include <utilities/timestamp.h>
#include <utilities/video_modality.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>
#include <tracking/shot_break_flags.h>
#include <vil/vil_image_view.h>


namespace vidtk {

// ----------------------------------------------------------------
/** Metadata reader process.
 *
 * This process reads a tagged metadata file and produces outputs.
 * There are no enables for individual tags like in the writer.  The
 * tags found in the input file drive the outputs. If there is a tag,
 * then it is output. If there is no tag, then no output on that port.
 *
 * If an output is not needed, then don't hook it up.
 *
 * Note that the timestamp vector output is simulated by generating a
 * vector of one from the last timestamp.
 */
class tagged_data_reader_process
  : public base_reader_process
{
public:
  typedef tagged_data_reader_process self_type;

  tagged_data_reader_process(vcl_string const& name);
  virtual ~tagged_data_reader_process();

  // Process interface
  virtual bool set_params(config_block const&);
  virtual bool initialize(base_io_process::internal_t);

  // define inputs
// <name> <type-name> <writer-type-name> <opt> <instance-name>
#define MDRW_SET_ONE                                                         \
  MDRW_INPUT ( timestamp,             vidtk::timestamp,                  timestamp_reader_writer,                  0) \
  MDRW_INPUT ( world_units_per_pixel, double,                            gsd_reader_writer,                        0) \
  MDRW_INPUT ( video_modality,        vidtk::video_modality,             video_modality_reader_writer,             0) \
  MDRW_INPUT ( video_metadata,        vidtk::video_metadata,             video_metadata_reader_writer,             0) \
  MDRW_INPUT ( video_metadata_vector, vcl_vector< vidtk::video_metadata >, video_metadata_vector_reader_writer,    0) \
  MDRW_INPUT ( shot_break_flags,      vidtk::shot_break_flags,           shot_break_flags_reader_writer,            0) \
  MDRW_INPUT ( src_to_ref_homography, vidtk::image_to_image_homography,  image_to_image_homography_reader_writer, "src2ref") \
  MDRW_INPUT ( src_to_utm_homography, vidtk::image_to_utm_homography,    image_to_utm_homography_reader_writer,   "src2utm") \
  MDRW_INPUT ( src_to_wld_homography, vidtk::image_to_plane_homography,  image_to_plane_homography_reader_writer, "src2wld") \
  MDRW_INPUT ( ref_to_wld_homography, vidtk::image_to_plane_homography,  image_to_plane_homography_reader_writer, "ref2wld") \
  MDRW_INPUT ( wld_to_src_homography, vidtk::plane_to_image_homography,  plane_to_image_homography_reader_writer, "wld2src") \
  MDRW_INPUT ( wld_to_ref_homography, vidtk::plane_to_image_homography,  plane_to_image_homography_reader_writer, "wld2ref") \
  MDRW_INPUT ( wld_to_utm_homography, vidtk::plane_to_utm_homography,    plane_to_utm_homography_reader_writer,   "wld2utm")

// This is not ideal
#define MDRW_SET_IMAGE                                                  \
  MDRW_INPUT ( image,                 vil_image_view < vxl_byte >,       image_view_reader_writer< vxl_byte >,   "image" ) \
  MDRW_INPUT ( image_mask,            vil_image_view < bool >,           image_view_reader_writer< bool >,       "mask" )

#define MDRW_SET MDRW_SET_ONE MDRW_SET_IMAGE


#define MDRW_INPUT(N,T,W,I)                             \
public:                                                 \
  T const& get_output_ ## N() { return m_ ## N; }       \
VIDTK_OUTPUT_PORT(  T const&, get_output_ ## N );       \
private:                                                \
  T m_ ## N;

  MDRW_SET  // apply macro over inputs
#undef MDRW_INPUT


 public:

  // timestamp vector output port
  vidtk::timestamp::vector_t  get_output_timestamp_vector() const;
  VIDTK_OUTPUT_PORT( vidtk::timestamp::vector_t, get_output_timestamp_vector );


private:
  virtual int error_hook( int status);
  virtual int pre_step_hook();
  virtual void post_step_hook();

  int m_state;

}; // end class tagged_data_reader_process

} // end namespace

#endif /* _TAGGED_DATA_READER_PROCESS_H_ */
