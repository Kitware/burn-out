/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <tracking/world_coord_super_process.h>
#include <pipeline/async_pipeline.h>
#include <utilities/homography.h>
#include <vil/vil_image_view.h>

#include <vcl_vector.h>
#include <vcl_fstream.h>


#define ElementsOf(A) (sizeof (A) / sizeof ((A)[0]))

using namespace vidtk;

namespace { // anonymous

// partial types
class OutputStepDelegate;
class TimestampFactory;
class VideoMetadataFactory;
class input_process;
class output_process;
class TestingPipeline;
class TestBase;

vcl_string G_test_directory;


// ----------------------------------------------------------------
/** Metadata table entry.
 *
 *
 */
struct MetadataEntry
{
  // These fields are initialized based on generated data
  int frame_number;
  vxl_uint_64 frame_time;
  vxl_uint_64 utc_time;

  double plat_lat, plat_lon,

    plat_alt, plat_roll, plat_pitch, plat_yaw,

    sensor_roll, sensor_pitch, sensor_yaw,

    ul_lat, ul_lon,

    ur_lat, ur_lon,

    lr_lat, lr_lon,

    ll_lat, ll_lon,

    slant_range, sensor_h_fov, sensor_v_fov,

    frame_center_lat, frame_center_lon;

  bool end_of_shot;


  // -- CONSTRUCTOR --
  MetadataEntry() { }
  MetadataEntry( vcl_fstream *  file_stream)
  {
    *file_stream >> frame_number
                 >> frame_time
                 >> utc_time
                 >> plat_lat >> plat_lon
                 >> plat_alt >> plat_roll >> plat_pitch >> plat_yaw
                 >> sensor_roll >> sensor_pitch >> sensor_yaw
                 >> ul_lat >> ul_lon
                 >> ur_lat >> ur_lon
                 >> lr_lat >> lr_lon
                 >> ll_lat>>  ll_lon
                 >> slant_range>> sensor_h_fov >> sensor_v_fov
                 >> frame_center_lat >> frame_center_lon
                 >> end_of_shot;

    // std::cout << "Read entry for frame: " << frame_number << std::endl; //+ temp
  }


}; // end class MetadataEntry


typedef vcl_vector <MetadataEntry> MetadataVector_t;
typedef MetadataVector_t::iterator MetadataIterator_t;

MetadataVector_t * ReadMetadata (vcl_string filename)
{
  MetadataVector_t* meta_vector = new MetadataVector_t();

  vcl_fstream fstream;
  fstream.open(filename.c_str(), vcl_ofstream::in);
  if ( fstream.fail() )
  {
    vcl_cerr << "Open data file: " << filename;
    return 0;
  }

  while ( true )
  {
    MetadataEntry e = MetadataEntry(&fstream);

    if ( ! fstream.good()) break;

    meta_vector->push_back ( e );
  }

  //+ std::cout << meta_vector->size() << " entries read\n"; //+ temp

  return meta_vector;
}


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
/** Video Metadata factory
 *
 * This class generates a sequence of metadata objects.
 */
class VideoMetadataFactory
{
public:
  vidtk::video_metadata  m_metadata;
  vidtk::shot_break_flags m_sb_flags;
  vidtk::timestamp m_timestamp;

  VideoMetadataFactory(MetadataEntry * table)
  { }


  VideoMetadataFactory(vcl_string const& filename )
    : m_vector(0)
  {
    m_vector = ReadMetadata(G_test_directory + filename); // can return 0
    TEST ("Could not open file", m_vector == 0, false);

    m_table = m_vector->begin();
  }


  ~VideoMetadataFactory()
  {
    delete m_vector;
  }


