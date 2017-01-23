/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <sstream>
#include <string>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>

#include <pipeline_framework/async_pipeline.h>

#include <tracking/shot_stitching_process.h>

#include <utilities/homography.h>

#include <testlib/testlib_test.h>


// Local testing macro
// Be sure to set TEST_NAME to the name of the test.
#define SBTEST(TXT, V1, V2)                     \
do {                                            \
    std::ostringstream sboss;                    \
    sboss << TEST_NAME << ": " << TXT;          \
    TEST(sboss.str().c_str(), V1, V2);          \
} while(0)



namespace { // anon

// ==================================================
//
// The proc_data_type is used to hold process inputs and outputs.
//
// ==================================================

struct proc_data_type
{
  vil_image_view< vxl_byte > img;
  vil_image_view< bool > mask;
  vidtk::video_metadata::vector_t m;
  vidtk::timestamp ts;
  vidtk::shot_break_flags sb_flags;
  vidtk::image_to_image_homography src2ref;


  proc_data_type()
  {
    img.set_size( 1, 1 );
    img.fill( 0 );
    src2ref.set_identity( true );
    m.push_back( vidtk::video_metadata() );
    m.push_back( vidtk::video_metadata() );
  }


  explicit proc_data_type( const vidtk::shot_stitching_process<vxl_byte>& proc)
  {
    this->set_from_proc_outputs( proc );
  }


  void copy_to_proc_inputs( vidtk::shot_stitching_process<vxl_byte>& proc )
  {
    proc.set_input_source_image( img );
    proc.set_input_rescaled_image( img );
    proc.set_input_timestamp( ts );
    proc.set_input_shot_break_flags( sb_flags );
    proc.set_input_src2ref_h( src2ref );
  }


  void set_from_proc_outputs( const vidtk::shot_stitching_process<vxl_byte>& proc )
  {
    img = proc.output_source_image();
    ts = proc.output_timestamp();
    sb_flags = proc.output_shot_break_flags();
    src2ref = proc.output_src2ref_h();
  }
};


bool
operator==( const proc_data_type& lhs, const proc_data_type& rhs )
{
  return
    (lhs.img == rhs.img) &&
    (lhs.m[0].timeUTC() == rhs.m[0].timeUTC()) &&
    (lhs.ts  == rhs.ts)  &&
    (lhs.sb_flags == rhs.sb_flags) &&
    (lhs.src2ref.get_transform() == rhs.src2ref.get_transform());
}


// ==================================================
//
// Since we have to test the process in an asynchronous setting,
// the proc_data_source and proc_data_sink classes are used
// to supply inputs and collect outputs from the async pipeline.
//
// ==================================================

struct proc_data_source_process
  : public vidtk::process
{
  // outside the pipeline framework, expose the data buffer
  // and allow the user to load it.  Call data_list().push_back(foo)
  // to load up the source before the pipeline executes.

  std::vector< proc_data_type> d;
  std::vector< proc_data_type>::iterator d_index;
  std::vector< proc_data_type >& data_list() { return d; }

  // inside the pipeline, read data off the list at each step
  // until we hit the end.

  proc_data_type v;

  typedef proc_data_source_process self_type;

  proc_data_source_process( const std::string& _name )
    : vidtk::process( _name, "proc_data_source")
  {
    d_index = d.end();
  }

  vidtk::config_block params() const { return vidtk::config_block(); }
  bool set_params( const vidtk::config_block& ) { return true; }
  bool initialize() { d_index = d.begin(); return true; }

  bool step()
  {
    if (d_index == d.end())
    {
      return false;
    }
    else
    {
      v = *d_index;
      ++d_index;
      return true;
    }
  }

  // needs to have output ports to match the shot_stitching_process
  // input ports

  vil_image_view<vxl_byte> output_image() const { return v.img; }
  VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte>, output_image );

  vidtk::image_to_image_homography output_src2ref_h() const { return v.src2ref; }
  VIDTK_OUTPUT_PORT( vidtk::image_to_image_homography, output_src2ref_h );

  vidtk::timestamp output_timestamp() const { return v.ts; }
  VIDTK_OUTPUT_PORT( vidtk::timestamp, output_timestamp );

  vidtk::shot_break_flags output_shot_break_flags() const { return v.sb_flags; }
  VIDTK_OUTPUT_PORT( vidtk::shot_break_flags, output_shot_break_flags );

