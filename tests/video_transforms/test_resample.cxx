/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>

#include <vil/vil_math.h>
#include <boost/bind.hpp>

#include <video_transforms/adjoint_resample.h>
#include <video_transforms/adjoint_image_utils.h>
#include <video_transforms/adjoint_image_op.h>

#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void
test_resampler()
{
  const unsigned ni = 100, nj = 200;
  const unsigned scale = 2;

  typedef adjoint_image_ops_func<float>::func_t func_t;
  func_t forward = boost::bind(down_sample<float>, _1, _2, scale, 0, 0);
  func_t backward = boost::bind(up_sample<float>, _1, _2, scale, 0, 0);

  adjoint_image_ops_func<float> adj(forward, backward, ni, nj, 3);

  TEST("Down sampling and up sampling adjoint", true, adj.is_adjoint());
}

void
test_rescaler()
{
  const unsigned ni = 100, nj = 200;
  vil_image_view<float> I0(ni, nj, 3), I1(ni/2, nj/2, 3);
  fill_random(I0, 0.0f, 255.0f);
  fill_random(I1, 0.0f, 255.0f);

  vil_image_view<float> d_I0, u_I1;
  down_scale(I0, d_I0, 2);
  up_scale(I1, u_I1, 2);

  double sum1 = dot_product(I0, u_I1);
  double sum2 = dot_product(I1, d_I0);
  double relative_error = vcl_abs(sum1 - sum2) / vcl_abs(sum1);

  TEST_NEAR("down scaling and up scaling adjoint", relative_error, 0.0, 1e-8);
}


} // end anonymous namespace

int
test_resample( int /*argc*/, char */*argv*/[] )
{
  testlib_test_start( "resample" );
  test_resampler();
  test_rescaler();

  return testlib_test_summary();
}
