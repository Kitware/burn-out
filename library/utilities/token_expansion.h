/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


/** \file
    \brief
    Utility class to take a string populated with magic tokens and
    expand that, replacing tokens with their full value.

    Current support:
    $ENV{ ... } - uses getenv(...) to replace ... with its value
                  in the environment variables.
*/

#ifndef __TOKEN_EXPANSION_H__
#define __TOKEN_EXPANSION_H__

#include <vcl_string.h>

namespace vidtk
{

class token_expansion
{

public:
  ///Expands a string, replacing magic tokens with full value
  static vcl_string expand_token( const vcl_string initial_string );

};

} // namespace vidtk

#endif //__TOKEN_EXPANSION_H__