  vidtk::video_metadata::vector_t output_metadata_vector() const { return v.m; }
  VIDTK_OUTPUT_PORT( vidtk::video_metadata::vector_t, output_metadata_vector );

  // klt_tracks
  std::vector< vidtk::klt_track_ptr > klt_tracks_output;
  std::vector< vidtk::klt_track_ptr > output_klt_tracks() const { return klt_tracks_output; }
  VIDTK_OUTPUT_PORT( std::vector < vidtk::klt_track_ptr >, output_klt_tracks );
};


struct proc_data_sink_process
  : public vidtk::process
{
  // outside the pipeline framework, expose the data buffer
  // and allow the user to access it.  Call data_list() to examine
  // values after the pipeline has executed.

  std::vector< proc_data_type> d;
  const std::vector< proc_data_type >& data_list() { return d; }

  // inside the pipeline, accept and store outputs at each step.

  proc_data_type v;

  typedef proc_data_sink_process self_type;

  proc_data_sink_process( const std::string& _name )
    : vidtk::process( _name, "proc_data_sink")
  {
  }

  vidtk::config_block params() const { return vidtk::config_block(); }
  bool set_params( const vidtk::config_block& ) { return true; }
  bool initialize() { d.clear(); return true; }

  bool step()
  {
    d.push_back( v );
    return true;
  }

  void input_image( const vil_image_view<vxl_byte>& i ) { v.img = i; }
  VIDTK_INPUT_PORT( input_image, const vil_image_view<vxl_byte>& );

  void input_metadata_vector( const vidtk::video_metadata::vector_t& md ) { v.m = md; }
  VIDTK_INPUT_PORT( input_metadata_vector, const vidtk::video_metadata::vector_t& );

  void input_timestamp( const vidtk::timestamp& t ) { v.ts = t; }
  VIDTK_INPUT_PORT( input_timestamp, const vidtk::timestamp& );

  void input_shot_break_flags( const vidtk::shot_break_flags& s ) { v.sb_flags = s; }
  VIDTK_INPUT_PORT( input_shot_break_flags, const vidtk::shot_break_flags& );

  void input_src2ref_h( const vidtk::image_to_image_homography& h ) { v.src2ref = h; }
  VIDTK_INPUT_PORT( input_src2ref_h, const vidtk::image_to_image_homography& );
};

// ==================================================
//
// Creates a test pipeline with a source, process node, and sink.
//
// ==================================================

struct ssp_test_pipeline
{
  vidtk::async_pipeline p;
  vidtk::process_smart_pointer< proc_data_source_process > src;
  vidtk::process_smart_pointer< vidtk::shot_stitching_process<vxl_byte> > ssp;
  vidtk::process_smart_pointer< proc_data_sink_process > dst;

  ssp_test_pipeline()
    : src( new proc_data_source_process( "src") ),
      ssp( new vidtk::shot_stitching_process<vxl_byte>( "stitch" )),
      dst( new proc_data_sink_process( "dst" ) )
  {
    p.add( src );
    p.add( ssp );

    p.connect( src->output_image_port(),            ssp->set_input_source_image_port() );
    p.connect( src->output_image_port(),            ssp->set_input_rescaled_image_port() );
    p.connect( src->output_src2ref_h_port(),        ssp->set_input_src2ref_h_port() );
    p.connect( src->output_timestamp_port(),        ssp->set_input_timestamp_port() );
    p.connect( src->output_shot_break_flags_port(), ssp->set_input_shot_break_flags_port() );
    p.connect( src->output_metadata_vector_port(),  ssp->set_input_metadata_vector_port() );
    p.connect( src->output_klt_tracks_port(),       ssp->set_input_klt_tracks_port() );

    p.add( dst );
    p.connect( ssp->output_source_image_port(),     dst->input_image_port() );
    p.connect( ssp->output_timestamp_port(),        dst->input_timestamp_port() );
    p.connect( ssp->output_shot_break_flags_port(), dst->input_shot_break_flags_port() );
    p.connect( ssp->output_src2ref_h_port(),        dst->input_src2ref_h_port() );
  }

  bool set_params_and_init( const vidtk::config_block& cfg )
  {
    return p.set_params( cfg ) && p.initialize();
  }

  vidtk::config_block params() const
  {
    return p.params();
  }

};