  /** Create a new video metadata object.  The metadata object is
   * filled in with only those fields that the SP looks at.
   *
   * @retval true - new data available
   * @retval false - no new data aavailable
   */
  bool Step ()
  {

    if (m_table->frame_number <= 0)

    {
      return (false); // end of data
    }

    m_timestamp.set_frame_number ( m_table->frame_number );
    m_timestamp.set_time ( static_cast<double>( m_table->frame_time ) );

    m_metadata.timeUTC ( m_table->utc_time );
    m_metadata.platform_location ( video_metadata::lat_lon_t( m_table->plat_lat, m_table->plat_lon) );
    m_metadata.platform_altitude ( m_table->plat_alt );
    m_metadata.platform_roll ( m_table->plat_roll );
    m_metadata.platform_pitch ( m_table->plat_pitch );
    m_metadata.platform_yaw ( m_table->plat_yaw );

    m_metadata.sensor_roll ( m_table->sensor_roll );
    m_metadata.sensor_pitch ( m_table->sensor_pitch );
    m_metadata.sensor_yaw ( m_table->sensor_yaw );

    m_metadata.corner_ul ( video_metadata::lat_lon_t( m_table->ul_lat, m_table->ul_lon) );
    m_metadata.corner_ur ( video_metadata::lat_lon_t( m_table->ur_lat, m_table->ur_lon) );
    m_metadata.corner_lr ( video_metadata::lat_lon_t( m_table->lr_lat, m_table->lr_lon) );
    m_metadata.corner_ll ( video_metadata::lat_lon_t( m_table->ll_lat, m_table->ll_lon) );

    m_metadata.slant_range ( m_table->slant_range );
    m_metadata.sensor_horiz_fov ( m_table->sensor_h_fov );
    m_metadata.sensor_vert_fov ( m_table->sensor_v_fov );

    m_metadata.frame_center (video_metadata::lat_lon_t( m_table->frame_center_lat, m_table->frame_center_lon) );

    // handle status infotmation
    m_sb_flags.set_shot_end(m_table->end_of_shot);

    // update our progress
    m_table++;

    return (true);
  }

  vidtk::video_metadata  const& GetMetadata() const { return m_metadata; }
  vidtk::shot_break_flags const& GetStatus() const { return m_sb_flags; }
  vidtk::timestamp const& GetTimestamp() const { return m_timestamp; }


private:
  MetadataVector_t * m_vector;

  MetadataIterator_t m_table;
}; // end class VideoMetadataFactory


// ----------------------------------------------------------------
/** Testing delegate.
 *
 *
 */
class OutputStepDelegate
{
public:
  OutputStepDelegate() { }
  virtual ~OutputStepDelegate() { }

  virtual bool operator() ( ) { return true; }

  TestBase * m_testBase;
  TestBase * Base() { return m_testBase; }

  // accessors for inputs
  vil_image_view < vxl_byte > const& input_image();
  vidtk::timestamp const& input_timestamp();
  vcl_vector < video_metadata > const& input_metadata_vector();
  vidtk::image_to_image_homography const& input_src_to_ref_homography();
  vidtk::timestamp::vector_t const& input_timestamp_vector();
  vidtk::shot_break_flags const& input_shot_break_flags();

  // Accessors for final results
  vidtk::timestamp const& final_timestamp();
  image_to_plane_homography const& final_src_to_ref_homography();
  plane_to_image_homography const& final_wld_to_src_homography();
  plane_to_utm_homography const& final_wld_to_utm_homography();
  image_to_plane_homography const& final_ref_to_wld_homography();
  double const& final_gsd();
  vidtk::shot_break_flags const& final_shot_break_flags();
}; // end class OutputStepDelegate



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

  // current value set
  vil_image_view < vxl_byte > m_cv_img;
  timestamp m_cv_ts;
  vcl_vector < video_metadata > m_cv_vmd_vec;
  video_metadata m_cv_vmd;
  image_to_image_homography m_cv_src_to_ref;
  vidtk::timestamp::vector_t  m_cv_tsvect;
  vidtk::shot_break_flags m_cv_ps;

  // factories
  VideoMetadataFactory * m_metaFactory;
  TimestampFactory * m_timestampFactory;


  input_process(VideoMetadataFactory * fact) // CTOR
    : process ("input_proc", "input proc"),
      m_metaFactory(0), m_timestampFactory(0)
  {
    m_timestampFactory = new TimestampFactory(1, 100);
    m_metaFactory = fact;

    m_cv_src_to_ref.set_identity(true);
  }


  virtual ~input_process()
  {
    delete m_timestampFactory;
  }

  // -- process interface --
  virtual config_block params() const { return vidtk::config_block(); }
  virtual bool set_params( config_block const& ) { return true; }
  virtual bool initialize()
  {
    m_cv_img = make_image( 925, 600 );
    return true;
  }

