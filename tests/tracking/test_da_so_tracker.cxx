/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_random.h>

#include <testlib/testlib_test.h>

#include <vnl/algo/vnl_symmetric_eigensystem.h>

#include <tracking/da_so_tracker_kalman.h>
#include <tracking/da_so_tracker_kalman_extended.h>

#include <tracking/extended_kalman_functions.h>

#include <tracking/synthetic_path_generator_process.h>
#include <tracking/linear_path.h>
#include <tracking/circular_path.h>

#include "test_util.h"

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

#define TEST_FROB( S, M1, M2 ) TEST_NEAR( S, ((M1) - (M2)).fro_norm(), 0, 1e-6 )
#define TEST_2NORM( S, V1, V2 ) TEST_NEAR( S, ((V1) - (V2)).magnitude(), 0, 1e-6 )

bool
is_symmetric( vnl_matrix<double> const& M, double tol )
{
  bool good = true;
  for( unsigned i = 0; i < M.rows(); ++i )
  {
    for( unsigned j = i; j < M.cols(); ++j )
    {
      if( vcl_fabs( M(i,j) - M(j,i) ) > tol )
      {
        vcl_cout << "Diff at ("<<i<<","<<j<<" = " << vcl_fabs( M(i,j) - M(j,i) ) << "\n";
        good = false;
      }
    }
  }
  return good;
}


void
test_perfect_measurement()
{
  // test with no noise.
  vidtk::da_so_tracker_kalman tracker;

  {
    vnl_double_4 s( 0, 0, 1, 2 );
    vnl_double_4x4 p;
    p.set_identity();
    vnl_double_4x4 q( 0.0 ); // no process noise
    vcl_cout << "q=\n" << q << "\n";
    tracker.set_state( s, p, 10 );
    vidtk::da_so_tracker_kalman::const_parameters_sptr
      params = new vidtk::da_so_tracker_kalman::const_parameters;
    params->process_cov_ = q;
    tracker.set_parameters(params);

    vcl_cout << "Initial state = " << tracker.state() << "\n";
    vcl_cout << "Initial covar =\n" << tracker.state_covar() << "\n";
    TEST_2NORM( "state diff", tracker.state(), vector("0 0 1 2") );
    TEST_FROB( "covar diff", tracker.state_covar(),
               matrix("1 0 0 0; 0 1 0 0; 0 0 1 0; 0 0 0 1") );
  }

  // Predict to 11
  {
    vnl_double_2 l;
    vnl_double_2x2 c;
    tracker.predict( 11, l, c );

    vcl_cout << "t=11 predicted pos = " << l << "\n";
    vcl_cout << "covar =\n" << c << "\n";
    TEST_2NORM( "pos diff", l, vector("1 2") );
    TEST_FROB( "pos covar diff", c,
               matrix("2 0; 0 2") );

    vnl_double_4 x;
    vnl_double_4x4 p;
    tracker.predict( 11, x, p );

    vcl_cout << "t=11 predicted state = " << x << "\n";
    vcl_cout << "covar =\n" << p << "\n";
    TEST_2NORM( "state diff", x, vector("1 2 1 2") );
    TEST_FROB( "covar diff", p,
               matrix("2 0 1 0; 0 2 0 1; 1 0 1 0; 0 1 0 1") );
  }

  // Update at 11
  {
    vnl_double_2 m( 1, 2 );
    vnl_double_2x2 r( 0.0 );
    tracker.update( 11, m, r );

    vcl_cout << "t=11 updated state = " << tracker.state() << "\n";
    vcl_cout << "covar =\n" << tracker.state_covar() << "\n";
    TEST_2NORM( "state diff", tracker.state(), vector("1 2 1 2") );
    TEST_FROB( "covar diff", tracker.state_covar(),
               matrix("0 0 0 0; 0 0 0 0; 0 0 0.5 0; 0 0 0 0.5") );
  }

  // Predict to 13
  {
    vnl_double_2 l;
    vnl_double_2x2 c;
    tracker.predict( 13, l, c );

    vcl_cout << "t=13 predicted pos = " << l << "\n";
    vcl_cout << "covar =\n" << c << "\n";
    TEST_2NORM( "pos diff", l, vector("3 6") );
    TEST_FROB( "pos covar diff", c,
               matrix("2 0; 0 2") );

    vnl_double_4 x;
    vnl_double_4x4 p;
    tracker.predict( 13, x, p );

    vcl_cout << "t=13 predicted state = " << x << "\n";
    vcl_cout << "covar =\n" << p << "\n";
    TEST_2NORM( "state diff", x, vector("3 6 1 2") );
    TEST_FROB( "covar diff", p,
               matrix("2 0 1 0; 0 2 0 1; 1 0 0.5 0; 0 1 0 0.5") );
  }

  // Update at 13
  {
    vnl_double_2 m( 3, 6 );
    vnl_double_2x2 r( 0.0 );
    tracker.update( 13, m, r );

    vcl_cout << "t=13 updated state = " << tracker.state() << "\n";
    vcl_cout << "covar =\n" << tracker.state_covar() << "\n";

    // With two absolutely certain measurements, a constant velocity
    // model is completely specified, and so the state estimate should
    // be absolutely certain.
    TEST_2NORM( "state diff", tracker.state(), vector("3 6 1 2") );
    TEST_FROB( "covar diff", tracker.state_covar(),
               matrix("0 0 0 0; 0 0 0 0; 0 0 0 0; 0 0 0 0") );
  }
}


