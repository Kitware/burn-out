/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include <vcl_iostream.h>
#include <vcl_sstream.h>
#include <vcl_algorithm.h>
#include <vcl_queue.h>

#include <testlib/testlib_test.h>

#include <pipeline/async_pipeline.h>

#include <utilities/homography.h>

#include <tracking/kw18_reader.h>
#include <tracking/tracking_keys.h>
#include <tracking/track_state.h>
#include <tracking/interpolate_track_state_process.h>
#include <tracking/tracking_keys.h>

using vidtk::track_sptr;
using vidtk::track_state_sptr;
using vidtk::image_to_image_homography;
using vidtk::image_to_utm_homography;
using vidtk::timestamp;

namespace {  // anon

static const vcl_string kw18_string =
      "800006 31 20 201.258 124.701 -1.71351 3.60446 535 286 523 263 547 286 1.3249 201.258 124.701 0 1222289806.160\n"
      "800006 31 21 201.27 124.727 -1.71351 3.60446 538.5 287 527 264 550 287 1.20871 201.27 124.727 0 1222289806.260\n"
      "800006 31 22 200.886 124.585 -1.71351 3.60446 538 289 526 265 550 289 1.61246 200.886 124.585 0 1222289806.360\n"
      "800006 31 23 200.645 124.911 -1.71351 3.60446 536 290 524 267 548 290 1.84552 200.645 124.911 0 1222289806.460\n"
      "800006 31 24 200.525 125.208 -1.71351 3.60446 536 290 522 267 550 290 2.09764 200.525 125.208 0 1222289806.560\n"
      "800006 31 25 200.374 125.314 -1.71351 3.60446 537 289 521 266 553 289 2.47602 200.374 125.314 0 1222289806.660\n"
      "800006 31 26 200.589 126.849 -1.71351 3.60446 536 284 521 240 551 284 3.31715 200.589 126.849 0 1222289806.760\n"
      "800006 31 27 200.563 126.999 -1.71351 3.60446 537 284 518 240 556 284 3.63154 200.563 126.999 0 1222289806.860\n"
      "800006 31 28 200.254 127.353 -1.71351 3.60446 533.5 282 516 238 551 282 2.96067 200.254 127.353 0 1222289806.960\n"
      "800006 31 29 200.128 127.512 -1.71351 3.60446 534.5 282 515 238 554 282 2.83199 200.128 127.512 0 1222289807.061\n"
      "800006 31 30 200.004 127.725 -1.71351 3.60446 535 282 512 238 558 282 3.6831 200.004 127.725 0 1222289807.161\n"
      "800006 31 31 199.304 128.308 -1.71351 3.60446 525.5 281 511 260 540 281 1.89959 199.304 128.308 0 1222289807.261\n"
      "800006 31 32 199.125 128.666 -1.71351 3.60446 524 280 510 259 538 280 1.73126 199.125 128.666 0 1222289807.361\n"
      "800006 31 33 199.4 130.238 -1.71351 3.60446 523 275 509 260 537 275 1.26564 199.4 130.238 0 1222289807.461\n"
      "800006 31 34 198.965 130.152 -1.71351 3.60446 521 278 507 261 535 278 1.44295 198.965 130.152 0 1222289807.561\n"
      "800006 31 35 198.685 130.113 -1.71351 3.60446 519.5 279 505 261 534 279 1.64485 198.685 130.113 0 1222289807.661\n"
      "800006 31 36 198.514 130.474 -1.71351 3.60446 523 287 493 236 553 287 1.7884 197.94 128.192 0 1222289807.761\n"
      "800006 31 37 198.342 130.833 -1.71614 3.5882 525 287 495 236 555 287 1.61637 197.903 128.126 0 1222289807.861\n"
      "800006 31 38 198.169 131.183 -1.72089 3.53311 528 287 498 236 558 287 1.69424 197.906 128.124 0 1222289807.961\n"
      "800006 31 39 197.995 131.508 -1.72911 3.40829 528 289 498 238 559 289 2.18933 197.767 128.034 0 1222289808.062\n"
      "800006 31 40 197.817 131.781 -1.74655 3.18261 526 291 495 240 557 291 2.52322 197.524 127.988 0 1222289808.162\n"
      "800006 31 41 197.63 131.968 -1.7806 2.82366 526 293 495 242 558 293 2.53905 197.243 127.888 0 1222289808.262\n"
      "800006 31 42 197.426 132.034 -1.83902 2.32081 524 295 492 244 556 295 2.61971 196.939 127.844 0 1222289808.362\n"
      "800006 31 43 197.211 131.953 -1.9026 1.68812 524 297 492 246 557 297 2.73977 196.796 127.832 0 1222289808.462\n"
      "800006 31 44 196.948 131.697 -2.03217 0.923337 519 299 487 248 552 299 2.05159 196.251 127.582 0 1222289808.562\n"
      "800006 31 45 196.64 131.291 -2.20124 0.114743 515 300 483 249 548 300 1.28568 195.852 127.522 0 1222289808.662\n"
      "800006 31 47 196.081 130.651 -2.36128 -0.779403 511 302 478 251 544 302 2.65646 195.521 127.52 0 1222289808.862\n"
      "800006 31 52 195.099 129.279 -2.18327 -1.65519 515 318 482 267 548 318 2.65646 195.476 127.424 0 1222289809.363\n"
      "800006 31 57 194.67 128.072 -1.73988 -1.9084 509 324 476 273 542 324 2.65646 195.552 127.568 0 1222289809.863\n"
      "800006 31 62 194.519 127.232 -1.34164 -1.84441 500 332 467 281 533 332 2.65646 195.427 127.378 0 1222289810.364\n"
      "800006 31 67 194.587 126.801 -0.974407 -1.60064 496 338 463 287 529 338 2.65646 195.574 127.455 0 1222289810.864\n"
      "800010 33 64 189.662 140.594 1.32657 5.40077 361 292 335 228 387 292 10.7275 189.662 140.594 0 1222289810.564\n"
      "800010 33 65 189.541 140.933 1.32657 5.40077 355.5 292 329 227 382 292 10.8061 189.541 140.933 0 1222289810.664\n"
      "800010 33 66 189.608 141.528 1.32657 5.40077 352.5 290 326 227 379 290 10.2749 189.608 141.528 0 1222289810.764\n"
      "800010 33 67 190.3 143.643 1.32657 5.40077 352.5 284 326 227 379 284 9.37619 190.3 143.643 0 1222289810.864\n"
      "800010 33 68 189.982 142.598 1.32657 5.40077 353 289 324 227 382 289 10.6923 189.982 142.598 0 1222289810.964\n"
      "800010 33 69 190.008 142.964 1.32657 5.40077 353 288 323 227 383 288 10.4665 190.008 142.964 0 1222289811.065\n"
      "800010 33 70 190.379 144.487 1.32657 5.40077 351 285 323 241 379 285 5.96342 190.379 144.487 0 1222289811.165\n"
      "800010 33 71 190.229 144.685 1.32657 5.40077 348 287 321 253 375 287 5.27406 190.229 144.685 0 1222289811.265\n"
      "800010 33 72 190.326 144.33 1.32657 5.40077 351 290 320 231 382 290 10.2421 190.326 144.33 0 1222289811.365\n"
      "800010 33 73 190.174 144.735 1.32657 5.40077 349 288 322 228 376 288 9.53832 190.174 144.735 0 1222289811.465\n"
      "800010 33 74 190.366 145.132 1.32657 5.40077 349.5 286 323 227 376 286 9.16036 190.366 145.132 0 1222289811.565\n"
      "800010 33 75 190.703 146.026 1.32657 5.40077 351 285 324 228 378 285 9.07208 190.703 146.026 0 1222289811.665\n"
      "800010 33 76 190.842 146.647 1.32657 5.40077 351.5 285 326 229 377 285 8.72017 190.842 146.647 0 1222289811.765\n"
      "800010 33 77 190.87 146.957 1.32657 5.40077 351.5 283 326 228 377 283 8.5952 190.87 146.957 0 1222289811.865\n"
      "800010 33 78 191.218 147.901 1.32657 5.40077 352.5 282 327 230 378 282 8.15673 191.218 147.901 0 1222289811.965\n"
      "800010 33 79 191.654 148.703 1.32657 5.40077 356 280 329 229 383 280 8.57201 191.654 148.703 0 1222289812.066\n"
      "800010 33 80 191.787 149.244 1.32657 5.40077 353 294 322 229 385 294 8.26729 190.299 145.362 0 1222289812.166\n"
      "800010 33 81 191.919 149.782 1.3172 5.37596 355 295 324 230 387 295 7.83669 190.359 145.65 0 1222289812.266\n"
      "800010 33 82 192.046 150.307 1.28849 5.29857 355 293 324 228 387 293 7.2615 190.451 146.01 0 1222289812.366\n"
      "800010 33 83 192.162 150.8 1.23391 5.13821 355 291 324 226 387 291 7.53493 190.643 146.338 0 1222289812.466\n"
      "800010 33 84 192.258 151.234 1.14127 4.87167 355 292 324 227 387 292 8.00776 190.7 146.755 0 1222289812.566\n"
      "800010 33 85 192.321 151.574 1.00342 4.46598 355 293 324 227 387 293 8.25954 190.754 146.962 0 1222289812.666\n"
      "800010 33 86 192.342 151.781 0.819226 3.91121 356 293 325 227 388 293 7.8899 190.807 147.158 0 1222289812.766\n"
      "800010 33 87 192.316 151.842 0.600133 3.24265 357 294 326 228 389 294 7.53429 190.889 147.487 0 1222289812.866\n"
      "800010 33 89 192.265 151.96 0.330551 2.40328 357 294 326 228 389 294 3.6396 191.089 148.296 0 1222289813.067\n"
      "800010 33 90 192.074 151.461 0.00966209 1.34583 356 300 325 234 388 300 0.748335 190.763 147.14 0 1222289813.167\n"
      "800010 33 92 191.846 150.915 -0.268534 0.360437 358 299 327 233 390 299 2.71443 190.919 147.631 0 1222289813.367\n"
      "800010 33 95 191.593 150.298 -0.436231 -0.346685 357 303 326 237 389 303 4.11599 191.144 148.404 0 1222289813.667\n"
      "800010 33 96 191.463 149.837 -0.516212 -0.741364 357 302 326 236 389 302 3.89847 191.205 148.565 0 1222289813.767\n"
      "800010 33 99 190.862 148.136 -0.88279 -1.95472 353 319 322 253 385 319 2.29938 189.812 144.664 0 1222289814.068\n"
      "800010 33 100 190.568 147.186 -1.04694 -2.55984 352 319 321 253 384 319 2.78925 189.985 145.036 0 1222289814.168\n"
      "800010 33 101 190.35 146.504 -1.13812 -2.90147 352 319 321 253 384 319 2.757 189.978 145.109 0 1222289814.268\n"
      "800010 33 106 190.042 145.594 -0.946951 -2.50461 351 319 320 253 383 319 2.757 190.548 146.645 0 1222289814.768\n";

struct itsp_input_frame
{
  vcl_vector< track_sptr > tl;
  vidtk::image_to_image_homography src2ref_h;
  vidtk::image_to_utm_homography ref2wld_h;
  vcl_vector< timestamp > tsv;
  double gsd;

private:
  enum {S_NONE = 0x0, S_TL = 0x01, S_S2R = 0x02, S_R2W = 0x04, S_TS = 0x08, S_GSD = 0x10,
        S_ALL = S_TL | S_S2R | S_R2W | S_TS | S_GSD };
  unsigned input_status;

public:

