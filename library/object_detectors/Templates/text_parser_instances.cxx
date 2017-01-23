/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/text_parser.txx>

template class vidtk::text_parser_model_group< vxl_byte >;
template class vidtk::text_parser_model_group< vxl_uint_16 >;

template class vidtk::text_parser< vxl_byte >;
template class vidtk::text_parser< vxl_uint_16 >;