enum {IMG_SQUARE, IMG_SQUARE_TRANS_7_4, IMG_SQUARE_TRANS_13_1, IMG_BLANK };
struct GLOBALS
{
  vil_image_view< vxl_byte > imgs[4];
  bool init(const std::string& data_dir)
  {
    std::string img_fns[3] = { "square.png", "square_trans+7+4.png", "square_trans+13-1.png" };
    for (unsigned i = 0; i < 3; ++i)
    {
      std::string full_path = data_dir + "/" + img_fns[i];
      imgs[i] = vil_load( full_path.c_str() );
      if ( !imgs[i] )
      {
        std::ostringstream oss;
        oss << "Failed to load '" << full_path << "'";
        TEST(oss.str().c_str(), false, true);
        return ( false );
      }
    }
    imgs[IMG_BLANK] = vil_image_view< vxl_byte >(128, 128, 3);
    imgs[IMG_BLANK].fill(0);
    return ( true );
  }


} G;


// ----------------------------------------------------------------
/** Compare two matrices.
 *
 * This function compares two matrices. Note that the normalized
 * values are compared.
 *
 * @param a one matrix
 * @param b other matrix
 * @param fuzz how close the compare must be
 *
 * @return TRUE if compare succeeds, else FALSE
 */
bool
matrices_match(const vgl_h_matrix_2d< double >&  a,
               const vgl_h_matrix_2d< double >&  b,
               double                            fuzz = 1.0e-05)
{
  for (unsigned r = 0; r < 3; ++r)
  {
    for (unsigned c = 0; c < 3; ++c)
    {
      double atmp = ( a.get(2, 2) == 0 ) ? 0 : a.get(r, c) / a.get(2, 2);
      double btmp = ( b.get(2, 2) == 0 ) ? 0 : b.get(r, c) / b.get(2, 2);
      if (std::abs(atmp - btmp) > fuzz)
      {
        std::cout << "\nMismatch at [" << r << "," << c << "] = "
                  << atmp << " - " << btmp << " = "
                  << std::abs(atmp - btmp) << "(" << fuzz << ")"
                  << std::endl;
        return false;
      }
    }
  }
  return true;
}



void
test_param_sets(std::string algo_type)
{
  vidtk::shot_stitching_process<vxl_byte> proc( "s" );
  {
    vidtk::config_block blk = proc.params();
    blk.set( "algo:type", algo_type );
    blk.set( "algo:klt:ssp_klt_tracking:feature_count", "10" );
    blk.set( "algo:klt:threshold", "0.3" );
#ifdef WITH_MAPTK_ENABLED
    blk.set( "algo:maptk:min_inliers", "4" );
#endif

    std::stringstream ss;
    ss << "Process: setting " << algo_type << " params does not trap";
    TEST( ss.str().c_str(), proc.set_params( blk ), true );
  }
  {
    ssp_test_pipeline p;
    vidtk::config_block blk = p.params();
    blk.set( "stitch:algo:type", algo_type );
    blk.set( "stitch:algo:klt:ssp_klt_tracking:feature_count", "10" );
    blk.set( "stitch:algo:klt:threshold", "0.3" );
#ifdef WITH_MAPTK_ENABLED
    blk.set( "stitch:algo:maptk:min_inliers", "4" );
#endif

    std::stringstream ss;
    ss << "Test pipeline: setting " << algo_type << " params does not trap";
    TEST( ss.str().c_str(), p.set_params_and_init( blk ), true );
  }
}

void
test_process_disabled()
{
  ssp_test_pipeline p;

  const unsigned N = 4;

  for (unsigned i = 0; i < N; ++i)
  {
    p.src->d.push_back( proc_data_type() );
    p.src->d[i].ts = vidtk::timestamp(i * 1000, i);
  }

  vgl_h_matrix_2d< double > xform;
  xform.set_identity();

  // randomly permute some fields
  p.src->d[0].sb_flags.set_frame_usable(true);
  xform.set_translation(3, 17);
  p.src->d[1].src2ref.set_transform(xform);

  vidtk::config_block blk = p.params();
  blk.set("stitch:enabled", "0");
  blk.set("stitch:verbose", "1");
  TEST("Disabled: param set & init succeeds", p.set_params_and_init(blk), true);
  TEST("Disabled: before run, output set is empty", p.dst->d.empty(), true);

  bool run_status = p.p.run();
  TEST("Disable: pipeline runs", run_status, true);
  bool all_outputs_present = ( p.dst->d.size() == N );
  {
    std::ostringstream oss;
    oss << "Disable: pipeline output has " << p.dst->d.size() << " (should be " << N << ")";
    TEST(oss.str().c_str(), all_outputs_present, true);
  }
  if ( !all_outputs_present )
  {
    return;
  }

  for (unsigned i = 0; i < N; ++i)
  {
    std::ostringstream oss;
    oss << "Disabled: output " << i << " of " << N << ": output matches input";
    TEST(oss.str().c_str(), p.src->d[i], p.dst->d[i]);
  }
}


