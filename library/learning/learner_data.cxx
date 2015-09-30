/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "learner_data.h"
#include <vbl/vbl_smart_ptr.txx>

VBL_SMART_PTR_INSTANTIATE( vidtk::learner_training_data );

VBL_SMART_PTR_INSTANTIATE( vidtk::learner_data );

vcl_ostream& operator<< (vcl_ostream& os, const vidtk::learner_training_data &p)
{
  os << p.label() << " " << p.vectorize();
  return os;
}
