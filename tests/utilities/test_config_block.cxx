/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_exception.h>
#include <vcl_fstream.h>
#include <vxl_config.h>
#include <testlib/testlib_test.h>
#include <vul/vul_file.h>

#include <utilities/config_block.h>
#include <utilities/config_block_parser.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


void
test_array()
{
  vcl_cout << "Test array\n";

  double vals[4] = { 0.0, 1.1, 2.2, 3.3 };

  vidtk::config_block blk;
  blk.add( "v" );

  TEST( "Set value (4 doubles)", blk.set( "v", "0.1 1.2 2.3 3.4" ), true );

  TEST( "Get double[4] value", blk.get( "v", vals ), true );
  TEST( "Values are correct", true,
        vals[0]==0.1 &&
        vals[1]==1.2 &&
        vals[2]==2.3 &&
        vals[3]==3.4 );

  double vals5[5] = { 1.0, 2.0 };
  TEST( "Get double[5] value fails", blk.get( "v", vals ), true );
  TEST( "Original values are untouched", true,
        vals5[0]==1.0 &&
        vals5[1]==2.0 );

  double vals3[3] = { 1.0, 2.0 };
  TEST( "Get double[3] value fails", blk.get( "v", vals ), true );
  TEST( "Original values are untouched", true,
        vals3[0]==1.0 &&
        vals3[1]==2.0 );
}

void
test_get_byte()
{
  vcl_cout << "\n\n\nTest get byte\n\n";

  vidtk::config_block blk;
  blk.add_parameter( "v", "Test value" );

  {
    blk.set( "v", "34" );
    vxl_byte v;
    TEST( "Get as a byte succeeds", blk.get("v",v), true );
    TEST( "Value is correct", v, 34 );
    TEST( "Unconditional get", blk.get<vxl_byte>("v"), 34 );
  }

  {
    blk.set( "v", "34" );
    unsigned char v;
    TEST( "Get as a unsigned char succeeds", blk.get("v",v), true );
    TEST( "Value is correct", v, 34 );
    TEST( "Unconditional get", blk.get<unsigned char>("v"), 34 );
  }

  {
    blk.set( "v", "-34" );
    signed char v;
    TEST( "Get as a signed char succeeds", blk.get("v",v), true );
    TEST( "Value is correct", v, -34 );
    TEST( "Unconditional get", blk.get<signed char>("v"), -34 );
  }

  {
    blk.set( "v", "3" );
    char v;
    TEST( "Get as a char succeeds", blk.get("v",v), true );
    TEST( "Value is correct", v, '3' );
    TEST( "Unconditional get", blk.get<char>("v"), '3' );
  }
}


void
test_parse_relative_filename( vcl_string const& data_dir )
{
  vcl_cout << "Testing relative file parsing with a filename\n";
  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "fixed", "a fixed path" );
    blk.add_parameter( "rel", "a relative path" );
    blk.add_parameter( "rel2", "a relative path" );

    blk.parse( data_dir+"/relative_filename.conf" );
    TEST( "Fixed path is unchanged", blk.get<vcl_string>( "fixed" ), "fixed_path" );
    vcl_cout << "Relative path: \"" << blk.get<vcl_string>( "rel" ) << "\"\n";
    TEST( "Relative path has been corrected",
          blk.get<vcl_string>( "rel" ),
          data_dir+"/relative_filename.conf" );

    TEST( "1st relative file exists",
          vul_file::exists( blk.get<vcl_string>( "rel" ) ), true );

    TEST( "1st relative path is same as 2nd relative path",
          blk.get<vcl_string>( "rel" ), blk.get<vcl_string>( "rel2" ) );
  }
  catch( vcl_exception& ex )
  {
    vcl_cout << "Exception thrown. What = \""
             << ex.what() << "\"" << vcl_endl;
    TEST( "", true, false );
  }

  vcl_cout << "Testing relative file parsing with istream\n";
  try
  {
    vidtk::config_block blk;
    blk.add_parameter( "fixed", "a fixed path" );
    blk.add_parameter( "rel", "a relative path" );
    blk.add_parameter( "rel2", "a relative path" );

    vcl_ifstream fstr( (data_dir+"/relative_filename.conf").c_str() );
    TEST( "Stream opened", fstr.good(), true );
    vidtk::config_block_parser parser;
    parser.set_input_stream( fstr );
    parser.parse( blk );
    TEST( "Fixed path is unchanged", blk.get<vcl_string>( "fixed" ), "fixed_path" );
    vcl_cout << "Relative path: \"" << blk.get<vcl_string>( "rel" ) << "\"\n";
    TEST( "1st relative path has been corrected",
          blk.get<vcl_string>( "rel" ),
          vul_file::get_cwd()+"/relative_filename.conf" );

    //Can't test for existence, because it depends on the directory in
    //which this executable is run.
    //
    //TEST( "Relative file exists",
    //      vul_file::exists( blk.get<vcl_string>( "rel" ) ), true );

    TEST( "1st relative path is same as 2nd relative path",
          blk.get<vcl_string>( "rel" ), blk.get<vcl_string>( "rel2" ) );
  }
  catch( vcl_exception& ex )
  {
    vcl_cout << "Exception thrown. What = \""
             << ex.what() << "\"" << vcl_endl;
    TEST( "", true, false );
  }
}


void
test_include( vcl_string const& data_dir )
{
  vcl_cout << "Testing includes\n";
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

    TEST( "inc 1 var 1", blk.get<vcl_string>( "inc1_var1"), "11" );
    TEST( "inc 1 var 2", blk.get<vcl_string>( "inc1_var2"), "12" );

    TEST( "inc 2 var 1", blk.get<vcl_string>( "inc2_var1"), "21" );
    TEST( "inc 2 var 2", blk.get<vcl_string>( "inc2_var2"), "22" );

    TEST( "Fixed path is unchanged", blk.get<vcl_string>( "fixed" ), "fixed_path" );
  } catch( vcl_exception& ex )
  {
    vcl_cout << "Exception thrown. What = \""
             << ex.what() << "\"" << vcl_endl;
    TEST( "", true, false );
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
    vcl_string data_dir( argv[1] );

    test_array();
    test_get_byte();
    test_parse_relative_filename( data_dir );
    test_include (data_dir);
  }

  return testlib_test_summary();
}