// Enable the frames via a config string, e.g. "BBBBG" specifies five
// frames, all bad but the last one
// For "inputs==outputs", no stitching should be triggered

void
test_process_enabled_no_stitching( const std::string& conf_str,
                                   const std::string& max_n_glitch_frames_str = "1" )
{
  ssp_test_pipeline p;

  unsigned N = conf_str.size();
  for (unsigned i = 0; i < N; ++i)
  {
    p.src->d.push_back( proc_data_type() );
    p.src->d[i].ts = vidtk::timestamp( i*1000, i );
    p.src->d[i].sb_flags.set_frame_usable( conf_str[i] == 'G' );
  }

  vgl_h_matrix_2d<double> xform;
  xform.set_identity();
  if (N > 0)
  {
    xform.set_translation( 3, 17 );
    p.src->d[0].src2ref.set_transform( xform );
  }

  vidtk::config_block blk = p.params();
  blk.set( "stitch:enabled", "1" );
  blk.set( "stitch:max_n_glitch_frames", max_n_glitch_frames_str );
  blk.set( "stitch:verbose", "1" );
  {
    std::ostringstream oss;
    oss << "Enabled, no stitch: '" << conf_str << "': param set & init succeeds";
    TEST( oss.str().c_str(), p.set_params_and_init( blk ), true );
  }

  bool run_status = p.p.run();
  {
    std::ostringstream oss;
    oss << "Enabled, no stitch: '" << conf_str << "': pipeline runs";
    TEST( oss.str().c_str(), run_status, true );
  }
  bool all_outputs_present = (p.dst->d.size() == N);
  {
    std::ostringstream oss;
    oss << "Enabled, no stitch: '" << conf_str << "': pipeline output has "
        << p.dst->d.size() << " (should be " << N << ")";
    TEST( oss.str().c_str(), all_outputs_present, true );
  }
  if ( ! all_outputs_present ) return;

  for (unsigned i=0; i<N; ++i)
  {
    std::ostringstream oss;
    oss << "Enabled, no stitch: '" << conf_str << "': "
        << i << " of " << N << ": output matches input";
    TEST( oss.str().c_str(), p.src->d[i], p.dst->d[i] );
  }
}


// ----------------------------------------------------------------
/**
 *
 *
 */
