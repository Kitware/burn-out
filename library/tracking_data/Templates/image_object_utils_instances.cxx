/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/image_object_util.txx>

template vil_image_view< vxl_byte >
vidtk::clip_image_for_detection( vil_image_view< vxl_byte >const& input_image,
                                 vidtk::image_object_sptr const detection,
                                 unsigned border);

template vil_image_view< vxl_uint_16 >
vidtk::clip_image_for_detection( vil_image_view< vxl_uint_16 >const& input_image,
                                 vidtk::image_object_sptr const detection,
                                 unsigned border);

template vil_image_view< bool >
vidtk::clip_image_for_detection( vil_image_view< bool >const& input_image,
                                 vidtk::image_object_sptr const detection,
                                 unsigned border);

template vil_image_view< float >
vidtk::clip_image_for_detection( vil_image_view< float >const& input_image,
                                 vidtk::image_object_sptr const detection,
                                 unsigned border);
