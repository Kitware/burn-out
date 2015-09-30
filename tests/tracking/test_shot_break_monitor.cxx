/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <tracking/shot_break_monitor.h>

#include <pipeline/async_pipeline.h>
#include <utilities/homography.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>



using namespace vidtk;

namespace { // anonymous


// ----------------------------------------------------------------
/** Process decorator.
 *
 * This class is the base class for process decorators. Derive a class
 * from this one and override desired methods, and Bob's your uncle.
 *
 * Use get_decorated() method to get pointer to the decorated class.
 *
 * Currently, the non-virtual methods of process:: are not supported
 * herein.
 */
class process_decorator
  : public process
{
public:
  process_decorator (vcl_string const& name, process * deco)
    : process(name, "process_decorator"),
    decorated_(deco)
  { }


  virtual ~process_decorator ()
  {
    delete decorated_;
  }

  virtual config_block params() const { return get_decorated()->params(); }
  virtual bool set_params( config_block const& blk ) { return get_decorated()->set_params(blk); }
  virtual bool set_param( vcl_string const& name, vcl_string const& value ) { return get_decorated()->set_param (name, value); }
  virtual bool initialize() { return get_decorated()->initialize(); }
  virtual bool reset() { return get_decorated()->reset(); }
  virtual bool cancel() { return get_decorated()->cancel(); }
  virtual bool step() { return get_decorated()->step(); }
  virtual process::step_status step2() { return get_decorated()->step2(); }

  // These are not virtual in the base class, so this is problematic
  // virtual vcl_string const& name() const { return get_decorated()->name(); }
  // virtual vcl_string const& class_name() const { return get_decorated()->class_name(); }
  // virtual void set_name( vcl_string const& ) { get_decorated()->set_name(); }


protected:
  process * get_decorated() { assert (decorated_); return decorated_; }
  process const* get_decorated() const { assert (decorated_); return decorated_; }

  // virtual bool push_output(step_status status) { return get_decorated()->push_output(status); }


private:

  vidtk::process * decorated_;

}; // end class process_decorator




// partial types
class TimestampFactory;
class DataFactory;
class input_process;
class output_process;
class TestingPipeline;
class TestBase;

typedef vxl_byte PixType;

#define STRINGIZE(A) #A
#define ElementsOf(A) (sizeof (A) / sizeof ((A)[0]))

// ----------------------------------------------------------------
struct ParameterElement
{
  const char * name;
  const char * value;
};


// ================================================================
class test_sbf : public shot_break_flags
{
public:
  test_sbf(bool end, bool us, bool np)
  {
    this->set_shot_end(end);
    this->set_frame_usable(us);
    this->set_frame_not_processed(np);
  }
};


class test_homog : public vidtk::image_to_image_homography
{
public:
  test_homog(bool v, bool nr)
  {
    vidtk::timestamp ts(1000, 1);
    this->set_identity(v);
    this->set_new_reference(nr);
    this->set_source_reference(ts);
    this->set_dest_reference(ts);
  }
};


// ----------------------------------------------------------------
/** Test data set - inputs and outputs.
 *
 *
 */
struct TestDataSet
{
  // inputs
  double gsd;
  vil_image_view< PixType > * image;
  test_sbf sbf;
  test_homog s2r_h;

  // outputs
  process::step_status expected_rc;
  bool expected_end_of_shot;
  bool expected_modality_change;
  video_modality expected_video_modality;
};



// ----------------------------------------------------------------
/** Create an image.
 *
 * Create a blank image.
 *
 * @param[in] ni - number of rows in image
 * @param[in] nj - number of columns in image
 */
vil_image_view<vxl_byte>
make_image( unsigned ni, unsigned nj )
{
  vil_image_view<vxl_byte> img( ni, nj, 3 );
  img.fill( 0 );

  return img;
}


// ----------------------------------------------------------------
/** Timestamp factory
 *
 * Create a series of timestamps that are in ascending order.
 */
class TimestampFactory
{
public:
  TimestampFactory(int start_frame = 1, double time_usec = 100)
    : m_frame(start_frame),
      m_timeUSec(time_usec),
      m_frameTime(0)
  {
    SetFrameRate(30);
  }