void test_stitch_identity_across_single_bad_frame(std::string algo_type)
{
  // three frames, all the square, middle one "bad";
  // stitching should jump the gap and return identity
  // for all homographies

  ssp_test_pipeline p;
  const unsigned N = 3;
  for (unsigned i= 0 ; i < N; ++i)
  {
    p.src->d.push_back( proc_data_type() );
    p.src->d[i].img = G.imgs[IMG_SQUARE];
    p.src->d[i].ts = vidtk::timestamp( i*1000, i );
    p.src->d[i].sb_flags.set_frame_usable( true );
    p.src->d[i].sb_flags.set_shot_end( false );
    p.src->d[i].src2ref.set_valid (true);
  }

  vgl_h_matrix_2d<double> xform;
  xform.set_identity();
  p.src->d[0].src2ref.set_transform( xform );
  p.src->d[2].src2ref.set_transform( xform );

  // shot break at index 1
  p.src->d[1].sb_flags.set_frame_usable( false );
  p.src->d[1].sb_flags.set_shot_end( true );
  p.src->d[1].src2ref.set_valid (false);
  p.src->d[2].src2ref.set_new_reference(true);


  vidtk::config_block blk = p.params();
  blk.set( "stitch:enabled", "1" );
  blk.set( "stitch:algo:type", algo_type );
  blk.set( "stitch:algo:klt:ssp_klt_tracking:feature_count", "10" );
  blk.set( "stitch:algo:klt:threshold", "0.3" );
  blk.set( "stitch:max_n_glitch_frames", "1" );
  blk.set( "stitch:verbose", "1" );
#ifdef WITH_MAPTK_ENABLED
  blk.set( "stitch:algo:maptk:min_inliers", "4" );
#endif

  TEST( "Identity-one-bad-frame: param set & init succeeds",
        p.set_params_and_init( blk ), true );

  bool run_status = p.p.run();
  TEST( "Identity-one-bad-frame: pipeline runs", run_status, true );

  bool all_outputs_present = (p.dst->d.size() == N);
  {
    std::ostringstream oss;
    oss << "Identity-one-bad-frame: pipeline output has "
        << p.dst->d.size() << " (should be " << N << ")";
    TEST( oss.str().c_str(), all_outputs_present, true );
  }
  if ( ! all_outputs_present ) return;

  TEST( "Identity-one-bad-frame: Frame 0 output matches",
        p.src->d[0], p.dst->d[0] );

  // did we actually interpolate?
  TEST( "Identity-one-bad-frame: Frame 1 shot end",
          p.dst->d[1].sb_flags.is_shot_end(), true );
  TEST( "Identity-one-bad-frame: Frame 1 homography is (roughly) identity",
        matrices_match( p.dst->d[1].src2ref.get_transform(), xform ), true );
  TEST( "Identity-one-bad-frame: Frame 2 homography is (roughly) identity",
        matrices_match( p.dst->d[2].src2ref.get_transform(), xform ), true );
}


// ----------------------------------------------------------------
/**
 *
 *
 */
void test_stitch_motion_across_single_bad_frame(std::string algo_type)
{
#define TEST_NAME "Motion-one-bad-frame"

  // three frames, all the square, middle one "bad";
  // stitching should jump the gap and return identity
  // for all homographies
  vgl_h_matrix_2d<double> xform;
  xform.set_identity();

  ssp_test_pipeline p;
  const unsigned N = 10;
  for (unsigned i = 0; i < N; ++i)
  {
    p.src->d.push_back( proc_data_type() );
    p.src->d[i].ts = vidtk::timestamp( (1+i)*1000, (1+i) );
    p.src->d[i].sb_flags.set_frame_usable( true );
    p.src->d[i].sb_flags.set_shot_end( false );

    p.src->d[i].src2ref.set_valid (true);
    p.src->d[i].src2ref.set_dest_reference( p.src->d[0].ts );
    p.src->d[i].src2ref.set_source_reference( p.src->d[i].ts );

    p.src->d[i].img = G.imgs[IMG_SQUARE];
    p.src->d[i].src2ref.set_transform( xform );
  }

  // index 0:
  // -- no change

  // index 1: anchor
  p.src->d[1].img = G.imgs[IMG_SQUARE_TRANS_7_4];

  vgl_h_matrix_2d<double> truth_1to0;
  truth_1to0.set_identity();
  // This transformation doesn't need to exactly match the image. This is mainly
  // used to ensure that this transformation is preserved as-is after stitching.
  truth_1to0.set_translation( -7, -4 );
  p.src->d[1].src2ref.set_transform( truth_1to0 );

  // index 2: shot break
  p.src->d[2].sb_flags.set_frame_usable( false );
  p.src->d[2].sb_flags.set_shot_end( true );
  p.src->d[2].src2ref.set_valid (false);

  // index 3: new-ref
  p.src->d[3].src2ref.set_new_reference(true);
  p.src->d[3].img = G.imgs[IMG_SQUARE_TRANS_13_1];

  // index 4: not new ref
  // -- no change - start of shot

  // configure stitcher
  vidtk::config_block blk = p.params();
  blk.set( "stitch:enabled", "1" );
  blk.set( "stitch:algo:type", algo_type );
  blk.set( "stitch:algo:klt:ssp_klt_tracking:feature_count", "10" );
  blk.set( "stitch:algo:klt:threshold", "0.3" );
  blk.set( "stitch:max_n_glitch_frames", "12" );
  blk.set( "stitch:verbose", "1" );
#ifdef WITH_MAPTK_ENABLED
  blk.set( "stitch:algo:maptk:min_inliers", "4" );
#endif

  SBTEST( "param set & init succeeds",
        p.set_params_and_init( blk ), true );

  // -- actually process inputs --
  bool run_status = p.p.run();
  SBTEST( "pipeline runs", run_status, true );

  bool all_outputs_present = (p.dst->d.size() == N-1);
  {
    std::ostringstream oss;
    oss << "pipeline output has "
        << p.dst->d.size() << " (should be " << N-1 << ")";
    SBTEST( oss.str().c_str(), all_outputs_present, true );
  }
  if ( ! all_outputs_present ) return;

  SBTEST( "Frame 0 output matches",
        p.src->d[0], p.dst->d[0] );

  SBTEST( "Frame 1 output matches",
        p.src->d[1], p.dst->d[1] );

  // did we actually interpolate?
  SBTEST( "Frame 2 processed",
        p.dst->d[2].sb_flags.is_shot_end(), false );

  SBTEST( "Frame 0 homography is (roughly) identity",
        matrices_match( p.dst->d[0].src2ref.get_transform(), xform), true );

  SBTEST( "Frame 3 reference",
        p.dst->d[3].src2ref.get_dest_reference(), p.dst->d[0].src2ref.get_dest_reference());

  SBTEST( "Frame 4 reference",
        p.dst->d[4].src2ref.get_dest_reference(), p.dst->d[0].src2ref.get_dest_reference());

  // since frame 0 = (0,0), frame 1 = (+7,+4), and frame 3 is (+13,-1), src2ref (aka frame 2 to
  // frame 0) should be (-13, +1). This will be computed after the stitch of
  // (-5,+5) from frame 3 to 1, is added to (-7,-4) from frame 1 to 0.
  vgl_h_matrix_2d<double> truth;
  truth.set_identity();
  truth.set_translation( -13.0, 1.0 );

  SBTEST( "Frame 3 homography is (roughly) -13+1 (src back to ref)",
        matrices_match( p.dst->d[3].src2ref.get_transform(), truth, 1.0),
        true );

#undef TEST_NAME
}


