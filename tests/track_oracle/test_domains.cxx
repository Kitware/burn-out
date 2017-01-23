/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vector>
#include <map>
#include <sstream>
#include <track_oracle/track_base.h>
#include <track_oracle/track_field.h>

#include <testlib/testlib_test.h>


using std::map;
using std::ostringstream;
using std::vector;

namespace { // anon

using vidtk::track_oracle;
using vidtk::track_handle_type;
using vidtk::domain_handle_type;

struct test_struct: public vidtk::track_base< test_struct >
{
  vidtk::track_field<unsigned>& id;
  test_struct():
    id( Track.add_field<unsigned>( "id" ))
  {}
};

void test_domains()
{
  TEST( "Empty oracle is empty",
        track_oracle::get_domain( vidtk::DOMAIN_ALL ).empty(),
        true );

  // add some entries
  vector< track_handle_type > odd_entries;
  vector< track_handle_type > even_entries;

  test_struct s;

  // odd == 1,3,5; even = 0,2,4
  for (unsigned i=0; i<6; ++i)
  {
    track_handle_type h = s.create();
    s( h ).id() = i;
    if ( i % 2 == 0 )
    {
      even_entries.push_back( h );
    }
    else
    {
      odd_entries.push_back( h );
    }
  }

  // test adding them to their own domains
  domain_handle_type d_odd =
    track_oracle::create_domain(
      track_oracle::track_to_generic_handle_list( odd_entries ));
  domain_handle_type d_even =
    track_oracle::create_domain(
      track_oracle::track_to_generic_handle_list( even_entries ));

  TEST( "Even and odd domains are different", d_odd != d_even, true );
  {
    size_t n = track_oracle::get_domain( vidtk::DOMAIN_ALL ).size();
    ostringstream oss;
    oss << "Oracle::all has " << n << " entries (should be 0); see comments in track_oracle_impl.cxx";
    TEST( oss.str().c_str(), n, 0 );
  }
  {
    size_t n = track_oracle::get_domain( d_odd ).size();
    ostringstream oss;
    oss << "Oracle::odd has " << n << " entries (should be 3)";
    TEST( oss.str().c_str(), n, 3 );
  }
  {
    size_t n = track_oracle::get_domain( d_even ).size();
    ostringstream oss;
    oss << "Oracle::even has " << n << " entries (should be 3)";
    TEST( oss.str().c_str(), n, 3 );
  }

  // append to a domain
  bool rc = track_oracle::add_to_domain(
    track_oracle::track_to_generic_handle_list( even_entries ),
    d_odd );
  TEST( "Adding even to odd succeeded", rc, true );

  {
    vidtk::track_handle_list_type tracks
      = track_oracle::generic_to_track_handle_list(
        track_oracle::get_domain( d_odd ));
    size_t n = tracks.size();

    ostringstream oss;
    oss << "Oracle::odd now has " << n << " entries (should be 6)";
    TEST( oss.str().c_str(), n, 6 );

    // are the entries [0..5] present?
    map<unsigned int, int> present;
    for (unsigned i=0; i<6; ++i)
    {
      ++present[ s( tracks[i] ).id() ];
    }
    for (unsigned i=0; i<6; ++i)
    {
      ostringstream oss2;
      oss2 << "ID " << i << " present " << present[i] << " time (should be 1)";
      TEST( oss2.str().c_str(), present[i], 1 );
    }
  }
}

} // anon namespace

int test_domains( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_domains" );

  test_domains();

  return testlib_test_summary();
}

