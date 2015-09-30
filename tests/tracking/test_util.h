/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef test_util_h_
#define test_util_h_

#include <vcl_string.h>
#include <vcl_vector.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>

#include <utilities/log.h>

/// \internal
/// Parse a string into a matrix.
///
/// The string is of the form "a b c d; e f g h", with ";" separating
/// rows of the matrix.
vnl_matrix<double> matrix( vcl_string const& s )
{
  vcl_istringstream istr( s );
  vcl_vector<double> numbers;
  unsigned M = 0;
  unsigned N = 0;
  unsigned i = 0;
  double v;
  while( istr >> v )
  {
    numbers.push_back( v );
    unsigned j = 1;
    while( (N == 0 || j < N) && ( istr >> v ) )
    {
      numbers.push_back( v );
      ++j;
    }
    if( N == 0 )  // on the first row, just count the columns and read the semicolon
    {
      N = j;
      // clear the failbit, for the case that the first row ends in a
      // semicolon (as opposed to EOF).
      istr.clear( istr.rdstate() & ~istr.failbit );
    }
    log_assert( j == N, "not enough elements on row " << i );
    ++i;
    char semicolon;
    istr >> semicolon;
    if( istr.eof() )
    {
      break;
    }
    log_assert( semicolon == ';' && !istr.fail(), "failed to read semicolon after line " << i-1 );
  }
  M = i;
  return vnl_matrix<double>( M, N, numbers.size(), &numbers[0] );
}


/// \internal
/// Parse a string into a vector.
///
/// The string is of the form "a b c d".
vnl_vector<double> vector( vcl_string const& s )
{
  vcl_istringstream istr( s );
  return vnl_vector<double>::read( istr );
}

#endif // test_util_h_
