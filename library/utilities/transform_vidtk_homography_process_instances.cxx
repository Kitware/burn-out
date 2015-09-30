/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/transform_vidtk_homography_process.txx>

//template< class InSrcType, class InDestType, class OutSrcType, class OutDestType >
template class vidtk::transform_vidtk_homography_process< vidtk::timestamp, vidtk::timestamp,
                                                          vidtk::timestamp, vidtk::timestamp  >;
template class vidtk::transform_vidtk_homography_process< vidtk::timestamp, vidtk::plane_ref_t,
                                                          vidtk::timestamp, vidtk::utm_zone_t >;
template class vidtk::transform_vidtk_homography_process< vidtk::timestamp, vidtk::timestamp,
                                                          vidtk::timestamp, vidtk::plane_ref_t >;
template class vidtk::transform_vidtk_homography_process< vidtk::timestamp, vidtk::plane_ref_t ,
                                                          vidtk::timestamp, vidtk::plane_ref_t >;