  virtual vidtk::process::step_status step2()
  {
    // handle metadata
    if (m_metaFactory->Step () == false)
    {
      vcl_cout << "*** end of input\n";
      return vidtk::process::FAILURE;
    }
    m_cv_vmd = m_metaFactory->GetMetadata();
    m_cv_vmd_vec.clear();
    m_cv_vmd_vec.push_back(m_cv_vmd);

    m_cv_ps = m_metaFactory->GetStatus();

    m_cv_ts =  m_metaFactory->GetTimestamp();
    m_cv_tsvect.clear();
    m_cv_tsvect.push_back(m_cv_ts);

    m_cv_src_to_ref.set_identity(true);

    return vidtk::process::SUCCESS;
  }

  // -- process outputs --
  vil_image_view < vxl_byte > const& get_output_image ( ) const { return m_cv_img; }
  VIDTK_OUTPUT_PORT( vil_image_view < vxl_byte > const&, get_output_image );

  timestamp const & get_output_timestamp ( ) const { return m_cv_ts; }
  VIDTK_OUTPUT_PORT( timestamp const &, get_output_timestamp );

  vcl_vector < video_metadata > const& get_output_video_metadata ( ) const { return m_cv_vmd_vec; }
  VIDTK_OUTPUT_PORT( vcl_vector < video_metadata > const&, get_output_video_metadata );

  image_to_image_homography const& get_output_src_to_ref_homography ( ) const { return m_cv_src_to_ref; }
  VIDTK_OUTPUT_PORT( image_to_image_homography const&, get_output_src_to_ref_homography );

  vidtk::timestamp::vector_t const& get_output_timestamp_vector ( ) const { return m_cv_tsvect; }
  VIDTK_OUTPUT_PORT( vidtk::timestamp::vector_t const&, get_output_timestamp_vector );

  vidtk::shot_break_flags const& get_output_shot_break_flags ( ) const { return m_cv_ps; }
  VIDTK_OUTPUT_PORT( vidtk::shot_break_flags const&, get_output_shot_break_flags );

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

  // process values
  vidtk::timestamp m_timestamp;
  image_to_plane_homography m_src_to_ref_homography;
  plane_to_image_homography m_wld_to_src_homography;
  plane_to_utm_homography m_wld_to_utm_homography;
  image_to_plane_homography m_ref_to_wld_homography;
  double m_gsd;
  vidtk::shot_break_flags m_sb_flags;

  OutputStepDelegate * m_outputStepDelegate;


  output_process() // CTOR
    : process ("out proc", "out_proc"),
      m_outputStepDelegate(0)
  {  }

  // -- process interface --
  virtual config_block params() const { return vidtk::config_block(); }
  virtual bool set_params( config_block const& ) { return true; }
  virtual bool initialize() { return true; }

  virtual vidtk::process::step_status step2()
  {
    // Call delegate
    if (m_outputStepDelegate != 0)
    {
      if (m_outputStepDelegate->operator()() == false)
      {
        return vidtk::process::FAILURE;
      }

    }
    return vidtk::process::SUCCESS;
  }


  // -- process inputs --
  void set_input_timestamp ( vidtk::timestamp const& value )
  { m_timestamp = value; }
  VIDTK_INPUT_PORT( set_input_timestamp, vidtk::timestamp const& );

  // skip image
  // skip src to ref homog
  // skip timestamp vector

  void set_input_src_to_wld_homography ( image_to_plane_homography const& value ) { m_src_to_ref_homography = value; }
  VIDTK_INPUT_PORT( set_input_src_to_wld_homography, image_to_plane_homography const& );

  void set_input_wld_to_src_homography ( plane_to_image_homography const& value ) { m_wld_to_src_homography = value; }
  VIDTK_INPUT_PORT( set_input_wld_to_src_homography, plane_to_image_homography const& );

  void set_input_wld_to_utm_homography ( plane_to_utm_homography const& value ) { m_wld_to_utm_homography = value; }
  VIDTK_INPUT_PORT( set_input_wld_to_utm_homography, plane_to_utm_homography const& );

  void set_input_ref_to_wld_homography ( image_to_plane_homography const& value ) { m_ref_to_wld_homography = value; }
  VIDTK_INPUT_PORT( set_input_ref_to_wld_homography, image_to_plane_homography const& );

  void set_input_world_units_per_pixel ( double value ) { m_gsd = value; }
  VIDTK_INPUT_PORT( set_input_world_units_per_pixel, double  );