  /** Set frame rate in frames per sec.
   */
  void SetFrameRate (int rate)
  {
    // convert to micro sec (usec)
    m_frameTime = (1.0 / static_cast< double >(rate)) * 1e6;
  }

  /** Create a new timestamp in the series
   */
  vidtk::timestamp Create()
  {
    // increment to next frame values
    m_frame++;
    m_timeUSec += m_frameTime;

    return vidtk::timestamp(m_timeUSec, m_frame);
  }

private:
  unsigned int m_frame;
  double m_timeUSec;
  double m_frameTime;
}; // end of class TimestampFactory


// ----------------------------------------------------------------
/** Input process
 *
 *
 */
class input_process
  : public process
{
public:
  typedef input_process self_type;

  // test data
  TestDataSet * m_dataSet;
  int m_index;
  bool m_endSeen;

  TimestampFactory m_tsf;

  input_process(TestDataSet * data) // CTOR
    : process ("input_proc", "input proc"),
      m_dataSet(data),
      m_index(0),
      m_endSeen(false)
  {
  }


  virtual ~input_process() { }


  // -- process interface --
  virtual config_block params() const { return vidtk::config_block(); }
  virtual bool set_params( config_block const& ) { return true; }
  virtual bool initialize()
  {
    return true;
  }


  virtual vidtk::process::step_status step2()
  {
    if (m_endSeen)
    {
      return vidtk::process::FAILURE;
    }

    if (m_dataSet[m_index].gsd < 0)
    {
      vcl_cout << "*** end of input (input)\n";
      m_cv_timestamp = vidtk::timestamp(); // invalid timestamp for EOV
      m_endSeen = true;
    }
    else
    {
      m_cv_timestamp = m_tsf.Create();

      // copy inputs to current value areas
      m_cv_world_units_per_pixel = m_dataSet[m_index].gsd;
      m_cv_image = *m_dataSet[m_index].image;
      m_cv_shot_break_flags = m_dataSet[m_index].sbf;
      m_cv_src_to_ref_homography = m_dataSet[m_index].s2r_h;

      m_index++; // on to next element
    }

    return vidtk::process::SUCCESS;
  }


  // -- process outputs --
#define CONNECTION_DEF(TYPE, BASENAME)                                  \
  TYPE m_cv_ ## BASENAME;                                               \
  TYPE const& output_ ## BASENAME() const { return m_cv_ ## BASENAME;}  \
  VIDTK_OUTPUT_PORT( TYPE, output_ ## BASENAME );                       \
  VIDTK_ALLOW_SEMICOLON_AT_END( BASENAME );

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace



}; // end class input_process


// ----------------------------------------------------------------
/** Output process
 *
 *
 */
class output_process
  : public process
{
public:
  typedef output_process self_type;

// #define TSTR(X) vcl_string( m_testName + ": " +  X ).c_str()

  // test data
  TestDataSet * m_dataSet;
  int m_index;

  output_process(TestDataSet * data) // CTOR
    : process ("out proc", "out_proc"),
      m_dataSet(data),
      m_index(0)
  {  }

  // -- process interface --
  virtual config_block params() const { return vidtk::config_block(); }
  virtual bool set_params( config_block const& ) { return true; }
  virtual bool initialize() { return true; }

  virtual vidtk::process::step_status step2()
  {
    // Test for end of data set
    if (m_dataSet[m_index].gsd < 0)
    {
      TEST ("unexpected end of input", true, false);
    }

    // Test sbm process outputs
    TEST ("sbm output gsd", world_units_per_pixel_data, m_dataSet[m_index].gsd );
    TEST ("sbm output sbf", shot_break_flags_data, m_dataSet[m_index].sbf );
    TEST ("sbm output s2r_h", src_to_ref_homography_data, m_dataSet[m_index].s2r_h );
    TEST ("sbm output video mode", video_modality_data, m_dataSet[m_index].expected_video_modality );

    m_index++;

    return vidtk::process::SUCCESS;
  }


  // -- process inputs --
#define CONNECTION_DEF(TYPE, BASENAME)                                  \
TYPE BASENAME ## _data;                                                 \
void input_ ## BASENAME(TYPE const& val) { BASENAME ## _data = val; }   \
VIDTK_INPUT_PORT( input_ ## BASENAME, TYPE );                           \
VIDTK_ALLOW_SEMICOLON_AT_END( BASENAME );

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace

  // Additional value supplied by process under test
  video_modality video_modality_data;
  void input_video_modality( video_modality val) { video_modality_data = val; }
  VIDTK_INPUT_PORT( input_video_modality, video_modality );

}; // end class output_process


// ----------------------------------------------------------------
/** shot break monitor decorator.
 *
 *
 */
class sbm_deco
  : public process_decorator
{
public:
  typedef sbm_deco self_type;

  sbm_deco(vidtk::shot_break_monitor < PixType > * proc, TestDataSet * data)
    : process_decorator("shot_break_mon", proc),
      m_dataSet(data),
      m_index(0),
      m_sbm(proc)
  { }

  virtual ~sbm_deco() { }


  virtual process::step_status step2()
  {
    // call decorated
    process::step_status status = get_decorated()->step2();

    TEST ("sbm return code", status, m_dataSet[m_index].expected_rc );

    // If SBM returned SUCCESS, then the outputs are valid. If anything else is returned,
    // then we must reset SBM and we will get the outputs on a later cycle.
    if (status == vidtk::process::SUCCESS)
    {
      TEST ("end of shot", m_sbm->is_end_of_shot(), m_dataSet[m_index].expected_end_of_shot);
      TEST ("modality change", m_sbm->is_modality_change(), m_dataSet[m_index].expected_modality_change);
      TEST ("modality", m_sbm->current_modality(),  m_dataSet[m_index].expected_video_modality);
    }
    else
    {
      // call reset to emulate pipeline environment
      get_decorated()->reset();
    }

    // Force return code based on end of test. We can only return
    // FAILURE at end of test since the testing pipeline does not
    // have any fail/recovery handling.

    // is this needed since the upstream process will fail at EOV?
    if (m_dataSet[m_index].gsd < 0)
    {
      vcl_cout << "*** end of input (deco)\n";
      return vidtk::process::FAILURE;
    }

    m_index++;

    return vidtk::process::SUCCESS;
  } // end step2()


  // input/output ports
  // forward calls to decorated process
#define CONNECTION_DEF(TYPE, BASENAME)                                  \
  void set_input_ ## BASENAME( TYPE const& v) { m_sbm->set_input_ ## BASENAME(v); } \
  VIDTK_INPUT_PORT( set_input_ ## BASENAME, TYPE const& );              \
  TYPE get_output_ ## BASENAME() const { return m_sbm->get_output_ ## BASENAME(); } \
  VIDTK_OUTPUT_PORT( TYPE, get_output_ ## BASENAME );                   \
  VIDTK_ALLOW_SEMICOLON_AT_END( BASENAME );

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace

  video_modality get_output_video_modality() const { return m_sbm->get_output_video_modality(); }
  VIDTK_OUTPUT_PORT( video_modality, get_output_video_modality );

private:
  // test data
  TestDataSet * m_dataSet;
  int m_index;

  // need subclass type for this pointer
  shot_break_monitor < PixType > * m_sbm;

}; // end class sbm_deco


// ----------------------------------------------------------------
/** Pipeline used for testing.
 *
 *
 */
class TestingPipeline
{
public:
  vidtk::async_pipeline m_pipeline;
  process_smart_pointer < input_process > m_in_proc;
  process_smart_pointer < sbm_deco > m_test_proc;
  process_smart_pointer < output_process > m_out_proc;
  vidtk::config_block m_config;


  TestingPipeline(TestDataSet * data)
    : m_in_proc(0),
      m_test_proc(0),
      m_out_proc(0)
  {
    // create processes with test data as parameter
    m_in_proc = new input_process(data);
    vidtk::shot_break_monitor < vxl_byte> * sbm  = new shot_break_monitor < vxl_byte > ("shot_break_mon");
    m_test_proc = new sbm_deco( sbm, data);
    m_out_proc = new output_process(data);

    // Add processes to pipeline
    m_pipeline.add (m_in_proc);
    m_pipeline.add (m_test_proc);
    m_pipeline.add (m_out_proc);

    // connect inputs
#define CONNECTION_DEF(TYPE, BASENAME)                                  \
    m_pipeline.connect ( m_in_proc->output_ ## BASENAME ## _port(),   m_test_proc->set_input_## BASENAME ## _port() ); \
    m_pipeline.connect ( m_test_proc->get_output_ ## BASENAME ##_port(),  m_out_proc->input_ ## BASENAME ## _port() );

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace

    // need to connect extra output
    m_pipeline.connect ( m_test_proc->get_output_video_modality_port(), m_out_proc->input_video_modality_port() );
  }


  bool SetParams (ParameterElement * parms)
  {
    m_config.add_subblock( m_pipeline.params(), "test" );
    // m_config.print(vcl_cout);
    while (parms->name != 0)
    {
      vcl_string name("test:shot_break_mon:");
      name += parms->name;
      m_config.set_value( name, parms->value);

      parms++; // go to next set of parameters
    } // end while

    return m_pipeline.set_params( m_config.subblock("test") );
}

  bool Initialize () { return m_pipeline.initialize(); }
  bool Run() { return m_pipeline.run(); }

}; // end class TestingPipeline


// ----------------------------------------------------------------
/** Base class for all tests.
 *
 * Call Setup to configure the test.
 * Call Run() to run the test.
 */
class TestBase
{
public:
#define TSTR(X) vcl_string( m_testName + ": " +  X ).c_str()

  TestBase(const char * name, ParameterElement * params, TestDataSet * data)
    : m_parms(params),
      m_testName(name),
      m_dataFactory(data),
      m_testPipeline(0)
  {
    m_testPipeline = new TestingPipeline(m_dataFactory);
  }


  virtual ~TestBase()
  {
    delete m_testPipeline;
  }


  bool Setup()
  {

    TEST( TSTR("set params"), m_testPipeline->SetParams( m_parms ), true );
    TEST( TSTR("pipeline init"), m_testPipeline->Initialize(), true );
    return true;
  }


  bool Run()
  {
    bool run_status = m_testPipeline->Run();
    TEST( TSTR("pipeline run"), run_status, true );
    PostTest();

    return true;
  }


  virtual void PostTest() { }


  // accessors for inputs and outputs
#define CONNECTION_DEF(TYPE, BASENAME)                                                  \
  TYPE const& input_ ## BASENAME () { return m_testPipeline->m_in_proc->output_ ## BASENAME(); } \
  TYPE const& output_ ## BASENAME () { return m_testPipeline->m_out_proc->BASENAME ## _data; }

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace


  void PrintFinal()
  {
    vcl_cout
#define CONNECTION_DEF(TYPE, BASENAME) << STRINGIZE(BASENAME) ": " << output_ ## BASENAME() << "\n"

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace
      ;
  }


protected:
  ParameterElement * m_parms;
  vcl_string m_testName;

  TestDataSet * m_dataFactory;
  TestingPipeline * m_testPipeline;

#undef TSTR

}; // end class TestBase



// test images
vil_image_view< PixType > image_EO;
vil_image_view< PixType > image_IR;


// ================================================================
// Testing for correct operation when disabled
// sbf_shot_end -> output shot end (no fail)
TestDataSet test1_data[] = {
  // baseline start
  // gsd,  image,    shot break flags(end,us,np), s2r homography(val,nr),               proc ret code,   ex eos, ex mode chg, new modality
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(true, false), /* outputs */ process::SUCCESS, false, false, VIDTK_INVALID },

  // set shot end -> expected shot end, SUCCESS
  { 0.05, &image_EO, test_sbf(true,  true, false), test_homog(true, false), /* outputs */ process::SUCCESS, false,  false, VIDTK_INVALID },

  // generic input
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(true, false), /* outputs */ process::SUCCESS, false, false, VIDTK_INVALID },

  // new ref ->
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(true, true), /* outputs */ process::SUCCESS, false, false, VIDTK_INVALID },

  // generic input
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(true, false), /* outputs */ process::SUCCESS, false, false, VIDTK_INVALID },


  { -1, /* END marker */ 0, test_sbf(false, true, false), test_homog(true, false), process::FAILURE, false, false, VIDTK_INVALID }
};

ParameterElement params_1[] =
{
  { "disabled",                   "true"  },
  { "fov_change_threshold",       "0.125" },
  { "eo_ir_detect:eo_multiplier", "4"     },
  { "eo_ir_detect:rg_diff",       "8"     },
  { "eo_ir_detect:rb_diff",       "8"     },
  { "eo_ir_detect:gb_diff",       "8"     },
  {                            0,       0 } // end marker
};

void test1()
{
  TestBase test("test disabled", params_1, test1_data);

  test.Setup();
  test.Run();
}


// ================================================================
// Test for correct operation when there is no valid homography

TestDataSet test2_data[] = {
  // baseline start
  // gsd,  image,    shot break flags(end, us,np), s2r homography(val, nr),               proc ret code,   ex eos, ex mode chg, new modality
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(false, false), /* outputs */ process::FAILURE, false, true, VIDTK_EO_N },
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(false, false), /* outputs */ process::SUCCESS, false, false, VIDTK_EO_N },

  // set shot end -> expected shot end, SUCCESS
  { 0.05, &image_EO, test_sbf(true,  true, false), test_homog(false, false), /* outputs */ process::SUCCESS, false,  false, VIDTK_EO_N },

  // use IR image, no new ref -> new modality
  { 0.05, &image_IR, test_sbf(false, true, false), test_homog(false, false), /* outputs */ process::SUCCESS, false, false, VIDTK_EO_N },

  // use IR image, new ref -> no new modality
  { 0.05, &image_IR, test_sbf(false, true, false), test_homog(false, true), /* outputs */ process::SUCCESS, false, false, VIDTK_EO_N },

  // use EO image, no new ref, M gsd -> new modality
  { 0.2, &image_EO, test_sbf(false, true, false), test_homog(false, false), /* outputs */ process::SUCCESS, false, false, VIDTK_EO_N },

  // use EO image, new ref, M gsd ->  no new modality
  { 0.2, &image_EO, test_sbf(false, true, false), test_homog(false, true), /* outputs */ process::SUCCESS, false, false, VIDTK_EO_N },

  // shot end -> SUCCESS
  { 0.05, &image_EO, test_sbf(true, true, false), test_homog(false, false), /* outputs */ process::SUCCESS, false, false, VIDTK_EO_N },

  { -1, /* END marker */ 0, test_sbf(false, true, false), test_homog(true, false), process::FAILURE, false, false, VIDTK_INVALID }
};

ParameterElement params_2[] =
{
  { "disabled",                   "false" },
  { "first_frame_only",           "true"  },
  { "fov_change_threshold",       "0.175" },
  { "eo_ir_detect:eo_multiplier", "4"     },
  { "eo_ir_detect:rg_diff",       "8"     },
  { "eo_ir_detect:rb_diff",       "8"     },
  { "eo_ir_detect:gb_diff",       "8"     },
  {                            0,       0 } // end marker
};

void test2()
{
  TestBase test ("Test first_frame_only", params_2, test2_data);

  test.Setup();
  test.Run();
}


// ================================================================
// Test for correct operation when there is valid homography

TestDataSet test3_data[] = {
  // baseline start
  // gsd,  image,    shot break flags(end, us,np), s2r homography(val, nr),               proc ret code,   ex eos, ex mode chg, new modality
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(true, false), /* outputs */ process::SUCCESS, false, false, VIDTK_INVALID },

