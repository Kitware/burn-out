/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_IMAGE_OBJECT_UTILS_H_
#define _VIDTK_IMAGE_OBJECT_UTILS_H_

#include <tracking_data/image_object.h>

#include <vil/vil_image_view.h>

namespace vidtk {

/**
 * @brief Clip image chip for detection
 *
 * This function clips a portion of the input image based on the
 * detection bounding box in the image object. The clipped image is
 * expanded, on all four sides, by the amount specified in the \c
 * border parameter. If the \c border parameter is zero (the
 * default), then the clipped image is not expanded.
 *
 * The returned image clipping (which is an image itself) is a \e
 * shallow copy of the original, so you will need to make a deep copy
 * if you can't control the lifetime of the original image.
 *
 * @param input_image Image that is to be clipped
 * @param detection Detection object that contains the bounding box
 * @param border Number of pixels to expand the image clipping
 *
 * @return \e Shallow copy of the input image that has been clipped to
 * the detection bounding box.
 */
template< typename T >
vil_image_view<T> clip_image_for_detection( vil_image_view< T >const& input_image,
                                            image_object_sptr const detection,
                                            unsigned border = 0 );
} // end namespace

#endif /* _VIDTK_IMAGE_OBJECT_UTILS_H_ */