  itsp_input_frame( const vcl_vector< track_sptr >& track_list,
                    const vidtk::image_to_image_homography& s_h,
                    const vidtk::image_to_utm_homography& r_h,
                    const vcl_vector< timestamp >& timestamp_v,
                    double g)
    : tl( track_list ), src2ref_h( s_h ), ref2wld_h( r_h ), tsv( timestamp_v ), gsd( g ), input_status( S_ALL )
  {}

  itsp_input_frame()
    : input_status( S_NONE )
  {}

  void set_tracks( const vcl_vector<track_sptr>& t ) { input_status |= S_TL; tl = t; }
  void set_s2r( const vidtk::image_to_image_homography& h ) { input_status |= S_S2R; src2ref_h = h; }
  void set_r2w( const vidtk::image_to_utm_homography& h ) { input_status |= S_R2W; ref2wld_h = h; }
  void set_ts( const timestamp& ts ) { input_status |= S_TS; tsv.clear(); tsv.push_back(ts); vcl_cerr << "Collector pushed " << ts.frame_number() << "\n";}
  void set_gsd( double g ) { input_status |= S_GSD; gsd = g; }
  bool all_inputs_set() const { return input_status == S_ALL; }
  void clear_input_flag() { input_status = S_NONE; }
};

struct itsp_input_pump
  : public vidtk::process
{
  typedef itsp_input_pump self_type;

  vcl_queue< itsp_input_frame > data_list;
  itsp_input_frame current_data;

  explicit itsp_input_pump( const vcl_string& name )
    : vidtk::process( name, "itsp_input_pump" )
  {}

  vidtk::config_block params() const { return vidtk::config_block(); }
  bool set_params( const vidtk::config_block& ) { return true; }
  bool initialize() { return true; }

  const vcl_vector<track_sptr>& output_track_list() const { return current_data.tl; }
  VIDTK_OUTPUT_PORT( const vcl_vector<track_sptr>&, output_track_list );
  const vidtk::image_to_image_homography& output_src2ref_h() const { return current_data.src2ref_h; }
  VIDTK_OUTPUT_PORT( const vidtk::image_to_image_homography&, output_src2ref_h );
  const vidtk::image_to_utm_homography& output_ref2wld_h() const { return current_data.ref2wld_h; }
  VIDTK_OUTPUT_PORT( const vidtk::image_to_utm_homography&, output_ref2wld_h );
  const vcl_vector< timestamp >& output_tsv() const { return current_data.tsv; }
  VIDTK_OUTPUT_PORT( const vcl_vector< timestamp >&, output_tsv );
  double output_gsd() const { return current_data.gsd; }
  VIDTK_OUTPUT_PORT( double, output_gsd );

  vidtk::process::step_status step2()
  {
    if (this->data_list.empty()) return vidtk::process::FAILURE;
    this->current_data = this->data_list.front();
    this->data_list.pop();
    return vidtk::process::SUCCESS;
  }
};

struct itsp_output_collector
  : public vidtk::process
{
  typedef itsp_output_collector self_type;

  vcl_vector< itsp_input_frame > data_list;
  itsp_input_frame current_data;

  explicit itsp_output_collector( const vcl_string& name )
    : vidtk::process( name, "itsp_output_collector" )
  {}

  vidtk::config_block params() const { return vidtk::config_block(); }
  bool set_params( const vidtk::config_block& ) { return true; }
  bool initialize() { return true; }

  void set_tracks( const vcl_vector<track_sptr>& tl ) { current_data.set_tracks( tl ); }
  VIDTK_OPTIONAL_INPUT_PORT( set_tracks, const vcl_vector<track_sptr>& );
  void set_img2ref_h( const image_to_image_homography& h ) { current_data.set_s2r( h ); }
  VIDTK_OPTIONAL_INPUT_PORT( set_img2ref_h, const image_to_image_homography& );
  void set_ref2wld_h( const image_to_utm_homography& h ) { current_data.set_r2w( h ); }
  VIDTK_OPTIONAL_INPUT_PORT( set_ref2wld_h, const image_to_utm_homography& );
  void set_ts( const timestamp& ts ) { current_data.set_ts( ts ); }
  VIDTK_OPTIONAL_INPUT_PORT( set_ts, const timestamp& );
  void set_gsd( double gsd ) { current_data.set_gsd( gsd ); }
  VIDTK_OPTIONAL_INPUT_PORT( set_gsd, double );

  vidtk::process::step_status step2()
  {
    bool all_data_present = this->current_data.all_inputs_set();
    vcl_cerr << "Collector stepped; all data present? " << all_data_present << "\n";
    this->current_data.clear_input_flag();
    if (all_data_present)
    {
      this->data_list.push_back( this->current_data );
      return vidtk::process::SUCCESS;
    }
    else
    {
      return vidtk::process::FAILURE;
    }
  }
};

struct test_pipeline
{
  vidtk::async_pipeline p;
  vidtk::process_smart_pointer< itsp_input_pump > datasource;
  vidtk::process_smart_pointer< vidtk::interpolate_track_state_process > itsp;
  vidtk::process_smart_pointer< itsp_output_collector > collector;