  // set new ref -> new video mode
  { 0.05, &image_EO, test_sbf(false, true, false), test_homog(true, true), /* outputs */ process::FAILURE, false, true, VIDTK_EO_N },

  // set shot end -> expected shot end, FAILURE
  { 0.05, &image_EO, test_sbf(true,  true, false), test_homog(true, false), /* outputs */ process::FAILURE, true,  false, VIDTK_EO_N },

  // use IR image, no new ref -> no new modality
  { 0.05, &image_IR, test_sbf(false, true, false), test_homog(true, false), /* outputs */ process::SUCCESS, false, false, VIDTK_EO_N },

  // use IR image, new ref -> new modality
  { 0.05, &image_IR, test_sbf(false, true, false), test_homog(true, true), /* outputs */ process::FAILURE, false, false, VIDTK_IR_N },

  // use EO image, no new ref, M gsd -> no new modality
  { 0.2, &image_EO, test_sbf(false, true, false), test_homog(true, false), /* outputs */ process::SUCCESS, false, false, VIDTK_IR_N },

  // use EO image, new ref, M gsd ->  new modality
  { 0.2, &image_EO, test_sbf(false, true, false), test_homog(true, true), /* outputs */ process::FAILURE, false, true, VIDTK_EO_M },

  // shot end -> FAILURE
  { 0.05, &image_EO, test_sbf(true, true, false), test_homog(true, false), /* outputs */ process::FAILURE, true, false, VIDTK_EO_M },

