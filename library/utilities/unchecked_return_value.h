/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef unchecked_return_value_h_
#define unchecked_return_value_h_

/**\file
   \brief
   Method and field definitions of an unchecked return value.
*/

#include <utilities/deprecation.h>

#include <stdexcept>
#include <string>

namespace vidtk
{
///Extends the std::runtime_error class to create unchecked return error messages.
class VIDTK_DEPRECATED("Obsolete") unchecked_return_value
  : public std::runtime_error
{
public:
  ///Constructor: pass an error message.
  unchecked_return_value( std::string const& v );
};

} // end namespace vidtk

#endif // unchecked_return_value_h_
