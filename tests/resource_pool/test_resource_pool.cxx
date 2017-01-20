/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <resource_pool/resource_pool.h>

#include <testlib/testlib_test.h>

using namespace vidtk;

namespace  {  // anonymous
  resource_pool* the_pool;

  int obs1_called(0);
  int obs2_called(0);

  void obs1( std::string const& name )
  {
    ++obs1_called;

    int val;
    TEST("Observer 1 getting resource", the_pool->get_resource< int > ( name, val ), true );
  }

  void obs2( std::string const& name )
  {
    ++obs2_called;

    int val;
    TEST("Observer 2 getting resource", the_pool->get_resource< int > ( name, val ), true );
  }


} // end anonymous


// ================================================================
int
test_resource_pool( int /* argc */, char* /* argv */[] )
{
  testlib_test_start( "Resource pool tests" );

  the_pool = resource_pool::instance();
  int val = 0;

  TEST( "Missing resource", the_pool->has_resource( "foo" ), false );
  TEST( "Set missing resource", the_pool->set_resource< int > ( "foo", 4 ), false );
  TEST( "Get missing resource", the_pool->get_resource< int > ( "foo", val ), false );

  bool except( false );
  try
  {
    the_pool->get_resource< int > ( "foo" );
  }
  catch ( ns_resource_pool::resource_pool_exception const& e )
  {
    //  std::cout << "Exception caught: " << e.what() << std::endl;
    except = true;
  }

  TEST( "Getting missing resource assignment (except)", except, true );



  TEST( "Create resource", the_pool->create_resource( "foo", 4 ), true );
  TEST( "Get resource", the_pool->get_resource< int > ( "foo", val ), true );
  TEST( "Check resource value", val, 4 );
  TEST( "Set resource value", the_pool->set_resource< int > ( "foo", 8 ), true );

  val = 0;
  TEST( "Get resource again", the_pool->get_resource< int > ( "foo", val ), true );
  TEST( "Check resource value again", val, 8 );

  TEST( "Resource available", the_pool->has_resource( "foo" ), true );

  TEST( "Delete resource", the_pool->delete_resource( "foo" ), true );
  TEST( "Delete resource again", the_pool->delete_resource( "foo" ), false );

  TEST( "Resource not available", the_pool->has_resource( "foo" ), false );
  TEST( "Get deleted resource", the_pool->get_resource< int > ( "foo", val ), false );

  {
    std::vector< std::string > list = the_pool->get_resource_list();
    TEST( "Resource list size", list.size(), 0 );
  }

  TEST( "Create another resource", the_pool->create_resource( "bar", 4 ), true );

  except = false;
  try
  {
    the_pool->observe( "foo", boost::bind( &obs1, _1 ) );
  }
  catch ( ns_resource_pool::resource_pool_exception const& e )
  {
    //  std::cout << "Exception caught: " << e.what() << std::endl;
    except = true;
  }
  TEST( "Add observer of missing resource (except)", except, true );

  except = false;
  try
  {
    the_pool->observe( "bar", boost::bind( &obs1, _1 ) );
  }
  catch ( ns_resource_pool::resource_pool_exception const& e )
  {
    //  std::cout << "Exception caught: " << e.what() << std::endl;
    except = true;
  }
  TEST( "Add observer 1 for resource", except, false );

  except = false;
  try
  {
    the_pool->observe( "bar", boost::bind( &obs2, _1 ) );
  }
  catch ( ns_resource_pool::resource_pool_exception const& e )
  {
    //  std::cout << "Exception caught: " << e.what() << std::endl;
    except = true;
  }
  TEST( "Add observer 2 for resource", except, false );

  obs1_called = 0;
  obs2_called = 0;
  TEST( "Set resource value", the_pool->set_resource< int > ( "bar", 8 ), true );
  TEST( "Observer 1 called", obs1_called, 1 );
  TEST( "Observer 2 called", obs2_called, 1 );

  TEST( "Create third resource", the_pool->create_resource( "baz", 44 ), true );
  except = false;
  try
  {
    the_pool->observe( "baz", boost::bind( &obs2, _1 ) );
  }
  catch ( ns_resource_pool::resource_pool_exception const& e )
  {
    //  std::cout << "Exception caught: " << e.what() << std::endl;
    except = true;
  }
  TEST( "Add observer 2 for new resource", except, false );

  obs1_called = 0;
  obs2_called = 0;
  TEST( "Set resource value", the_pool->set_resource< int > ( "baz", 8 ), true );
  TEST( "Observer 1 called", obs1_called, 0 );
  TEST( "Observer 2 called", obs2_called, 1 );

  //notify observers
  obs1_called = 0;
  obs2_called = 0;
  the_pool->notify_observers( "bar" );
  TEST( "Observer 1 called", obs1_called, 1 );
  TEST( "Observer 2 called", obs2_called, 1 );

  {
    std::vector< std::string > list = the_pool->get_resource_list();
    TEST( "Resource list size", list.size(), 2 );
  }

  return testlib_test_summary();
} // test_resource_pool
