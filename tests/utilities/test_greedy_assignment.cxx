/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_random.h>
#include <testlib/testlib_test.h>

#include <utilities/greedy_assignment.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
test_output_structure()
{
  vcl_cout << "Testing output for 10x5 cost matrix\n";

  vnl_matrix<double> cost1( 10, 5 );
  vcl_vector<unsigned> assn1 = vidtk::greedy_assignment( cost1 );
  TEST( "Assignment vector length", assn1.size(), 10 );

  vcl_cout << "Testing output for 5x10 cost matrix\n";

  vnl_matrix<double> cost2( 5, 10 );
  vcl_vector<unsigned> assn2 = vidtk::greedy_assignment( cost2 );
  TEST( "Assignment vector length", assn2.size(), 5 );
}

void
test_valid_assignment()
{
  vcl_cout << "Testing that the output is valid\n";

  vnl_random rand;
  unsigned M = rand.lrand32( 5, 15 );
  unsigned N = rand.lrand32( 5, 15 );

  vnl_matrix<double> cost( M, N );
  for( unsigned i = 0; i < M; ++i )
  {
    for( unsigned j = 0; j < N; ++j )
    {
      cost(i,j) = rand.drand64();
    }
  }

  vcl_vector<unsigned> assn = vidtk::greedy_assignment( cost );

  TEST( "Assignment vector length", assn.size(), M );

  bool good = true;
  vcl_vector<bool> used_tgt( N, false );
  for( unsigned i = 0; i < M; ++i )
  {
    if( assn[i] != unsigned(-1) )
    {
      if( used_tgt[ assn[i] ] )
      {
        vcl_cout << "ERROR: target " << assn[i]
                 << " reused with src " << i << "\n";
        good = false;
      }
      else
      {
        used_tgt[ assn[i] ] = true;
      }
    }
  }
  if( ! good )
  {
    vcl_cout << "Cost =\n" << cost << "\n";
    vcl_cout << "Assignment=\n";
    for( unsigned i = 0; i < M; ++i )
    {
      vcl_cout << "   " << i << " -> " << assn[i] << "\n";
    }
  }
  TEST( "Assignment is valid", good, true );
}


} // end anonymous namespace

int test_greedy_assignment( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "greedy_assignment" );

  test_output_structure();
  test_valid_assignment();

  return testlib_test_summary();
}