  void set_input_shot_break_flags ( vidtk::shot_break_flags const& value ) { m_sb_flags = value; }
  VIDTK_INPUT_PORT( set_input_shot_break_flags, vidtk::shot_break_flags const& );

}; // end class output_process


// ----------------------------------------------------------------
struct ParameterElement
{
  const char * name;
  const char * value;
};


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
  process_smart_pointer < vidtk::world_coord_super_process < vxl_byte> > m_meta_proc;
  process_smart_pointer < output_process > m_out_proc;
  vidtk::config_block m_config;


  TestingPipeline(VideoMetadataFactory * fact)
    : m_in_proc(0),
      m_meta_proc(0),
      m_out_proc(0)
  {
    // create processes
    m_in_proc = new input_process(fact);
    m_meta_proc = new world_coord_super_process < vxl_byte > ("world_coord_sp");
    m_out_proc = new output_process();

    // Add processes to pipeline
    m_pipeline.add (m_in_proc);
    m_pipeline.add (m_meta_proc);
    m_pipeline.add (m_out_proc);

    // connect inputs
    m_pipeline.connect ( m_in_proc->get_output_image_port(),             m_meta_proc->set_source_image_port() );
    m_pipeline.connect ( m_in_proc->get_output_timestamp_port(),         m_meta_proc->set_source_timestamp_port() );
    m_pipeline.connect ( m_in_proc->get_output_video_metadata_port(),    m_meta_proc->set_source_metadata_vector_port() );
    m_pipeline.connect ( m_in_proc->get_output_src_to_ref_homography_port(), m_meta_proc->set_src_to_ref_homography_port() );
    m_pipeline.connect ( m_in_proc->get_output_timestamp_vector_port(),  m_meta_proc->set_input_timestamp_vector_port() );
    m_pipeline.connect ( m_in_proc->get_output_shot_break_flags_port(),  m_meta_proc->set_input_shot_break_flags_port() );

    // connect outputs
    m_pipeline.connect ( m_meta_proc->src_to_wld_homography_port(),          m_out_proc->set_input_src_to_wld_homography_port() );
    m_pipeline.connect ( m_meta_proc->wld_to_src_homography_port(),          m_out_proc->set_input_wld_to_src_homography_port() );
    m_pipeline.connect ( m_meta_proc->wld_to_utm_homography_port(),          m_out_proc->set_input_wld_to_utm_homography_port() );
    m_pipeline.connect ( m_meta_proc->get_output_ref_to_wld_homography_port(), m_out_proc->set_input_ref_to_wld_homography_port() );
    m_pipeline.connect ( m_meta_proc->world_units_per_pixel_port(),          m_out_proc->set_input_world_units_per_pixel_port() );
    m_pipeline.connect ( m_meta_proc->get_output_shot_break_flags_port(),    m_out_proc->set_input_shot_break_flags_port() );
  }

  bool SetParams (ParameterElement * parms)
  {
    m_config.add_subblock( m_pipeline.params(), "test" );
    while (parms->name != 0)
    {
      vcl_string name("test:world_coord_sp:");
      name += parms->name;
      m_config.set_value( name, parms->value);

      parms++; // go to next set of parameters
    } // end for

    return m_pipeline.set_params( m_config.subblock("test") );
}

  bool Initialize () { return m_pipeline.initialize(); }
  bool Run() { return m_pipeline.run(); }

}; // end class TestingPipeline


// Test parameters (parameters, data_table, testing_predicate, test_name)


// ----------------------------------------------------------------
/** Base class for all tests.
 *
 * Call Setup to configure the test.
 * Call Run() to run the test.
 */
class TestBase
{
public:
  TestBase(const char * name, ParameterElement * params, vcl_string const& filename)
    : m_parms(params),
      m_testName(name),
      m_dataFactory(0),
      m_testPipeline(0)
  {
    m_dataFactory = new VideoMetadataFactory(filename);
    m_testPipeline = new TestingPipeline(m_dataFactory);
  }

  virtual ~TestBase()
  {
    delete m_testPipeline;
    delete m_dataFactory;
  }

  void SetTestDelegate (OutputStepDelegate * del)
  {
    del->m_testBase = this;
    m_testPipeline->m_out_proc->m_outputStepDelegate = del;
  }


// #define TSTR(X) vcl_string( m_testName + " " + oss.str() + X ).c_str()
#define TSTR(X) vcl_string( m_testName + ": " +  X ).c_str()

