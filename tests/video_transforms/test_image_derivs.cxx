/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <video_transforms/adjoint_image_derivs.h>
#include <video_transforms/adjoint_image_utils.h>
#include <video_transforms/adjoint_image_op.h>

#include <vil/vil_math.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_image_derivs_adjoint()
{
  const unsigned ni = 100, nj = 200;

  typedef adjoint_image_ops_func<double>::func_t func_t;
  func_t forward = forward_gradient<double>;
  func_t backward = backward_divergence<double>;

  adjoint_image_ops_func<double> adj(forward, backward, ni, nj, 1, ni, nj, 2);

  TEST("Forward gradient and backward divergence adjoint (1 plane)",
       true, adj.is_adjoint(1e-6));

  adj.set_src_size(ni, nj, 3);
  adj.set_src_size(ni, nj, 6);

  TEST("Forward gradient and backward divergence adjoint (3 planes)",
       true, adj.is_adjoint(1e-6));
}

void
test_image_derivs_norm()
{
  const unsigned ni = 100, nj = 100, np = 1;

  typedef adjoint_image_ops_func<double>::func_t func_t;
  func_t forward = forward_gradient<double>;
  func_t backward = backward_divergence<double>;

  adjoint_image_ops_func<double> adj(forward, backward, ni, nj, np);

  double norm = adj.norm_estimation(1e-6);
  TEST_NEAR("Gradient norm", norm, vcl_sqrt(8.0), 1e-2);
}


} // end anonymous namespace

int test_image_derivs( int /*argc*/, char */*argv*/[] )
{
  testlib_test_start( "image_derivs" );

  test_image_derivs_adjoint();
  test_image_derivs_norm();

  return testlib_test_summary();
}
