/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_string_to_vector_h_
#define vidtk_string_to_vector_h_

#include <iostream>
#include <vector>
#include <string>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace vidtk
{

/**
 * @brief Convert string to vector to types.
 *
 * This function converts a string that contains character
 * representations of the templated type into a vector of values of
 * type T.
 *
 * If there is a conversion error, \b false is returned and the
 * contents of the output vector are undefined.
 *
 * @param[in] str Values in string format,
 * @param[out] out Vector of values,
 * @param[in] delims Delimiters to split the string by,
 *
 * @tparam T Type for elements of output vector
 *
 * @return \b true if all values have been converted without error. \b
 * false if conversion error.
 */
template< typename T >
bool string_to_vector( const std::string& str,
                       std::vector< T >& out,
                       const std::string delims = "\n\t\v ," )
{
  out.clear();

  std::vector< std::string > parsed_string;

  boost::split( parsed_string, str,
                boost::is_any_of( delims ),
                boost::token_compress_on );

  try
  {
    BOOST_FOREACH( std::string s, parsed_string )
    {
      if( !s.empty() )
      {
        out.push_back( boost::lexical_cast< T >( s ) );
      }
    }
  }
  catch( boost::bad_lexical_cast& )
  {
    return false;
  }

  return true;
}

} // end namespace vidtk

#endif // vidtk_string_to_vector_h_