  test_pipeline()
    : datasource( new itsp_input_pump( "datasource" )),
      itsp( new vidtk::interpolate_track_state_process( "itsp" )),
      collector( new itsp_output_collector( "collector" ))
  {
    p.add( datasource );
    p.add( itsp );
    p.add( collector );

    p.connect( datasource->output_track_list_port(), itsp->set_input_tracks_port() );
    p.connect( datasource->output_src2ref_h_port(), itsp->set_input_img2ref_homography_port() );
    p.connect( datasource->output_ref2wld_h_port(), itsp->set_input_ref2world_homography_port() );
    p.connect( datasource->output_tsv_port(), itsp->set_input_timestamps_port() );
    p.connect( datasource->output_gsd_port(), itsp->set_input_gsd_port() );

    p.connect( itsp->output_track_list_port(), collector->set_tracks_port() );
    p.connect( itsp->output_timestamp_port(), collector->set_ts_port() );
    p.connect( itsp->output_img2ref_homography_port(), collector->set_img2ref_h_port() );
    p.connect( itsp->output_ref2utm_homography_port(), collector->set_ref2wld_h_port() );
    p.connect( itsp->output_gsd_port(), collector->set_gsd_port() );
  }
};

vcl_vector< itsp_input_frame > make_some_data( size_t N )
{
  vcl_vector< itsp_input_frame > ret;
  vgl_h_matrix_2d<double> I;
  I.set_identity();
  vidtk::image_to_image_homography test_src2ref_h;
  test_src2ref_h.set_transform( I );
  vidtk::image_to_utm_homography test_ref2wld_h;
  test_ref2wld_h.set_transform( I );
  for (unsigned i=0; i<N; ++i)
  {
    itsp_input_frame f;
    f.set_tracks( vcl_vector<track_sptr>() );
    f.set_gsd( 0.5 );
    f.set_s2r( test_src2ref_h );
    f.set_r2w( test_ref2wld_h );
    f.set_ts( timestamp( 1000.0+i, 500+i ));
    ret.push_back( f );
  }
  return ret;
}

vcl_vector< itsp_input_frame > compress_to_schedule( const vcl_vector<itsp_input_frame>& data,
                                                     const vcl_vector<unsigned>& schedule )
{
  vcl_vector< itsp_input_frame > ret;

  unsigned sum = 0;
  for (unsigned i=0; i<schedule.size(); ++i) sum += schedule[i];
  TEST( "compress: schedule matches data size", (data.size()==sum), true );
  if (data.size() != sum) return ret;

  unsigned index = 0;
  for (unsigned i=0; i<schedule.size(); ++i)
  {
    itsp_input_frame exemplar = data[index];
    exemplar.tsv.clear();
    for (unsigned j=0; j<schedule[i]; ++j)
    {
      if ( ! data[index].tsv.empty() )
      {
        exemplar.tsv.push_back( data[index].tsv[0] );
      }
      index++;
    }
    ret.push_back( exemplar );
  }
  return ret;
}

bool track_states_are_equal( track_state_sptr lhs, track_state_sptr rhs )
{
  if (( ! lhs ) && ( ! rhs )) return true;
  if (! ( lhs && rhs )) return false;

  bool tmp1 =
    ( lhs->time_ == rhs->time_ ) &&
    ( lhs->loc_ == rhs->loc_ ) &&
    ( lhs->vel_ == rhs->vel_ );
  if ( ! tmp1 ) return false;

  vcl_vector< vidtk::image_object_sptr > lhs_objs, rhs_objs;
  lhs->data_.get( vidtk::tracking_keys::img_objs, lhs_objs );
  rhs->data_.get( vidtk::tracking_keys::img_objs, rhs_objs );
  if ( lhs_objs.size() != rhs_objs.size() ) return false;
  if ( ! lhs_objs.empty() )
  {
    vgl_box_2d<unsigned> lhs_box = lhs_objs[0]->bbox_;
    vgl_box_2d<unsigned> rhs_box = rhs_objs[0]->bbox_;
    if ( ! (lhs_box == rhs_box ) ) return false;
  }
  return true;
}

bool tracks_are_equal( track_sptr lhs, track_sptr rhs )
{
  if (( ! lhs ) && ( ! rhs )) return true;
  if (! ( lhs && rhs )) return false;

  if (lhs->id() != rhs->id()) return false;

  vcl_vector< track_state_sptr > lhs_history( lhs->history() );
  vcl_vector< track_state_sptr > rhs_history( rhs->history() );

  if ( lhs_history.size() != rhs->history().size() ) return false;
  for (size_t i=0; i<lhs_history.size(); ++i)
  {
    if ( ! track_states_are_equal( lhs_history[i], rhs_history[i] )) return false;
  }

  return true;
}

bool tracklists_are_equal( const vcl_vector< track_sptr >& lhs, const vcl_vector< track_sptr >& rhs )
{
  if ( lhs.size() != rhs.size() ) return false;
  for (size_t i = 0; i < lhs.size(); ++i)
  {
    if ( ! tracks_are_equal( lhs[i], rhs[i] )) return false;
  }
  return true;
}

void test_track_tests( vcl_vector< vidtk::track_sptr >& tracks )
{

  TEST( "track #1 reflexively equal", tracks_are_equal( tracks[0], tracks[0] ), true );
  TEST( "track #2 reflexively equal", tracks_are_equal( tracks[1], tracks[1] ), true );
  TEST( "track #1 != track #2", tracks_are_equal( tracks[0], tracks[1]), false );
  TEST( "track #2 != null", tracks_are_equal( 0, tracks[1] ), false );
  TEST( "tracklists reflexively equal", tracklists_are_equal( tracks, tracks ), true );
  if ( tracks.size() > 1 )
  {
    vcl_vector< track_sptr > rev_tracks( tracks );
    vcl_reverse( rev_tracks.begin(), rev_tracks.end() );
    TEST( "tracklists != rev list", tracklists_are_equal( tracks, rev_tracks ), false );
  }
}


void test_process_null_inputs()
{
  vidtk::interpolate_track_state_process itsp( "itsp" );
  TEST( "null inputs: initialize", itsp.initialize(), true );
  // should fail on no inputs
  TEST( "null inputs: step fails", itsp.step2(), vidtk::process::FAILURE );
}

void test_copy_inputs_when_disabled( vcl_vector< track_sptr >& tracks )
{
  vidtk::interpolate_track_state_process itsp( "itsp" );
  TEST( "copy inputs: initialize", itsp.initialize(), true );

  vidtk::config_block params = itsp.params();
  params.set( "enabled", "0" );
  params.set( "tsv_live_index", "3" );
  TEST( "copy inputs: set parms", itsp.set_params(params), true );

  double test_gsd = 0.5;
  vgl_h_matrix_2d<double> I;
  I.set_identity();
  vidtk::image_to_image_homography test_src2ref_h;
  test_src2ref_h.set_transform( I );
  vidtk::image_to_utm_homography test_ref2wld_h;
  test_ref2wld_h.set_transform( I );
  timestamp test_ts( 1000.0, 512 ); // arbitrary timestamp and frame number

  vcl_vector< timestamp > tsv;
  for (unsigned i=0; i<6; ++i)
  {
    tsv.push_back(
      (i == 3)
      ? test_ts
      : timestamp());
  }

  itsp.set_input_tracks( tracks );
  itsp.set_input_img2ref_homography( test_src2ref_h );
  itsp.set_input_ref2world_homography( test_ref2wld_h );
  itsp.set_input_timestamps( tsv );
  itsp.set_input_gsd( test_gsd );

  TEST( "copy inputs: step succeeds", itsp.step2(), vidtk::process::SUCCESS );

  TEST( "copy inputs: tracks equal", tracklists_are_equal( tracks, itsp.output_track_list() ), true );
  TEST( "copy inputs: img2ref equal", (itsp.output_img2ref_homography().get_transform() == I), true );
  TEST( "copy inputs: ref2wld equal", (itsp.output_ref2utm_homography().get_transform() == I), true );
  TEST( "copy inputs: gsd equal", (itsp.output_gsd() == test_gsd ), true );

  TEST( "copy inputs: next step fails", itsp.step2(), vidtk::process::FAILURE );
}



void test_upsampling( const vcl_string& test_name, vcl_vector<itsp_input_frame>& data, vcl_vector<unsigned>& schedule )
{
  test_pipeline tpl;

  // test a single frame to make sure it gets upsampled into three
  // this test works because empty track lists are "complete"

  vcl_vector<itsp_input_frame> input_data =
    compress_to_schedule( data, schedule );
  if (input_data.empty()) return;

  for (unsigned i=0; i<input_data.size(); ++i)
  {
    tpl.datasource->data_list.push( input_data[i] );
  }

  unsigned int N = data.size();
  double test_gsd = data[0].gsd;

#define TSTR(x) vcl_string( test_name+" "+oss.str()+x ).c_str()
  {
    vcl_ostringstream oss;
    TEST( TSTR("set params"), tpl.p.set_params( tpl.p.params().set_value( "itsp:enabled", "1") ), true );
    TEST( TSTR("pipeline init"), tpl.p.initialize(), true );
    bool run_status = tpl.p.run();
    TEST( TSTR("pipeline run"), run_status, true );
  }
  {
    vcl_ostringstream oss;
    oss << "output size " << tpl.collector->data_list.size() << " (should be " << N << ")";
    TEST( TSTR(""), tpl.collector->data_list.size(), N );
  }
  for (unsigned i=0; i<tpl.collector->data_list.size(); ++i)
  {
    const itsp_input_frame& f = tpl.collector->data_list[i];
    {
      vcl_ostringstream oss;
      oss << "output " << i << ": ";
      TEST( TSTR("no tracks"), f.tl.empty(), true );
      TEST( TSTR("src2ref is identity"), f.src2ref_h.get_transform().get_matrix().is_identity(), true );
      TEST( TSTR("ref2wld is identity"), f.ref2wld_h.get_transform().get_matrix().is_identity(), true );
      TEST( TSTR("gsd is correct"), (f.gsd == test_gsd), true );
      TEST( TSTR("tsv has one entry"), f.tsv.size(), 1 );
    }
    if (f.tsv.size() == 1)
    {
      vcl_ostringstream oss;
      oss << "output " << i << ": "
           << "ts " << f.tsv[0] << " matches " << data[i].tsv[0];
      TEST( TSTR(""), (f.tsv[0] == data[i].tsv[0]), true );
    }
    else
    {
      vcl_cerr << test_name << ": output " << i << ": tsv has " << f.tsv.size() << " values\n";
    }
  }
#undef TSTR
}

void test_empty_tsv()
{
  vcl_vector< itsp_input_frame > data = make_some_data( 7 );
  data[2].tsv.clear();
  vcl_vector< unsigned > schedule;
  schedule.push_back( 2 );
  schedule.push_back( 2 );
  schedule.push_back( 3 );
  test_upsampling( "empty tsv", data, schedule );
}

void test_upsampling_1()
{
  vcl_vector< itsp_input_frame > data = make_some_data( 7 );
  vcl_vector< unsigned > schedule;
  schedule.push_back( 1 );
  schedule.push_back( 6 );
  test_upsampling( "upsample 1-6", data, schedule );
  schedule.clear();
  schedule.push_back( 2 );
  schedule.push_back( 2 );
  schedule.push_back( 3 );
  test_upsampling( "upsample 2-2-3", data, schedule );
  schedule.clear();
  schedule.push_back( 1 );
  schedule.push_back( 1 );
  schedule.push_back( 1 );
  schedule.push_back( 1 );
  schedule.push_back( 1 );
  schedule.push_back( 1 );
  schedule.push_back( 1 );
  test_upsampling( "upsample 1x7", data, schedule );
  schedule.clear();
  schedule.push_back( 6 );
  schedule.push_back( 1 );
  test_upsampling( "upsample 6-1", data, schedule );
}

void test_homography_interpolation()
{
  double h1[] = {-0.588730094125, -0.0330450194316, 16.1224707046,0.0178854655981, -0.597643975479, 1.87581193325,-1.79970134434e-07, 6.32137358906e-08, -0.575434605978}; // frame 74
  double h2[] = { 0.590149961143, 0.0368862778079, -17.6276995688,-0.0199891616362, 0.599338527046, -2.46033266601,7.20702948115e-07, 1.10321987127e-06, 0.574916891868}; // frame 80
  double rw[] = { -0.131147257914, -3.63447192869, -182325.796834,  -0.769227417898, -23.5218963071, -1180434.85663,  -1.89896201038e-07, -5.81164614593e-06, -0.291214772868};

  test_pipeline tpl;

  vidtk::image_to_utm_homography r2w;
  r2w.set_transform( vgl_h_matrix_2d<double>( rw ));

  {
    // set up packet 1
    vidtk::image_to_image_homography s2r;
    s2r.set_transform( vgl_h_matrix_2d<double>( h1 ));
    itsp_input_frame f;
    f.set_tracks( vcl_vector<track_sptr>() );
    f.set_gsd(0.5);
    f.set_s2r( s2r );
    f.set_r2w( r2w );
    f.tsv.push_back( timestamp( 1000.0, 74 ));
    f.tsv.push_back( timestamp( 1001.0, 75 ));
    f.tsv.push_back( timestamp( 1002.0, 76 ));
    f.tsv.push_back( timestamp( 1003.0, 77 ));
    f.tsv.push_back( timestamp( 1004.0, 78 ));
    f.tsv.push_back( timestamp( 1005.0, 79 ));
    tpl.datasource->data_list.push( f );
  }
  {
    // set up packet 2
    vidtk::image_to_image_homography s2r;
    s2r.set_transform( vgl_h_matrix_2d<double>( h2 ));
    itsp_input_frame f;
    f.set_tracks( vcl_vector<track_sptr>() );
    f.set_gsd(0.5);
    f.set_s2r( s2r );
    f.set_r2w( r2w );
    f.tsv.push_back( timestamp( 1006.0, 80 ));
    tpl.datasource->data_list.push( f );
  }

  TEST( "test_hi: set params", tpl.p.set_params( tpl.p.params()), true );
  TEST( "test_hi: pipeline init", tpl.p.initialize(), true );
  bool run_status = tpl.p.run();
  TEST( "test_hi: pipeline run", run_status, true );
  {
    vcl_ostringstream oss;
    oss << "test_hi: output size " << tpl.collector->data_list.size() << " (should be 7)";
    TEST( oss.str().c_str(), tpl.collector->data_list.size(), 7 );
  }
  for (unsigned i=0; i<tpl.collector->data_list.size(); ++i)
  {
    vcl_cout << "Output " << i << ":" << vcl_endl;
    const itsp_input_frame& f = tpl.collector->data_list[i];
    vcl_cout << "... ts " << f.tsv[0] << vcl_endl;
    vcl_cout << "... s2r:\n" << f.src2ref_h << vcl_endl;
  }
}

void test_track_upsampling()
{
  vcl_string kw18_string =
    "317 5 0  100 100   10 10  100 100  100 100 110 110  100  100 100 0  1000.0\n"
    "317 5 3  130 130   10 10  130 130  130 130 140 140  100  130 130 0  1003.0\n"
    "317 5 6  160 160   10 10  160 160  160 160 170 170  100  160 160 0  1006.0\n"
    "317 5 9  190 190   10 10  190 190  190 190 200 200  100  190 190 0  1009.0\n"
    "317 5 12  220 220  10 10  220 220  220 220 230 230  100  220 220 0  1012.0\n";
  const unsigned nframes = 5;
  const unsigned ts_skip = 3;
  const unsigned expected_final_frames = nframes * ts_skip;

  vcl_istringstream iss( kw18_string );
  vcl_vector< vidtk::track_sptr > tracks;
  vidtk::kw18_reader reader( iss );
  reader.read( tracks );
  TEST( "Track-upsample: got the test track", ((tracks.size() == 1) && (tracks[0]->history().size() == nframes)), true );

  test_pipeline tpl;

  vgl_h_matrix_2d<double> I;
  I.set_identity();
  vidtk::image_to_image_homography s2r;
  s2r.set_transform( I );
  vidtk::image_to_utm_homography r2w;
  r2w.set_transform( I );

  // one packet for each track frame
  const vcl_vector< vidtk::track_state_sptr >& src_h = tracks[0]->history();
  for (unsigned i=0; i<nframes; ++i)
  {
    // clone the track but trim the history up to i'th frame
    vidtk::track_sptr trk = tracks[0]->clone();
    vcl_vector< vidtk::track_state_sptr > h;
    for (unsigned j=0; j<= i; ++j)
    {
      log_assert( j < src_h.size(), "ran out of track history?");
      h.push_back( src_h[j] );
    }
    trk->reset_history( h );

    itsp_input_frame f;
    f.set_tracks( vcl_vector<track_sptr>( 1, trk ));
    f.set_gsd( 0.5 );
    f.set_s2r( s2r );
    f.set_r2w( r2w );
    f.tsv.push_back( timestamp( 1000.0 + (i*ts_skip) + 0,  (i*ts_skip) + 0 ));
    f.tsv.push_back( timestamp( 1000.0 + (i*ts_skip) + 1,  (i*ts_skip) + 1 ));
    f.tsv.push_back( timestamp( 1000.0 + (i*ts_skip) + 2,  (i*ts_skip) + 2 ));
    tpl.datasource->data_list.push( f );
  } // for all frames

  TEST( "Track-upsample: set params", tpl.p.set_params( tpl.p.params()), true );
  TEST( "Track-upsample: pipeline init", tpl.p.initialize(), true );
  bool run_status = tpl.p.run();
  TEST( "Track-upsample: pipeline run", run_status, true );
  {
    vcl_ostringstream oss;
    oss << "Track-upsample: output size " << tpl.collector->data_list.size() << " (should be " << expected_final_frames << ")";
    TEST( oss.str().c_str(), tpl.collector->data_list.size(), expected_final_frames );
  }
  for (unsigned i=0; i<tpl.collector->data_list.size(); ++i)
  {
    vcl_cout << "Output " << i << ":" << vcl_endl;
    const itsp_input_frame& f = tpl.collector->data_list[i];
    vcl_cout << "... ts " << f.tsv[0] << vcl_endl;
    vcl_cout << "... trk-size " << f.tl.size() << vcl_endl;
    for (unsigned j=0; j<f.tl.size(); ++j)
    {
      vcl_vector< vidtk::image_object_sptr > objs;
      if ( ! f.tl[j]->last_state()->data_.get( vidtk::tracking_keys::img_objs, objs ))
      {
        vcl_cout << "...index " << j << ": track " << f.tl[j]->id() << " has no bbox" << vcl_endl;
      }
      else
      {
        vgl_box_2d<unsigned> box = objs[0]->bbox_;
        vcl_cout << "...index " << j << ": track " << f.tl[j]->id() << " bbox " << box << vcl_endl;
      }
    }
  }
}

void test_track_upsampling_2()
{
  vcl_string kw18_string =
    "318 5 1  100 100   10 10  100 100  100 100 110 110  100  100 100 0  1000.0\n"
    "318 5 4  130 130   10 10  130 130  130 130 140 140  100  130 130 0  1003.0\n"
    "318 5 7  160 160   10 10  160 160  160 160 170 170  100  160 160 0  1006.0\n"
    "318 5 10  190 190   10 10  190 190  190 190 200 200  100  190 190 0  1009.0\n"
    "318 5 12  220 220  10 10  220 220  220 220 230 230  100  220 220 0  1012.0\n";
  const unsigned nframes = 5;
  const unsigned ts_skip = 3;
  const unsigned expected_final_frames = nframes * ts_skip;

  vcl_istringstream iss( kw18_string );
  vcl_vector< vidtk::track_sptr > tracks;
  vidtk::kw18_reader reader( iss );
  reader.read( tracks );
  TEST( "Track-upsample: got the test track", ((tracks.size() == 1) && (tracks[0]->history().size() == nframes)), true );

  test_pipeline tpl;

  vgl_h_matrix_2d<double> I;
  I.set_identity();
  vidtk::image_to_image_homography s2r;
  s2r.set_transform( I );
  vidtk::image_to_utm_homography r2w;
  r2w.set_transform( I );

  // one packet for each track frame
  const vcl_vector< vidtk::track_state_sptr >& src_h = tracks[0]->history();
  for (unsigned i=0; i<nframes; ++i)
  {
    // clone the track but trim the history up to i'th frame
    vidtk::track_sptr trk = tracks[0]->clone();
    vcl_vector< vidtk::track_state_sptr > h;
    for (unsigned j=0; j<= i; ++j)
    {
      log_assert( j < src_h.size(), "ran out of track history?");
      h.push_back( src_h[j] );
    }
    trk->reset_history( h );

    itsp_input_frame f;
    f.set_tracks( vcl_vector<track_sptr>( 1, trk ));
    f.set_gsd( 0.5 );
    f.set_s2r( s2r );
    f.set_r2w( r2w );
    f.tsv.push_back( timestamp( 1000.0 + (i*ts_skip) + 0,  (i*ts_skip) + 0 ));
    f.tsv.push_back( timestamp( 1000.0 + (i*ts_skip) + 1,  (i*ts_skip) + 1 ));
    f.tsv.push_back( timestamp( 1000.0 + (i*ts_skip) + 2,  (i*ts_skip) + 2 ));
    tpl.datasource->data_list.push( f );
  } // for all frames

  TEST( "Track-upsample: set params", tpl.p.set_params( tpl.p.params().set_value("itsp:tsv_live_index", "1")), true );
  TEST( "Track-upsample: pipeline init", tpl.p.initialize(), true );
  bool run_status = tpl.p.run();
  TEST( "Track-upsample: pipeline run", run_status, true );
  {
    vcl_ostringstream oss;
    oss << "Track-upsample: output size " << tpl.collector->data_list.size() << " (should be " << expected_final_frames << ")";
    TEST( oss.str().c_str(), tpl.collector->data_list.size(), expected_final_frames );
  }
  for (unsigned i=0; i<tpl.collector->data_list.size(); ++i)
  {
    vcl_cout << "Output " << i << ":" << vcl_endl;
    const itsp_input_frame& f = tpl.collector->data_list[i];
    vcl_cout << "... ts " << f.tsv[0] << vcl_endl;
    vcl_cout << "... trk-size " << f.tl.size() << vcl_endl;
    for (unsigned j=0; j<f.tl.size(); ++j)
    {
      vcl_vector< vidtk::image_object_sptr > objs;
      if ( ! f.tl[j]->last_state()->data_.get( vidtk::tracking_keys::img_objs, objs ))
      {
        vcl_cout << "...index " << j << ": track " << f.tl[j]->id() << " has no bbox" << vcl_endl;
      }
      else
      {
        vgl_box_2d<unsigned> box = objs[0]->bbox_;
        vcl_cout << "...index " << j << ": track " << f.tl[j]->id() << " bbox " << box << vcl_endl;
      }
    }
  }
}


} // anon namespace

int test_interpolate_track_state_process( int argc, char* argv[] )
{
  testlib_test_start( "Track state interpolation" );

  vcl_istringstream iss( kw18_string );
  vcl_vector< vidtk::track_sptr > tracks;
  vidtk::kw18_reader reader( iss );
  reader.read( tracks );

  test_track_tests( tracks );
  test_process_null_inputs();
  test_copy_inputs_when_disabled( tracks );
  // this merely checks that it doesn't coredump  test_empty_tsv();
  test_upsampling_1();
  test_homography_interpolation();
  test_track_upsampling();
  test_track_upsampling_2();

  return testlib_test_summary();
}
