/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/threshold_image_process.txx>

#include <vxl_config.h>

INSTANTIATE_THRESHOLD_IMAGE_PROCESS( vxl_byte );
INSTANTIATE_THRESHOLD_IMAGE_PROCESS( vxl_uint_16 );
INSTANTIATE_THRESHOLD_IMAGE_PROCESS( float );
INSTANTIATE_THRESHOLD_IMAGE_PROCESS( double );
