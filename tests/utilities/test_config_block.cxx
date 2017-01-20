/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <exception>
#include <fstream>
#include <vxl_config.h>
#include <testlib/testlib_test.h>
#include <vul/vul_file.h>

#include <utilities/config_block.h>
#include <utilities/config_block_parser.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
test_subblock()
{
  vidtk::config_block blk, parent;
  blk.add_parameter( "v", "Dummy variable" );
  parent.add_subblock(blk, "Meaning_of_everything");
  TEST( "Setting subblock", parent.set( "Meaning_of_everything:v", "42" ), true );
  int v = parent.get<int>( "Meaning_of_everything:v" );
  TEST( "Values are correct", v, 42);
  TEST( "bad move", parent.move_subblock("garbage", "more_garbage"), false);
  TEST( "good move", parent.move_subblock("Meaning_of_everything", "question"), true);
  v = parent.get<int>( "question:v");
  TEST( "Values are correct", v, 42);
}

void
test_array()
{
  std::cout << "Test array\n";

  double vals[4] = { 0.0, 1.1, 2.2, 3.3 };

  vidtk::config_block blk;
  blk.add_parameter( "v", "Dummy variable" );

  TEST( "Set value (4 doubles)", blk.set( "v", "0.1 1.2 2.3 3.4" ), true );

  bool tmp_bool = blk.get<double, 4>( "v", vals );
  TEST( "Get double[4] value", tmp_bool, true );
  TEST( "Values are correct", true,
        vals[0]==0.1 &&
        vals[1]==1.2 &&
        vals[2]==2.3 &&
        vals[3]==3.4 );

  double vals5[5] = { 1.0, 2.0 };
  tmp_bool = blk.get<double, 5>( "v", vals5 );
  TEST( "Get double[5] value fails", tmp_bool, false );
  TEST( "Original values are untouched", vals5[0], 1.0 );
  TEST( "Original values are untouched", vals5[1], 2.0 );

  double vals3[3] = { 1.0, 2.0 };
  tmp_bool = blk.get<double, 3>( "v", vals3 );
  TEST( "Get double[3] value fails", tmp_bool, false );
  TEST( "Original values are untouched", vals3[0], 1.0 );
  TEST( "Original values are untouched", vals3[1], 2.0 );
}

void
test_bool()
{
  std::cout << "Test bool\n";

  char const* const true_value_strings[] = { "true", "1", "yes", "on", NULL };
  char const* const false_value_strings[] = { "false", "0", "no", "off", NULL };

  char const* const* true_str = &true_value_strings[0];
  while (*true_str)
  {
    vidtk::config_block blk;
    blk.add_parameter( "v", "Dummy variable" );

    bool val;
    std::cout << "Testing " << *true_str << " as a true value" << std::endl;
    TEST( "Set value", blk.set( "v", *true_str ), true );
    val = blk.get<bool>( "v" );
    TEST( "Correct value", val, true );

    ++true_str;
  }

  char const* const* false_str = &false_value_strings[0];
  while (*false_str)
  {
    vidtk::config_block blk;
    blk.add_parameter( "v", "Dummy variable" );

    bool val;
    std::cout << "Testing " << *false_str << " as a false value" << std::endl;
    TEST( "Set value", blk.set( "v", *false_str ), true );
    val = blk.get<bool>( "v" );
    TEST( "Correct value", val, false );

    ++false_str;
  }
}

void
test_get_byte()
{
  std::cout << "\n\n\nTest get byte\n\n";

  vidtk::config_block blk;
  blk.add_parameter( "v", "Test value" );

  {
    blk.set( "v", "34" );
    vxl_byte v = blk.get<vxl_byte>("v");
    TEST( "Value is correct", v, 34 );
    TEST( "Unconditional get", blk.get<vxl_byte>("v"), 34 );
  }

  {
    blk.set( "v", "34" );
    unsigned char v = blk.get<unsigned char>("v");
    TEST( "Value is correct", v, 34 );
    TEST( "Unconditional get", blk.get<unsigned char>("v"), 34 );
  }

  {
    blk.set( "v", "-34" );
    signed char v = blk.get<signed char>("v");
    TEST( "Value is correct", v, -34 );
    TEST( "Unconditional get", blk.get<signed char>("v"), -34 );
  }

  {
    blk.set( "v", "3" );
    char v =  blk.get<char>("v");
    TEST( "Value is correct", v, '3' );
    TEST( "Unconditional get", blk.get<char>("v"), '3' );
  }
}

void
test_string()
{
  std::cout << "Testing strings\n";
  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "v", "dummy value" );
    std::string tmp;

    try
    {
      tmp =  blk.get<std::string>( "v" );
      TEST( "Expected an exception", true, false );
    }
    catch( std::exception& ex )
    {
      TEST( "got an exception for un init value", true, true );
    }

    blk.set( "v", "expected" );
    tmp = blk.get<std::string>( "v" );
    TEST( "Received correct value",
          tmp, "expected" );
  }
  catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, false );
  }

  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "v", "default", "dummy value" );

    std::string tmp = blk.get<std::string>( "v" );
    TEST( "Received correct value",
          tmp, "default" );

    blk.set( "v", "expected" );
    tmp = blk.get<std::string>( "v" );
    TEST( "Received correct value",
          tmp, "expected" );
  }
  catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, false );
  }

  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "v", "", "dummy value" );

    std::string tmp = blk.get<std::string>( "v" );

    TEST( "Received correct value", tmp, "" );
  }
  catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, false );
  }
}


