/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/external_settings.h>

#include <string>
#include <vector>

using namespace vidtk;

#define settings_macro( add_param ) \
  add_param( var1, unsigned, 10, "Test Variable 1" ) \
  add_param( var2, double, 0.3, "Test Variable 2" )

init_external_settings( test_settings1, settings_macro );

#undef settings_macro

#define settings_macro( add_param, add_array_param, add_enum_param ) \
  add_param( var1, unsigned, 10, "Test Variable 1" ) \
  add_array_param( var2, double, 3, "0.2 0.2 0.1", "Test Variable 2" ) \
  add_enum_param( var3, (V1) (V2) (V3), V2, "Test Variable 3" )

init_external_settings3( test_settings2, settings_macro );

#undef settings_macro

int test_external_settings( int argc, char *argv[] )
{
  testlib_test_start( "external_settings" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    std::string data_dir( argv[1] );
    std::string filename1 = data_dir + "/simple_config_file.conf";
    std::string filename2 = data_dir + "/simple_config_file2.conf";

    test_settings1 test1;
    test_settings2 test2;

    TEST( "Default settings value 1", test1.var1, 10 );
    TEST( "Default settings value 2", test1.var2, 0.3 );
    TEST( "Default settings value 3", test2.var1, 10 );
    TEST( "Default settings value 4", test2.var2.size(), 3 );
    TEST( "Default settings value 5", test2.var2[1], 0.2 );
    TEST( "Default settings value 6", test2.var3, test_settings2::V2 );

    TEST( "Parse from config file 1", test1.load_config_from_file( filename1 ), true );
    TEST( "Parse from config file 2", test2.load_config_from_file( filename2 ), true );

    TEST( "Loaded settings value 1", test1.var1, 12 );
    TEST( "Loaded settings value 2", test1.var2, 0.25 );
    TEST( "Loaded settings value 3", test2.var1, 12 );
    TEST( "Loaded settings value 4", test2.var2.size(), 3 );
    TEST( "Loaded settings value 5", test2.var2[2], 0.05 );
    TEST( "Loaded settings value 6", test2.var3, test_settings2::V3 );
  }

  return testlib_test_summary();
}
