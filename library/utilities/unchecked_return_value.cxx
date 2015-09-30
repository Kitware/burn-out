/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/unchecked_return_value.h>

namespace vidtk
{


unchecked_return_value
::unchecked_return_value( vcl_string const& v )
  : vcl_runtime_error( v )
{
}


} // end namespace vidtk