void
test_symmetric_covar()
{
  // verify that the covariance matrices always remain symmetric as
  // they should.

  vcl_cout << "Test symmetric covar\n";

  vidtk::da_so_tracker_kalman tracker;

  {
    vnl_double_4 s( 12.479479, 5.127005, 0.1690353, 0.660023 );
    vnl_double_4x4 p;
    p.set_identity();
    tracker.set_state( s, p, 0 );
    vidtk::da_so_tracker_kalman::const_parameters_sptr
      params = new vidtk::da_so_tracker_kalman::const_parameters;
    params->process_cov_ = p;
    tracker.set_parameters(params);

    vcl_cout << "State=\n" << tracker.state() << "\n";
    vcl_cout << "State covar=\n" << tracker.state_covar() << "\n";
    TEST( "Initial state", is_symmetric( tracker.state_covar(), 1e-6 ), true );
  }

  vnl_random rand;
  for( unsigned i = 1; i < 1000; ++i )
  {
    vnl_double_2 p;
    p(0) = rand.drand32(0.0, 50.0);
    p(1) = rand.drand32(0.0, 50.0);
    tracker.update( i, p, matrix( "2 0; 0 2" ) );
    vcl_cout << "State=\n" << tracker.state() << "\n";
    vcl_cout << "State covar=\n" << tracker.state_covar() << "\n";
    TEST( "Next step: is symmetric", is_symmetric( tracker.state_covar(), 1e-6 ), true );
  }
}

