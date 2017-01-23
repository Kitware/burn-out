/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/bag_of_words_model.txx>

#define INSTANTIATE( WORDTYPE, MAPTYPE ) \
template class vidtk::bag_of_words_model< WORDTYPE , MAPTYPE >; \
template std::ostream& vidtk::operator<<( \
  std::ostream& out,vidtk::bag_of_words_model< WORDTYPE , MAPTYPE >& model ); \

INSTANTIATE( float, double );
INSTANTIATE( double, double );

#undef INSTANTIATE