  bool Setup()
  {
    bool rc = m_testPipeline->SetParams( m_parms );
    TEST( TSTR("set params"), rc, true );

    rc = m_testPipeline->Initialize();
    TEST( TSTR("pipeline init"), rc, true );
    return true;
  }

  bool Run()
  {
    bool run_status = m_testPipeline->Run();
    TEST( TSTR("pipeline run"), run_status, true );
    PostTest();

    return true;
  }

  virtual void PostTest() = 0;

  // accessors for inputs
  vil_image_view < vxl_byte > const& input_image() { return m_testPipeline->m_in_proc->m_cv_img; }
  vidtk::timestamp const& input_timestamp() { return m_testPipeline->m_in_proc->m_cv_ts; }
  vcl_vector < video_metadata > const& input_metadata_vector() { return m_testPipeline->m_in_proc->m_cv_vmd_vec; }
  vidtk::image_to_image_homography const& input_src_to_ref_homography() { return m_testPipeline->m_in_proc->m_cv_src_to_ref; }
  vidtk::timestamp::vector_t const& input_timestamp_vector() { return m_testPipeline->m_in_proc->m_cv_tsvect; }
  vidtk::shot_break_flags const& input_shot_break_flags() { return m_testPipeline->m_in_proc->m_cv_ps; }

  // Accessors for final results
  vidtk::timestamp const& final_timestamp() { return m_testPipeline->m_out_proc->m_timestamp; }
  image_to_plane_homography const& final_src_to_ref_homography() { return m_testPipeline->m_out_proc->m_src_to_ref_homography; }
  plane_to_image_homography const& final_wld_to_src_homography() { return m_testPipeline->m_out_proc->m_wld_to_src_homography; }
  plane_to_utm_homography const& final_wld_to_utm_homography() { return m_testPipeline->m_out_proc->m_wld_to_utm_homography; }
  image_to_plane_homography const& final_ref_to_wld_homography() { return m_testPipeline->m_out_proc->m_ref_to_wld_homography; }
  double const& final_gsd() { return m_testPipeline->m_out_proc->m_gsd; }
  vidtk::shot_break_flags const& final_shot_break_flags() { return m_testPipeline->m_out_proc->m_sb_flags; }


  void PrintFinal()
  {
    vcl_cout << "final_timestamp: " << final_timestamp() << "\n"
             << "final_src_to_ref_homography: " << final_src_to_ref_homography() << "\n"
             << "final_wld_to_src_homography: " << final_wld_to_src_homography() << "\n"
             << "final_wld_to_utm_homography: " << final_wld_to_utm_homography() << "\n"
             << "final_ref_to_wld_homography: " << final_ref_to_wld_homography() << "\n"
             << "final_gsd: " << final_gsd() << "\n"
             << "final_sb_flags: " << final_shot_break_flags() << "\n"
      ;

  }


protected:
  ParameterElement * m_parms;
  vcl_string m_testName;

  VideoMetadataFactory * m_dataFactory;
  TestingPipeline * m_testPipeline;


}; // end class TestBase

// accessors for inputs
vil_image_view< vxl_byte > const& OutputStepDelegate::input_image() { return ( Base()->input_image() ); }
vidtk::timestamp const& OutputStepDelegate::input_timestamp() { return ( Base()->input_timestamp() ); }
vcl_vector< video_metadata > const& OutputStepDelegate::input_metadata_vector() { return ( Base()->input_metadata_vector() ); }
vidtk::image_to_image_homography const& OutputStepDelegate::input_src_to_ref_homography() { return ( Base()->input_src_to_ref_homography() ); }
vidtk::timestamp::vector_t const& OutputStepDelegate::input_timestamp_vector() { return ( Base()->input_timestamp_vector() ); }
vidtk::shot_break_flags const& OutputStepDelegate::input_shot_break_flags() { return ( Base()->input_shot_break_flags() ); }