void test_noisey_points_fixed_process_noise()
{
  vidtk::synthetic_path_generator_process<vidtk::linear_path> generator("generator");
  generator.set_std_dev(1);
  generator.set_number_of_generations(1000);
  generator.set_frame_rate(2.0);
  generator.initialize();

  vnl_double_4x4 init_cov;
  init_cov.fill(0);
  vnl_double_4x4 process_cov;
  process_cov.set_identity();
  process_cov *= 0.0001;
  vidtk::da_so_tracker_kalman tracker;
  vnl_double_4 tracker_state;
  vnl_double_2 nloc, correct;
  generator.current_noisy_location(nloc);
  tracker_state[0] = nloc[0];
  tracker_state[1] = nloc[1];
  tracker_state[2] = 0;
  tracker_state[3] = 1;
  vidtk::timestamp cur_ts = generator.timestamp();
  tracker.set_state( tracker_state, init_cov, cur_ts.time_in_secs() );
  vidtk::da_so_tracker_kalman::const_parameters_sptr
      params = new vidtk::da_so_tracker_kalman::const_parameters;
  params->process_cov_ = process_cov;
  params->process_noise_speed_scale_ = vnl_double_2(1,1);
  params->use_fixed_process_noise_ = true;
  tracker.set_parameters(params);

  double step_count = 0;
  double sum = 0;
  double count = 0;
  vnl_matrix_fixed<double,2,2> ave_pred_cov;
  ave_pred_cov.fill(0);
  while( generator.next_loc(nloc) )
  {
    vidtk::timestamp cur_ts = generator.timestamp();
    generator.current_correct_location(correct);
    vnl_double_2x2 mod_cov;
    mod_cov.set_identity();
    vnl_vector_fixed<double,2> pred_pos;
    vnl_matrix_fixed<double,2,2> pred_cov;
    tracker.predict( cur_ts.time_in_secs(), pred_pos, pred_cov );
    if(step_count++ > 100)
    {
      count++;
      sum += (pred_pos-correct).magnitude();
      ave_pred_cov += pred_cov;
    }
    tracker.update( cur_ts.time_in_secs(), nloc, mod_cov );
  }
  ave_pred_cov = ave_pred_cov/count;

  TEST( "Kalman filter fits nicely to noisy linear path", sum/count < 0.4, true );
  TEST( "Average noisy covar is symmetric", is_symmetric( ave_pred_cov, 1e-6 ), true );
  TEST( "Covar values small", ave_pred_cov(0,0) < 0.2 && ave_pred_cov(0,1) < 0.2 && 
                              ave_pred_cov(1,0) < 0.2 && ave_pred_cov(1,1) < 0.2, true );
}

// vcl_ofstream out("point.txt");

