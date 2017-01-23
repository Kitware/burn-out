/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/variable_image_erode.txx>


#define VIDTK_VARIABLE_ERODE_INSTANTIATE(PT, HT) \
template void vidtk::variable_image_erode( const vil_image_view<PT >& src, \
                                           const vil_image_view<HT >& elem_ids, \
                                           const vidtk::structuring_element_vector& se_vector, \
                                           const vidtk::offset_vector& offset_vector, \
                                           const HT & max_id, \
                                           vil_image_view<PT >& dst ); \
template void vidtk::variable_image_erode( const vil_image_view<PT >& src, \
                                           const vil_image_view<HT >& elem_ids, \
                                           const vidtk::structuring_element_vector& se_vector, \
                                           vil_image_view<PT >& dst );


VIDTK_VARIABLE_ERODE_INSTANTIATE(bool,unsigned);
VIDTK_VARIABLE_ERODE_INSTANTIATE(vxl_byte,unsigned);
VIDTK_VARIABLE_ERODE_INSTANTIATE(vxl_uint_16,unsigned);

#undef VIDTK_VARIABLE_ERODE_INSTANTIATE
