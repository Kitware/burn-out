/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <resource_pool/resource_user.h>

#include <testlib/testlib_test.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <pipeline_framework/sync_pipeline.h>
#include <utilities/config_block.h>

#include <vcl_iostream.h>

#define DEBUG 0

using namespace vidtk;

namespace  {  // anonymous

vidtk::resource_trait< int > test_int_res( "test_int_value" );
vidtk::resource_trait< std::string > test_string_res( "test_string_value" );
vidtk::resource_trait< int > test_int2_res( "test_int2_value" );


// ----------------------------------------------------------------
// Resource consumer
class process_one :
  public vidtk::process,
  public vidtk::resource_user
{
public:
  typedef process_one self_type;

  process_one( std::string const& n )
    : vidtk::process( n, "process_one" )
  {
    // declare that this resource is being used.
    use_resource( test_int_res );
    use_resource( test_int_res );

    TEST( "Create resource 2", create_resource( test_int2_res, 11 ), true );
    TEST( "Create resource 2 again", create_resource( test_int2_res, 11 ), false );
  }


  virtual ~process_one() { }


  // -- process interface --
  virtual config_block params() const
  {
    return this->config_;
  }


  virtual bool set_params( config_block const& )
  {
    return true;
  }


// ----------------------------------------------------------------
  virtual bool initialize()
  {
    int val = 0;

    TEST( "Resource is available", resource_available( test_int_res ), true );
    TEST( "Resource 2 is available", resource_available( test_int2_res ), true );
    TEST( "Getting resource", get_resource( test_int_res, val ), true );
    TEST( "Resource value as expected", val, 5 );

    val = -1;
    try
    {
      val = get_resource( test_int_res );
    }
    catch ( ns_resource_pool::resource_pool_exception const& e )
    {
      TEST( "Getting resource assignment", true, false );
    }

    TEST( "Resource value as expected again", val, 5 );

    TEST( "Resource is not available", resource_available( test_string_res ), false );

    bool except( false );
    try
    {
      get_resource( test_string_res );
    }
    catch ( ns_resource_pool::resource_pool_exception const& e )
    {
      std::cout << "Exception caught: " << e.what() << std::endl;
      except = true;
    }

    TEST( "Getting missing resource assignment (except)", except, true );

    except = false;
    try
    {
      set_resource<std::string>( test_string_res, "test" );
    }
    catch ( ns_resource_pool::resource_pool_exception const& e )
    {
      std::cout << "Exception caught: " << e.what() << std::endl;
      except = true;
    }

    TEST( "Setting missing resource assignment (except)", except, true );

    return true;
  }


// ----------------------------------------------------------------
  virtual bool step()
  {
    // Terminate if count gets to zero
    int val = get_resource( test_int_res );

    if (2 == val)
    {
      // cause notify() call
      set_resource( test_int2_res, 3 );
    }

    if (val <= 0)
    {
      notify_observers( test_int2_res );
      return false;
    }

    return true;
  }


// ----------------------------------------------------------------
  // -- process outputs --
  VIDTK_OUTPUT_PORT( int, output_int );
  int output_int() const
  {
    return 1;
  }


private:
  config_block config_;

};


// ================================================================
// Resource producer
class process_two :
  public vidtk::process,
  public vidtk::resource_user
{
public:
  typedef process_two self_type;

  process_two( std::string const& n )
    : vidtk::process( n, "process_two" )
  {

    // publish resource
    TEST( "Create resource", create_resource( test_int_res, 5 ), true );
    use_resource( test_int2_res );
    use_resource( test_int_res );
  }


  virtual ~process_two() { }


  // -- process interface --
  virtual config_block params() const
  {
    return this->config_;
  }


  virtual bool set_params( config_block const& )
  {
    return true;
  }


// ----------------------------------------------------------------
  virtual bool initialize()
  {
    // display all resources
    std::vector< std::string > list;

    list = get_owned_list();
    TEST("Number of owned resources", list.size(), 1 );

#if DEBUG
    std::cout << "Process TWO Owned resources: ";
    for ( std::vector< std::string >::const_iterator i = list.begin(); i != list.end(); ++i )
    {
      std::cout << *i << ' ';
    }
    std::cout << std::endl;
#endif

    list = get_used_list();
    TEST("Number of used resources", list.size(), 2 );

#if DEBUG
    std::cout << "Process TWO Used resources: ";
    for ( std::vector< std::string >::const_iterator i = list.begin(); i != list.end(); ++i )
    {
      std::cout << *i << ' ';
    }
    std::cout << std::endl;
#endif

    // Register observer
    TEST("Register observer",
         observe_resource( test_int2_res, boost::bind( &process_two::notify, this, _1  ) ),
         true );

    return true;
  }


// ----------------------------------------------------------------
  void notify( std::string const& res_name)
  {
    TEST("Resource name supplied to notify", res_name, "test_int2_value" );

    // std::cout << "Notified of resource change: " << res_name << std::endl;

    int val = 0;
    get_resource( test_int2_res, val );
    TEST("Resource value after notify", val, 3 );
  }


// ----------------------------------------------------------------
  virtual bool step()
  {
    // Decrement counter
    int val = get_resource( test_int_res );
    set_resource( test_int_res, --val );

    return true;
  }


// ----------------------------------------------------------------
  // -- process outputs --
  VIDTK_INPUT_PORT( input_int, int );
  void input_int( int  )
  {
  }


private:
  config_block config_;
};


} // end namespace anonymous


// ================================================================
int test_resource_user( int /* argc */, char* /* argv */[] )
{
  testlib_test_start( "Resource pool tests" );

  process_smart_pointer< process_one > p1( new process_one( "one" ) );
  process_smart_pointer< process_two > p2( new process_two( "two" ) );
  sync_pipeline pipe;

  pipe.add( p1 );
  pipe.add( p2 );

  pipe.connect( p1->output_int_port(), p2->input_int_port() );

  vidtk::config_block blk = pipe.params();
  pipe.set_params( blk );
  pipe.initialize();
  pipe.run();

  return testlib_test_summary();
}
