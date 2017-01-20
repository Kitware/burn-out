/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _TAGGED_DATA_PORTS_H_
#define _TAGGED_DATA_PORTS_H_

#include <utilities/timestamp.h>
#include <utilities/video_modality.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>
#include <tracking_data/shot_break_flags.h>
#include <vil/vil_image_view.h>

  // define inputs
// <name> <type-name> <writer-type-name> <opt> <instance-name>
#define MDRW_SET_ONE                                                       \
  MDRW_INPUT ( timestamp,             vidtk::timestamp,                  timestamp_reader_writer,                  0) \
  MDRW_INPUT ( world_units_per_pixel, double,                            gsd_reader_writer,                        0) \
  MDRW_INPUT ( video_modality,        vidtk::video_modality,             video_modality_reader_writer,             0) \
  MDRW_INPUT ( video_metadata,        vidtk::video_metadata,             video_metadata_reader_writer,             0) \
  MDRW_INPUT ( video_metadata_vector, std::vector< vidtk::video_metadata >, video_metadata_vector_reader_writer,    0) \
  MDRW_INPUT ( shot_break_flags,      vidtk::shot_break_flags,           shot_break_flags_reader_writer,            0) \
  MDRW_INPUT ( src_to_ref_homography, vidtk::image_to_image_homography,  image_to_image_homography_reader_writer,  "src2ref") \
  MDRW_INPUT ( src_to_utm_homography, vidtk::image_to_utm_homography,    image_to_utm_homography_reader_writer,    "src2utm") \
  MDRW_INPUT ( src_to_wld_homography, vidtk::image_to_plane_homography,  image_to_plane_homography_reader_writer,  "src2wld") \
  MDRW_INPUT ( ref_to_wld_homography, vidtk::image_to_plane_homography,  image_to_plane_homography_reader_writer,  "ref2wld") \
  MDRW_INPUT ( wld_to_src_homography, vidtk::plane_to_image_homography,  plane_to_image_homography_reader_writer,  "wld2src") \
  MDRW_INPUT ( wld_to_ref_homography, vidtk::plane_to_image_homography,  plane_to_image_homography_reader_writer,  "wld2ref") \
  MDRW_INPUT ( wld_to_utm_homography, vidtk::plane_to_utm_homography,    plane_to_utm_homography_reader_writer,    "wld2utm")


// This is not ideal in that these inputs must be treated differently
// since they require an additional filename parameter.

#define MDRW_SET_IMAGE                                                         \
  MDRW_INPUT ( image_8,     vil_image_view < vxl_byte >,      image_view_reader_writer< vxl_byte >,    "image_8" ) \
  MDRW_INPUT ( image_16,    vil_image_view < vxl_uint_16 >,   image_view_reader_writer< vxl_uint_16 >, "image_16" ) \
  MDRW_INPUT ( image_mask,  vil_image_view < bool >,          image_view_reader_writer< bool >,        "mask" )

  // Make one macro that contains all inputs.
#define MDRW_SET MDRW_SET_ONE MDRW_SET_IMAGE

#endif // _TAGGED_DATA_PORTS_H_