// Accessors for final results
vidtk::timestamp const& OutputStepDelegate::final_timestamp() { return ( Base()->final_timestamp() ); }
image_to_plane_homography const& OutputStepDelegate::final_src_to_ref_homography() { return ( Base()->final_src_to_ref_homography() ); }
plane_to_image_homography const& OutputStepDelegate::final_wld_to_src_homography() { return ( Base()->final_wld_to_src_homography() ); }
plane_to_utm_homography const& OutputStepDelegate::final_wld_to_utm_homography() { return ( Base()->final_wld_to_utm_homography() ); }
image_to_plane_homography const& OutputStepDelegate::final_ref_to_wld_homography() { return ( Base()->final_ref_to_wld_homography() ); }
double const& OutputStepDelegate::final_gsd() { return ( Base()->final_gsd() ); }
vidtk::shot_break_flags const& OutputStepDelegate::final_shot_break_flags() { return ( Base()->final_shot_break_flags() ); }


// ================================================================
void test_gsd()
{
  ParameterElement parms[] =
  {
    { "metadata_type",    "stream"  },
    { "gen_gsd:disabled", "false"   },
    {                  0,         0 } // end marker
  };

  class Test_GSD : public TestBase
  {
  public:
    Test_GSD(ParameterElement * params, vcl_string const& filename)
    : TestBase("test GSD", params, filename)
    {
    }

    virtual void PostTest()
    {
      // PrintFinal();
      TEST_NEAR("GSD value", final_gsd(), 0.0401373, 6e-8);
    }


  }; // end class Test_GSD


  Test_GSD test(parms, "wcsp_data_1.dat");

  test.Setup();
  test.Run();

} // end of test


// ================================================================
void test_end_of_video()
{
  ParameterElement internal_params[] =
  {
    { "metadata_type",    "stream"  },
    { "gen_gsd:disabled", "false"   },
    { "fallback_gsd",     "1.0"     },
    { "fallback_count",   "10"      },
    {                0,           0 } // end marker
  };

  class Test_EOV : public TestBase
  {
  public:
    Test_EOV(ParameterElement * params, vcl_string const& filename)
    : TestBase("test end of video trigger", params, filename)
    {
    }

    virtual void PostTest()
    {
      TEST("Valid EOF GSD", final_gsd() == 1.0, true);
    }


  };

  Test_EOV test(internal_params, "wcsp_data_3.dat");

  test.Setup();
  test.Run();

}


// ================================================================
void test_break()
{
  ParameterElement parms[] =
    {
      { "metadata_type",    "stream"  },
      { "gen_gsd:disabled", "false"   },
      {                  0,         0 } // end marker
    };


  class del : public OutputStepDelegate
  {
  public:
    virtual bool operator() ( )
    {
      if (input_shot_break_flags().is_shot_end())
      {

      }

      return true;
    }



  };



  class Test_one : public TestBase
  {
  public:
    Test_one(ParameterElement * params, vcl_string const& filename)
      : TestBase ("test shot break", params, filename) { }

    virtual void PostTest()
    {
//      PrintFinal();
    }
  }; // end class Test_GSD

  Test_one test (parms, "wcsp_data_2.dat");

  del m_del;
  test.SetTestDelegate(&m_del);

  test.Setup();
  test.Run();

} // end of test



#if 0

// ================================================================
void test_one( vcl_string test_name)
{
  VideoMetadataFactory * fact1 = new VideoMetadataFactory("wcsp_data_1");

  ParameterElement parms[] = {
    { "metadata_type",  "stream" },
    { "gen_gsd:disabled", "false" },
    //{ "src_img_queue:max_length", "5" },
    //{ "src_timestamp_queue:max_length", "5" },
    //{ "src2ref_homog_queue:max_length", "5" },
    //{ "src_timestamp_vector_queue:max_length", "5" },
    {0,0} // end marker
  };

  TestingPipeline tpl(fact1);

  Test_GSD test (parms, table1);


#define TSTR(X) vcl_string( test_name + " " + oss.str() + X ).c_str()
  {
    vcl_ostringstream oss;
    TEST( TSTR("set params"), tpl.SetParams( parms ), true );
    TEST( TSTR("pipeline init"), tpl.m_pipeline.initialize(), true );
    bool run_status = tpl.m_pipeline.run();
    TEST( TSTR("pipeline run"), run_status, true );

  }

}

#endif

} // end anonymous namespace



// ----------------------------------------------------------------
/** Main test driver
 *
 *
 */
int test_world_coord_super_process( int argc, char * argv[] )
{
  testlib_test_start( "world coordinate super process" );
  if ( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    G_test_directory = argv[1];
    G_test_directory +=  "/";

    test_gsd();
    test_break();
    test_end_of_video();
  }

  return testlib_test_summary();

}