void test_extended_kalman_filter()
{
//   vidtk::synthetic_path_generator_process<vidtk::circular_path> generator("generator");
  vidtk::synthetic_path_generator_process<vidtk::linear_path> generator("generator");
  generator.set_std_dev(1);
  generator.set_number_of_generations(10000);
  generator.set_frame_rate(2.0);
  generator.initialize();

  unsigned int n = 4;
  vnl_matrix<double> init_cov(n,n);
  init_cov.fill(0);
  vnl_matrix<double> process_cov(n,n);
  process_cov.fill(0);
  process_cov(2,2)=0.0005;
  process_cov(3,3)=0.00000009;
  init_cov = process_cov;
  vnl_double_2 nloc;
  vnl_double_2 correct;
  vidtk::extended_kalman_functions::speed_heading_fun fun;
  vidtk::da_so_tracker_kalman_extended<vidtk::extended_kalman_functions::speed_heading_fun>::const_parameters_sptr
    params = new vidtk::da_so_tracker_kalman_extended<vidtk::extended_kalman_functions::speed_heading_fun>::const_parameters;
  params->process_cov_ = process_cov;
  generator.current_noisy_location(nloc);
  vnl_vector<double> tracker_state(n);
  tracker_state[0] = 0;
  tracker_state[1] = 0;
  tracker_state[2] = 0.5;
  tracker_state[3] = 1;
  vidtk::da_so_tracker_kalman_extended<vidtk::extended_kalman_functions::speed_heading_fun> tracker(fun,params);
  vidtk::timestamp cur_ts = generator.timestamp();
  tracker.set_state( tracker_state, init_cov, cur_ts.time_in_secs() );

  double step_count = 0;
  double sum = 0;
  double count = 0;
  vnl_matrix_fixed<double,2,2> ave_pred_cov;
  ave_pred_cov.fill(0);

  vnl_double_2 prev_l;
  while( generator.next_loc(nloc) )
  {
    vidtk::timestamp cur_ts = generator.timestamp();
    generator.current_correct_location(correct);
    vnl_double_2x2 mod_cov;
    mod_cov.set_identity();
    vnl_vector_fixed<double,2> pred_pos;
    vnl_matrix_fixed<double,2,2> pred_cov;
    tracker.predict( cur_ts.time_in_secs(), pred_pos, pred_cov );
    if(step_count++ > 1000)
    {
      count++;
      vnl_double_2 loc(pred_pos[0],pred_pos[1]);
      sum += (loc-correct).magnitude();
//       vcl_cout << (prev_l-loc).magnitude() << vcl_endl;
      prev_l = loc;
//       out << correct[0] << " " << correct[1] << " "
//           << nloc[0] << " " << nloc[1] << " "
//           << loc[0] << " " << loc[1] << vcl_endl;
      ave_pred_cov += pred_cov;
      vnl_double_2x2 current_cov;
      tracker.get_current_location_covar(current_cov);

/*      vcl_cout << "Current: \n" << current_cov << "pred:\n" << pred_cov << vcl_endl << vcl_endl;*/
    }
//     filter.update( cur_ts.time_in_secs(), nloc, mod_cov );
    tracker.update( cur_ts.time_in_secs(), nloc, mod_cov );
  }
  ave_pred_cov = ave_pred_cov/count;
  //vcl_cout << sum/count << vcl_endl;
  TEST( "Extended Kalman filter kits nicely to noisy linear path", sum/count < 0.35, true );
  vnl_double_2x2 current_cov;
  tracker.get_current_location_covar(current_cov);

//   vcl_cout << current_cov << vcl_endl;

  vnl_matrix_fixed<double,2,2> V;
  vnl_vector_fixed<double,2> lambda;
  vnl_symmetric_eigensystem_compute( current_cov.as_ref(), V.as_ref().non_const(), lambda.as_ref().non_const() );

  //double sig_0 = vcl_sqrt( lambda(0) );
  //double sig_1 = vcl_sqrt( lambda(1) );
  vnl_double_2 ax0( V(0,0), V(1,0) );
  vnl_double_2 ax1( V(0,1), V(1,1) );
//   vcl_cout << sig_0 << "\t\t" << ax0 << vcl_endl;
//   vcl_cout << sig_1 << "\t\t" << ax1 << vcl_endl;
  vnl_vector<double> state = tracker.get_current_state();
//   vcl_cout << state << vcl_endl;
//   vcl_cout << vcl_cos(state[3]) << " " << vcl_sin(state[3]) << vcl_endl;
//   vcl_cout << state[2]*0.5 << vcl_endl;
  vnl_double_2 vc;
  tracker.get_current_velocity(vc);
  vc *= 0.5;
//   vcl_cout << generator.get_current_speed() << vcl_endl;

  vnl_double_2 v = generator.get_current_velocity();
  double vdvc =  vcl_sqrt(dot_product(v,vc));
//   vcl_cout << vdvc << vcl_endl;
  TEST( "Final state has simular velocity to synthetic", vdvc <= 4.01 && vdvc >= 3.99, true );
  v.normalize();
  double vda = vcl_abs(dot_product(v,ax1));
  vcl_cout << vda << vcl_endl;
  TEST( "The major axis of the location covariance is near what is expected", vda<=1.0 && vda >= 0.99999, true );
}

