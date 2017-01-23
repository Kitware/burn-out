/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <string>
#include <limits>
#include <track_oracle/scoring_framework/aries_interface/aries_interface.h>
#include <testlib/testlib_test.h>


using std::map;
using std::numeric_limits;
using std::string;

namespace { // anon

void test_maps()
{
  map< size_t, string >::const_iterator i2a_probe;
  map< size_t, string >::const_iterator i2pvo_probe;
  size_t idx = numeric_limits<size_t>::max();

  // testing A2I
  {
    bool exists = false;
    try
    {
      idx = vidtk::aries_interface::activity_to_index( "Walking" );
      exists = true;
    }
    catch (vidtk::aries_interface_exception& /*e*/)
    {
    }
    TEST( "'Walking' exists", exists, true );
    TEST( "'Walking' is index 4", idx, 4);
  }
  {
    bool exists = false;
    try
    {
      idx = vidtk::aries_interface::activity_to_index( "PersonWalking" );
      exists = true;
    }
    catch (vidtk::aries_interface_exception& /*e*/)
    {
    }
    TEST( "'PersonWalking' exists", exists, true );
    TEST( "'PersonWalking' is index 4", idx, 4);
  }
  {
    bool exists = false;
    try
    {
      idx = vidtk::aries_interface::activity_to_index( "Foobar" );
      exists = true;
    }
    catch (vidtk::aries_interface_exception& /*e*/)
    {
    }
    TEST( "'Foobar' does not exist", exists, false );
  }

  // testing I2A
  map< size_t, string > i2a_map = vidtk::aries_interface::index_to_activity_map();

  idx = vidtk::aries_interface::activity_to_index( "PersonRunning" );
  i2a_probe = i2a_map.find( idx );
  TEST( "Self-consistent on 'PersonRunning'", i2a_probe->second, "PersonRunning" );
  idx = vidtk::aries_interface::activity_to_index( "Running" );
  i2a_probe = i2a_map.find( idx );
  TEST( "Alias for 'Running' is 'PersonRunning'", i2a_probe->second, "PersonRunning" );
  i2a_probe = i2a_map.find( 3 );
  idx = vidtk::aries_interface::activity_to_index( i2a_probe->second );
  TEST( "Self-consistent on index 3", idx, 3 );

  // testing A2PVO

  {
    bool exists = false;
    string pvo = "";
    try
    {
      pvo = vidtk::aries_interface::activity_to_PVO( "Standing" );
      exists = true;
    }
    catch ( vidtk::aries_interface_exception& /*e*/ )
    {
    }
    TEST( "'Standing' exists", exists, true );
    TEST( "'Standing' is a 'person' event", pvo, vidtk::aries_interface::PVO_PERSON );
  }

  {
    bool exists = false;
    string pvo = "";
    try
    {
      pvo = vidtk::aries_interface::activity_to_PVO( "Turning" );
      exists = true;
    }
    catch ( vidtk::aries_interface_exception& /*e*/ )
    {
    }
    TEST( "'Turning' exists", exists, true );
    TEST( "'Turning' is a 'vehicle' event", pvo, vidtk::aries_interface::PVO_VEHICLE );
  }


  {
    string pvo = vidtk::aries_interface::activity_to_PVO( "VEHICLE_NO_VEHICLE" );
    TEST( "'VEHICLE_NO_VEHICLE' has no pvo value", pvo, vidtk::aries_interface::PVO_NULL );
  }

  // testing I2PVO
  map< size_t, string > i2pvo_map = vidtk::aries_interface::index_to_PVO_map();

  i2pvo_probe = i2pvo_map.find( vidtk::aries_interface::activity_to_index( "Standing"));
  TEST( "'Standing' index to PVO is a 'person'",
        i2pvo_probe->second,
        vidtk::aries_interface::PVO_PERSON );

  i2pvo_probe = i2pvo_map.find( vidtk::aries_interface::activity_to_index( "Turning" ));
  TEST( "'Turning' index to PVO is a 'vehicle'",
        i2pvo_probe->second,
        vidtk::aries_interface::PVO_VEHICLE );

  i2pvo_probe = i2pvo_map.find( vidtk::aries_interface::activity_to_index( "VEHICLE_NO_VEHICLE" ));
  TEST( "'VEHICLE_NO_VEHICLE' index to PVO is 'null'",
        i2pvo_probe->second,
        vidtk::aries_interface::PVO_NULL );

}

} // anon namespace

int test_aries_interface( int /*argc*/, char * /*argv*/[] )
{
  testlib_test_start( "test_aries_interface" );

  test_maps();

  return testlib_test_summary();
}