  { -1, /* END marker */ 0, test_sbf(false, true, false), test_homog(true, false), process::FAILURE, false, false, VIDTK_INVALID }
};

ParameterElement params_3[] =
{
  { "disabled",                   "false" },
  { "first_frame_only",           "false" },
  { "fov_change_threshold",       "0.175" },
  { "eo_ir_detect:eo_multiplier", "4"     },
  { "eo_ir_detect:rg_diff",       "8"     },
  { "eo_ir_detect:rb_diff",       "8"     },
  { "eo_ir_detect:gb_diff",       "8"     },
  {                            0,       0 } // end marker
};

void test3()
{
  TestBase test ("Test streaming mode", params_3, test3_data);

  test.Setup();
  test.Run();
}


} // end namespace

// ----------------------------------------------------------------
/** Main test driver
 *
 *
 */
int test_shot_break_monitor( int argc, char * argv[] )
{
  vcl_string data_dir;

  testlib_test_start( "shot break monitor process" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    data_dir = argv[1];
    data_dir += "/";

    // initialize the two test images (EO, IR)
    image_EO = vil_load((data_dir + "aph_imgs00001.png").c_str());

    vil_image_view< PixType > image_temp;
    image_temp = vil_load((data_dir + "ocean_city.png").c_str());

    // Convert to 3 plane image
    image_IR.set_size( image_temp.ni(), image_temp.nj(), 3 );
    for( unsigned int i = 0; i < image_temp.ni(); i++)
    {
      for(unsigned int j = 0; j < image_temp.nj(); j++)
      {
        PixType pix = image_temp(i, j);
        image_IR(i, j, 0) = pix;
        image_IR(i, j, 1) = pix;
        image_IR(i, j, 2) = pix;
      }
    }

    test1();
    test2();
    test3();
  }

  return testlib_test_summary();

}