void
test_parse_relative_filename( std::string const& data_dir )
{
  std::cout << "Testing relative file parsing with a filename\n";
  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "fixed", "a fixed path" );
    blk.add_parameter( "rel", "a relative path" );
    blk.add_parameter( "rel2", "a relative path" );

    blk.parse( data_dir+"/relative_filename.conf" );
    TEST( "Fixed path is unchanged", blk.get<std::string>( "fixed" ), "fixed_path" );
    std::cout << "Relative path: \"" << blk.get<std::string>( "rel" ) << "\"\n";
    TEST( "Relative path has been corrected",
          blk.get<std::string>( "rel" ),
          data_dir+"/relative_filename.conf" );

    TEST( "1st relative file exists",
          vul_file::exists( blk.get<std::string>( "rel" ) ), true );

    TEST( "1st relative path is same as 2nd relative path",
          blk.get<std::string>( "rel" ), blk.get<std::string>( "rel2" ) );
  }
  catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, false );
  }

  std::cout << "Testing relative file parsing with istream\n";
  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "fixed", "a fixed path" );
    blk.add_parameter( "rel", "a relative path" );
    blk.add_parameter( "rel2", "a relative path" );

    std::ifstream fstr( (data_dir+"/relative_filename.conf").c_str() );
    TEST( "Stream opened", fstr.good(), true );
    vidtk::config_block_parser parser;
    parser.set_input_stream( fstr );
    parser.parse( blk );
    TEST( "Fixed path is unchanged", blk.get<std::string>( "fixed" ), "fixed_path" );
    std::cout << "Relative path: \"" << blk.get<std::string>( "rel" ) << "\"\n";
    TEST( "1st relative path has been corrected",
          blk.get<std::string>( "rel" ),
          vul_file::get_cwd()+"/relative_filename.conf" );

    //Can't test for existence, because it depends on the directory in
    //which this executable is run.
    //
    //TEST( "Relative file exists",
    //      vul_file::exists( blk.get<std::string>( "rel" ) ), true );

    TEST( "1st relative path is same as 2nd relative path",
          blk.get<std::string>( "rel" ), blk.get<std::string>( "rel2" ) );
  }
  catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, false );
  }
}


void
test_include( std::string const& data_dir )
{
  std::cout << "Testing includes\n";
  try
  {
    // fill in config block with parameters
    vidtk::config_block blk;
    blk.add_parameter( "inc1_var1", "var");
    blk.add_parameter( "inc1_var2", "var");
    blk.add_parameter( "inc2_var1", "var");
    blk.add_parameter( "inc2_var2", "var");

    blk.add_parameter( "fixed", "a fixed path" );
    blk.add_parameter( "rel", "a relative path" );
    blk.add_parameter( "rel2", "a relative path" );

    vidtk::config_block_parser parser;
    parser.set_filename( data_dir + "/include1.conf" );
    parser.parse( blk );

    TEST( "inc 1 var 1", blk.get<std::string>( "inc1_var1"), "11" );
    TEST( "inc 1 var 2", blk.get<std::string>( "inc1_var2"), "12" );

    TEST( "inc 2 var 1", blk.get<std::string>( "inc2_var1"), "21" );
    TEST( "inc 2 var 2", blk.get<std::string>( "inc2_var2"), "22" );

    TEST( "Fixed path is unchanged", blk.get<std::string>( "fixed" ), "fixed_path" );
  } catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, false );
  }
}

void
test_update()
{
  std::cout << "Testing get_has_user_value() and update()\n";
  vidtk::config_block src, dst;
  try
  {
    src.add_parameter( "var1", "src_val", "test with default value" );
    bool has_user_value = true;
    src.get_has_user_value( "var1", has_user_value );
    TEST( " src has_user_value_ check", has_user_value, false );

    dst.add_parameter( "var1", "Dummy variable" );
    dst.set( "var1", "dst_val" );
    dst.get_has_user_value( "var1", has_user_value );
    TEST( " dst has_user_value_ check", has_user_value, true );

    dst.update( src );

    dst.get_has_user_value( "var1", has_user_value );
    TEST( " src has_user_value_ updated", has_user_value, false );
    TEST( " src defaul_value_ updated", dst.get<std::string>( "var1" ), "src_val" );
  }
  catch( const std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, false );
  }
}

void
test_enum()
{
  std::cout << "Testing enumerated types\n";

  vidtk::config_enum_option metadata_mode;
  metadata_mode.add( "stream", 1 )
               .add( "fixed",  2 )
               .add( "ortho",  3 );


  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "v", "dummy" );
    blk.set( "v", "fixed" );
    int temp;

    temp = blk.get< int >( metadata_mode, "v");
    TEST( "Getting value for enum", temp, 2 );

    temp = blk.get< int >( metadata_mode, "x"); // should throw
  }
  catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, true );
  }

  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "v", "dummy" );
    blk.set( "v", "badvalue" );
    int temp;

    temp = blk.get< int >( metadata_mode, "v");
    TEST( "Getting value for enum", temp, 2 );
  }
  catch( std::exception& ex )
  {
    std::cout << "Exception thrown. What = \""
             << ex.what() << "\"" << std::endl;
    TEST( "", true, true );
  }


}

} // end anonymous namespace

// ----------------------------------------------------------------
/** Main test routine.
 *
 *
 */
int test_config_block( int argc, char* argv[] )
{
  testlib_test_start( "config_block" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    std::string data_dir( argv[1] );

    test_subblock();
    test_bool();
    test_array();
    test_get_byte();
    test_string();
    test_parse_relative_filename( data_dir );
    test_include (data_dir);
    test_update();
    test_enum();
  }

  return testlib_test_summary();
}
