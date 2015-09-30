/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <vnl/vnl_inverse.h>
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

#include <tracking/wh_tracker_kalman.h>

//#include "test_util.h"

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{
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
test_one_step()
{
  unsigned int curr_time = 10;
  unsigned int next_time = 15;

  vnl_double_4x4 q(0.0);// process noise
  vnl_double_2x2 r(0.0);// observation noise
  q(2,2)=q(3,3)=0.1;
  r(0,0)=r(1,1)=0.1;

  vidtk::wh_tracker_kalman::const_parameters_sptr
      params = new vidtk::wh_tracker_kalman::const_parameters;
    params->process_cov_ = q;

  vidtk::wh_tracker_kalman wh_tracker(params);
  {
    vnl_double_4 x( 10, 10, 0, 0 );// init state of w and h, and their speeds
    vnl_double_4x4 p;// init covariance matrix
    p.set_identity();
    vcl_cout << "q=\n" << q << "\n";
    wh_tracker.set_state( x, p, curr_time );

    vcl_cout << "Initial state = " << wh_tracker.state() << "\n";
    vcl_cout << "Initial covar =\n" << wh_tracker.state_covar() << "\n";


    //// Predict to next_time
    vnl_double_4 pred_x;
    vnl_double_4x4 pred_cov;
    wh_tracker.predict( next_time, pred_x, pred_cov );

    vcl_cout << "t="<<next_time<<" predicted state = " << pred_x << "\n";
    vcl_cout << "covar =\n" << pred_cov << "\n";

    // expected results
    vnl_double_4x4 F; // trans matrix
    F.set_identity();
    F(0,2)=F(1,3)=next_time-curr_time;
    vnl_double_4 expected_state = F*x;
    vnl_double_4x4 expected_cov = F*p*F.transpose() + q;

    TEST_2NORM( "state diff", pred_x, expected_state );
    TEST_FROB( "covar diff", pred_cov, expected_cov );

    //// Update at next_time
    vnl_double_2 m(10,10);
    wh_tracker.update( next_time, m, r );

    vcl_cout << "t="<<next_time<<" updated state = " << wh_tracker.state() << "\n";
    vcl_cout << "covar =\n" << wh_tracker.state_covar() << "\n";

    // expected results
    vnl_matrix_fixed<double,4,2> K; //Kalman gain
    vnl_double_4x4 I;
    vnl_matrix_fixed<double,2,4> H;
    H.fill(0.0);
    H(0,0) = H(1,1) = 1.0;
    I.set_identity();
    vnl_double_2x2 temp = H * pred_cov * H.transpose() + r;
    K=pred_cov * H.transpose() * vnl_inverse(temp);

    expected_state = pred_x + K*(m - H * pred_x);
    expected_cov = (I - K * H) * pred_cov;

    TEST_2NORM( "state diff", wh_tracker.state(), expected_state );
    TEST_FROB( "covar diff", wh_tracker.state_covar(), expected_cov );
  }
}


void
test_symmetric_covar()
{
  // verify that the covariance matrices always remain symmetric as
  // they should.

  vcl_cout << "Test symmetric covar\n";

  vnl_double_4x4 q(0.0);// process noise
  vnl_double_2x2 r(0.0);// observation noise
  q(2,2)=q(3,3)=0.1;
  r(0,0)=r(1,1)=0.1;

  vidtk::wh_tracker_kalman::const_parameters_sptr
      params = new vidtk::wh_tracker_kalman::const_parameters;
    params->process_cov_ = q;

  vidtk::wh_tracker_kalman wh_tracker(params);
  {
    vnl_double_4 s( 10., 10., 0., 0. );
    vnl_double_4x4 p;
    p.set_identity();
    wh_tracker.set_state( s, p, 0 );

    vcl_cout << "State=\n" << wh_tracker.state() << "\n";
    vcl_cout << "State covar=\n" << wh_tracker.state_covar() << "\n";
    TEST( "Initial state", is_symmetric( wh_tracker.state_covar(), 1e-6 ), true );
  }

  vnl_random rand;
  for( unsigned i = 1; i < 1000; ++i )
  {
    vnl_double_2 wh;
    wh(0) = rand.drand32(9.0, 11.0);
    wh(1) = rand.drand32(9.0, 11.0);
    wh_tracker.update( i, wh, r );
    vcl_cout << "State=\n" << wh_tracker.state() << "\n";
    vcl_cout << "State covar=\n" << wh_tracker.state_covar() << "\n";
    TEST( "Next step: is symmetric", is_symmetric( wh_tracker.state_covar(), 1e-6 ), true );
  }
}

void test_noisey_points_fixed_process_noise()
{
  vnl_double_4x4 init_cov;
  init_cov.fill(0);
  vnl_double_4x4 process_cov;
  process_cov.set_identity();
  process_cov *= 0.0001;

  vnl_double_2x2 r(0.0);// observation noise
  r(0,0)=r(1,1)=0.01;


  //vidtk::da_so_tracker_kalman tracker;

  vnl_double_4 tracker_state;
  vnl_double_2 wh;
  vnl_double_2 correct(10., 10.);
  tracker_state[0] = correct[0];
  tracker_state[1] = correct[1];
  tracker_state[2] = 0;
  tracker_state[3] = 0;

  vidtk::wh_tracker_kalman::const_parameters_sptr
      params = new vidtk::wh_tracker_kalman::const_parameters;
  params->process_cov_ = process_cov;

  vidtk::wh_tracker_kalman wh_tracker(params);

  wh_tracker.set_state( tracker_state, init_cov, 0 );

  double step_count = 0;
  double sum = 0;
  double count = 0;
  vnl_matrix_fixed<double,2,2> ave_pred_cov;
  ave_pred_cov.fill(0);


  vnl_random rand;
  for(int i=1;i<1000;i++)
  {
    wh[0]=rand.drand32(9.99, 10.01);
    wh[1]=rand.drand32(9.99, 10.01);

    vnl_vector_fixed<double,2> pred_pos;
    vnl_matrix_fixed<double,2,2> pred_cov;
    wh_tracker.predict( i, pred_pos, pred_cov );
    if(step_count++ > 100)
    {
      count++;
      sum += (pred_pos-correct).magnitude();
      ave_pred_cov += pred_cov;
    }
    wh_tracker.update( i, wh, r );
  }
  ave_pred_cov = ave_pred_cov/count;

  TEST( "Kalman filter fits nicely to noisy linear path", sum/count < 0.4, true );
  TEST( "Average noisy covar is symmetric", is_symmetric( ave_pred_cov, 1e-6 ), true );
  TEST( "Covar values small", ave_pred_cov(0,0) < 0.2 && ave_pred_cov(0,1) < 0.2 && 
                              ave_pred_cov(1,0) < 0.2 && ave_pred_cov(1,1) < 0.2, true );
}

} // end namespace


int test_track_bbox_smoothing( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "track_bbox_smooth" );

  test_one_step();
  test_symmetric_covar();
  test_noisey_points_fixed_process_noise();

  return testlib_test_summary();
}