#define SB(N)                                           \
  p.src->d[N].sb_flags.set_frame_usable( false );       \
  p.src->d[N].sb_flags.set_shot_end( true );            \
  p.src->d[N].src2ref.set_valid (false)

#define NR(N)                                   \
  p.src->d[N].src2ref.set_new_reference(true)


// ----------------------------------------------------------------
/**
 * test across multiple breaks
 *  -, sb, nr, sb, nr, -, -, -, -, sb, nr, -, -, -, - (15 frames)
 *
 */
void test_stitch_motion_across_multi_break(std::string algo_type)
{
#define TEST_NAME "multi-break"

  // three frames, all the square, middle one "bad";
  // stitching should jump the gap and return identity
  // for all homographies
  vgl_h_matrix_2d<double> xform;
  xform.set_identity();

  ssp_test_pipeline p;
  const unsigned N = 16;
  for (unsigned i = 0; i < N; ++i)
  {
    p.src->d.push_back( proc_data_type() );
    p.src->d[i].ts = vidtk::timestamp( (1+i)*1000, (1+i) );
    p.src->d[i].sb_flags.set_frame_usable( true );
    p.src->d[i].sb_flags.set_shot_end( false );

    p.src->d[i].src2ref.set_valid (true);
    p.src->d[i].src2ref.set_dest_reference( p.src->d[0].ts );
    p.src->d[i].src2ref.set_source_reference( p.src->d[i].ts );

    p.src->d[i].img = G.imgs[IMG_SQUARE];
    p.src->d[i].src2ref.set_transform( xform );
  }

  p.src->d[2].img = G.imgs[IMG_SQUARE_TRANS_13_1];

  // Configure input frames as follows
  //   0   1  2   3   4   5  6  7  8  9   10  11 12 13 14
  //   -, sb, nr, sb, nr, -, -, -, -, sb, nr, -, -, -, - (15 frames)
  //   results
  //   p  d   d  d    p   p  p p p    d   p   p  p  p  p   (drop, pass)
  //   0              1   2  3 4 5        6   7  8  9  10
  SB(1);
  NR(2);
  SB(3);
  NR(4);
  SB(9);
  NR(10);

  // configure stitcher
  vidtk::config_block blk = p.params();
  blk.set( "stitch:enabled", "1" );
  blk.set( "stitch:algo:type", algo_type );
  blk.set( "stitch:algo:klt:ssp_klt_tracking:feature_count", "10" );
  blk.set( "stitch:algo:klt:threshold", "0.3" );
  blk.set( "stitch:max_n_glitch_frames", "12" );
  blk.set( "stitch:verbose", "1" );
#ifdef WITH_MAPTK_ENABLED
  blk.set( "stitch:algo:maptk:min_inliers", "4" );
#endif

  SBTEST( "param set & init succeeds", p.set_params_and_init( blk ), true );

  // -- actually process inputs --
  bool run_status = p.p.run();
  SBTEST( "pipeline runs", run_status, true );

  bool all_outputs_present = (p.dst->d.size() == N-4);
  {
    std::ostringstream oss;
    oss << "pipeline output has "
        << p.dst->d.size() << " (should be " << N-4 << ")";
    SBTEST( *(oss.str().c_str()), all_outputs_present, true );
  }
  if ( ! all_outputs_present ) return;

  SBTEST( "Frame 0 output matches", p.src->d[0], p.dst->d[0] );

  // did we actually interpolate?
  SBTEST( "Frame 1 processed", p.dst->d[1].sb_flags.is_shot_end(), false );

  SBTEST( "Frame 1 homography is (roughly) identity",  matrices_match( p.dst->d[0].src2ref.get_transform(), xform), true );

  SBTEST( "Frame 4 reference", p.dst->d[4].src2ref.get_dest_reference(), p.dst->d[0].src2ref.get_dest_reference());
  SBTEST( "Frame 5 reference", p.dst->d[5].src2ref.get_dest_reference(), p.dst->d[0].src2ref.get_dest_reference());
  SBTEST( "Frame 7 reference", p.dst->d[7].src2ref.get_dest_reference(), p.dst->d[0].src2ref.get_dest_reference());
  SBTEST( "Frame 9 reference", p.dst->d[9].src2ref.get_dest_reference(), p.dst->d[0].src2ref.get_dest_reference());

#undef TEST_NAME
}