// void test_extended_kalman_filter()
// {
// //   vidtk::synthetic_path_generator_process<vidtk::circular_path> generator("generator");
//   vidtk::synthetic_path_generator_process<vidtk::linear_path> generator("generator");
//   generator.set_std_dev(1);
//   generator.set_number_of_generations(10000);
//   generator.set_frame_rate(2.0);
// 
//   unsigned int n = 4;
//   vnl_matrix<double> init_cov(n,n);
//   init_cov.fill(0);
//   vnl_matrix<double> process_cov(n,n);
// //   vnl_matrix<double> process_cov(5,5);
//   process_cov.set_identity();
//   process_cov *= 0.00001;
// //   vidtk::extened_kalman_functions::circular_fun fun;
// //   vidtk::extended_kalman_filter<vidtk::extened_kalman_functions::circular_fun> filter(fun,process_cov);
// //   vidtk::extened_kalman_functions::linear_fun fun;
// //   vidtk::extended_kalman_filter<vidtk::extened_kalman_functions::linear_fun> filter(fun,process_cov);
// //   vidtk::timestamp cur_ts = generator.timestamp();
//   vnl_double_2 nloc;
//   vnl_double_2 correct;
//   vidtk::extended_kalman_functions::speed_heading_fun fun;
//   vidtk::da_so_tracker_kalman_extended<vidtk::extended_kalman_functions::speed_heading_fun>::const_parameters_sptr
//     params = new vidtk::da_so_tracker_kalman_extended<vidtk::extended_kalman_functions::speed_heading_fun>::const_parameters;
//   params->process_cov_ = process_cov;
//   generator.current_noisy_location(nloc);
//   vnl_vector<double> tracker_state(n);
//   tracker_state[0] = 0;
//   tracker_state[1] = 0;
//   tracker_state[2] = 0.5;
//   tracker_state[3] = 1;
// //   tracker_state[4] = 0;
// //   tracker_state[0] = nloc[0];
// //   tracker_state[1] = nloc[1];
// //   tracker_state[2] = 60;
// //   tracker_state[3] = 0;
// //   tracker_state[4] = 0;
//   vidtk::da_so_tracker_kalman_extended<vidtk::extended_kalman_functions::speed_heading_fun> tracker(fun,params);
//   vidtk::timestamp cur_ts = generator.timestamp();
//   tracker.set_state( tracker_state, init_cov, cur_ts.time_in_secs() );
// 
// //   vidtk::da_so_tracker_kalman tracker;
// //   vnl_double_4 tracker_state;
// //   vnl_double_2 nloc, correct;
// //   generator.current_noisy_location(nloc);
// //   tracker_state[0] = nloc[0];
// //   tracker_state[1] = nloc[1];
// //   tracker_state[2] = 0;
// //   tracker_state[3] = 1;
// //   vidtk::timestamp cur_ts = generator.timestamp();
// //   tracker.set_state( tracker_state, init_cov, cur_ts.time_in_secs() );
// //   vidtk::da_so_tracker_kalman::const_parameters_sptr
// //       params = new vidtk::da_so_tracker_kalman::const_parameters;
// //   params->process_cov_ = process_cov;
// //   params->process_noise_speed_scale_ = vnl_double_2(1,1);
// //   params->use_fixed_process_noise_ = true;
// //   tracker.set_parameters(params);
// 
//   double step_count = 0;
//   double sum = 0;
//   double count = 0;
//   vnl_matrix_fixed<double,2,2> ave_pred_cov;
//   ave_pred_cov.fill(0);
// /*  vnl_double_2 nloc;*/
// //   vnl_double_2 correct;
//   while( generator.next_loc(nloc) )
//   {
//     vidtk::timestamp cur_ts = generator.timestamp();
//     generator.current_correct_location(correct);
//     vnl_double_2x2 mod_cov;
//     mod_cov.set_identity();
// //     vnl_vector<double> pred_pos;
// //     vnl_matrix<double> pred_cov;
// //     filter.predict( cur_ts.time_in_secs(), pred_pos, pred_cov );
//     vnl_vector_fixed<double,2> pred_pos;
//     vnl_matrix_fixed<double,2,2> pred_cov;
//     tracker.predict( cur_ts.time_in_secs(), pred_pos, pred_cov );
//     if(step_count++ > 1000)
//     {
//       count++;
//       vnl_double_2 loc(pred_pos[0],pred_pos[1]);
//       sum += (loc-correct).magnitude();
//       vcl_cout << correct[0] << " " << correct[1] << " "
//                << nloc[0] << " " << nloc[1] << " "
//                << loc[0] << " " << loc[1] << vcl_endl;
// //       ave_pred_cov += pred_cov;
//     }
// //     filter.update( cur_ts.time_in_secs(), nloc, mod_cov );
//     tracker.update( cur_ts.time_in_secs(), nloc, mod_cov );
//   }
//   ave_pred_cov = ave_pred_cov/count;
//   vcl_cout << sum/count << vcl_endl;
// 
// }


} // end anonymous namespace

int test_da_so_tracker( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "da_so_tracker" );

  test_perfect_measurement();
  test_symmetric_covar();
  test_noisey_points_fixed_process_noise();
  test_extended_kalman_filter();

  return testlib_test_summary();
}
