/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "token_expansion.h"

#include <utilities/log.h>
#include <vcl_iostream.h>
#include <algorithm>

namespace vidtk
{

vcl_string
token_expansion
::expand_token ( const vcl_string initial_string )
{
  vcl_string remaining_str = initial_string;
  vcl_string final_string = "";

  while ( remaining_str.length() > 0 )
  {
    size_t index_tok = remaining_str.find("$ENV");
    size_t index_st_curl = remaining_str.find("{");
    size_t index_ed_curl = remaining_str.find("}");

    //ensure increasing index order, i.e. $ENV { ... }, otherwise this isn't the sequence we want.
    //in that case, skip to the end of } and start again.
    bool ordered = ( (index_tok < index_st_curl) ? (index_st_curl < index_ed_curl) : false );
    if ( ordered &&
         ( index_tok != vcl_string::npos  &&
           index_st_curl != vcl_string::npos &&
           index_ed_curl != vcl_string::npos) )
    {
      final_string += remaining_str.substr(0, index_tok);

      // must make length - 1 since we added +1 at the start
      vcl_string var = remaining_str.substr(index_st_curl + 1, (index_ed_curl - index_st_curl - 1) );

      const char * v = var.c_str();
      char * env_expansion = getenv( v );

      if ( env_expansion == NULL )
      {
        log_error("Environment Variable: " << var << " not found" << vcl_endl);
        final_string = "";
        return final_string;
      }

      final_string += env_expansion;

      index_ed_curl = ( index_ed_curl != vcl_string::npos ? index_ed_curl:remaining_str.length() );
      remaining_str = remaining_str.substr( index_ed_curl + 1 );
    }
    else
    {
      final_string += remaining_str;
      break;
    }
  }

  return final_string;
}

} // namespace vidtk