// test across short shot
// sb, nr, -, -, sb, nr, -, -, -, -, - (11 frames)

} // anon namespace

// ----------------------------------------------------------------
/** Main test driver
 *
 *
 */
int test_shot_stitching_process( int argc, char* argv[] )
{
  testlib_test_start( "shot stitching process" );
  if ( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    bool data_ready = G.init( argv[1] );
    TEST( "Data loaded", data_ready, true );
    if (data_ready)
    {
      // basic sanity tests
      test_param_sets("klt");
      test_process_disabled();

      // Configure a sequence of (G)ood or (B)ad frames
      // to ensure that the process state machine flushes
      // everything correctly.  None of these sequences
      // triggers any stitching (no "GBG" sequences).
      //
      // all of these: inputs should match outputs
      test_process_enabled_no_stitching( "" );
      test_process_enabled_no_stitching( "B" );
      test_process_enabled_no_stitching( "G" );
      test_process_enabled_no_stitching( "GG" );
      test_process_enabled_no_stitching( "GB" );
      test_process_enabled_no_stitching( "BG" );
      test_process_enabled_no_stitching( "BB" );
      test_process_enabled_no_stitching( "BBB" );
      test_process_enabled_no_stitching( "BBG" );
      test_process_enabled_no_stitching( "BGB" );
      test_process_enabled_no_stitching( "GBB" );
      test_process_enabled_no_stitching( "GGGGG" );
      test_process_enabled_no_stitching( "GGGBB" );
      test_process_enabled_no_stitching( "BBGGG" );
      test_process_enabled_no_stitching( "BBGGB" );

      // Check the timeout (well, the frame timeout, anyway) ...
      // A GbbbG sequence should time out with a default of 1 frame
      // and thus return outputs identical to inputs
      test_process_enabled_no_stitching( "GBBBG" );

      // test actual stitching
      test_stitch_identity_across_single_bad_frame("klt");
      test_stitch_motion_across_single_bad_frame("klt");
      test_stitch_motion_across_multi_break("klt");

#ifdef WITH_MAPTK_ENABLED
      test_param_sets("maptk");
      test_stitch_identity_across_single_bad_frame("maptk");
      test_stitch_motion_across_single_bad_frame("maptk");
      test_stitch_motion_across_multi_break("maptk");
#endif
    }
  }
  return testlib_test_summary();
}
