/*ckwg +5
 * Copyright 2010-11 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "amhi_data.h"

namespace vidtk
{
// Deep copies all information from copy to this class
void 
amhi_data
::deep_copy( amhi_data const & copy)
{
  //shallow copy
  *this = copy;

  //deep copy
  this->weight.clear();
  this->weight.deep_copy(copy.weight);
  this->image.clear();
  this->image.deep_copy(copy.image);
}

}//end namespace vidtk
