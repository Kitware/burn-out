/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <classifier/hashed_image_classifier.txx>

template class vidtk::hashed_image_classifier_model<double>;
template class vidtk::hashed_image_classifier_model<float>;

template class vidtk::hashed_image_classifier<vxl_byte,double>;
template class vidtk::hashed_image_classifier<vxl_byte,float>;

template class vidtk::hashed_image_classifier<vxl_uint_16,double>;
template class vidtk::hashed_image_classifier<vxl_uint_16,float>;

template std::ostream& vidtk::operator<<( std::ostream& os,
  const vidtk::hashed_image_classifier_model< float >& obj );
template std::ostream& vidtk::operator<<( std::ostream& os,
  const vidtk::hashed_image_classifier_model< double >& obj );

template std::ostream& vidtk::operator<<( std::ostream& os,
  const vidtk::hashed_image_classifier< vxl_byte, float >& obj );
template std::ostream& vidtk::operator<<( std::ostream& os,
  const vidtk::hashed_image_classifier< vxl_byte, double >& obj );
