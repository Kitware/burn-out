/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <string>

#include <video_io/frame_process_sp_facade_interface.h>
#include <video_io/frame_process_sp_facade.h>
#include <pipeline_framework/super_process.h>

#include <testlib/testlib_test.h>

//#define TEST_COMPILE_FAILURES

namespace
{

using namespace vidtk;


// Defines a standard set of methods that all test classes will define.
#define TEST_CLASS_BODY( CLS_NAME )                                            \
public:                                                                        \
                                                                               \
  CLS_NAME( std::string const& _name )                                         \
    SP_INHERIT( CLS_NAME )                                                     \
  {                                                                            \
    (void) _name;                                                              \
    metadata_ = new video_metadata();                                          \
    image_ = new vil_image_view<bool>();                                       \
  }                                                                            \
                                                                               \
  ~CLS_NAME()                                                                  \
  {                                                                            \
    delete metadata_;                                                          \
    delete image_;                                                             \
  }                                                                            \
                                                                               \
  config_block params() const                { return config_block(); }        \
  bool set_params( config_block const& /*blk*/ ) { return true; }              \
  bool initialize()                          { return true; }                  \
  process::step_status step2()               { return process::FAILURE; }      \
  bool seek( unsigned /*frame_num*/ )            { return false; }             \
  vidtk::timestamp timestamp() const         { return vidtk::timestamp(); }    \
  video_metadata metadata() const     { return *metadata_; }                   \
  vil_image_view<bool> image() const    { return *image_; }                    \
  unsigned ni() const                        { return 0; }                     \
  unsigned nj() const                        { return 0; }                     \
  unsigned nframes() const                   { return 0; }                     \
  double frame_rate() const                  { return 0.0; }                   \
                                                                               \
private:                                                                       \
  video_metadata *metadata_;                                                   \
  vil_image_view<bool> *image_;


#define SP_INHERIT( CLS_NAME )                                                 \
    : super_process( _name, #CLS_NAME )

class inherit_both
  : public super_process,
    public frame_process_sp_facade_interface<bool>
{
  TEST_CLASS_BODY( inherit_both )
};


class inherit_sp
  : public super_process
{
  TEST_CLASS_BODY( inherit_sp )
};

#undef SP_INHERIT
#define SP_INHERIT( N )


class inherit_fpspi
  : public frame_process_sp_facade_interface<bool>
{
  TEST_CLASS_BODY( inherit_fpspi )
};


} // end anonymous namespace


int test_frame_process_sp_facade( int /*argc*/, char* /*argv*/[] )
{
  // test that instantiation cast checks correctly pass/fail where expected
  // fail cases cannot be compiled, as is the intentions.

  testlib_test_start("frame_process_sp_facade");

  frame_process_sp_facade<bool, inherit_both> check_1( "test_1" );
  TEST( "Test construction of proper object", true, true );

  // These instantiations will cause compile "cannot convert ..." errors
#ifdef TEST_COMPILE_FAILURES
  frame_process_sp_facade<bool, inherit_sp> check_2( "test_2" );
  frame_process_sp_facade<bool, inherit_fpspi> check_3( "test_3" );
#endif

  return testlib_test_summary();
}
