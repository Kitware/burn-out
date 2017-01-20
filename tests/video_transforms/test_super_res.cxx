/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_math.h>
#include <vil/vil_convert.h>
#include <vil/vil_crop.h>

#include <testlib/testlib_test.h>

#include <video_transforms/super_res.h>
#include <video_transforms/adjoint_dbw.h>
#include <video_transforms/adjoint_flow_warp.h>

#include <boost/make_shared.hpp>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

bool callback_called = false;

//Create the warps for downsampled images
vcl_vector<adjoint_image_ops_func<double> >
create_warps(int sni, int snj,
             unsigned int scale_factor,
             bool down_sample_averaging,
             bool bicubic_warping)
{
  int dni = sni / scale_factor;
  int dnj = snj / scale_factor;

  vcl_vector<adjoint_image_ops_func<double> > warps;

  double sigma = 0.25 * sqrt(scale_factor * scale_factor - 1.0);

  for (unsigned int i = 0; i < scale_factor; i++)
  {
    for (unsigned int j = 0; j < scale_factor; j++)
    {
      vil_image_view<double> flow(sni, snj, 2);
      vil_plane<double>(flow, 0).fill(static_cast<double>(i));
      vil_plane<double>(flow, 1).fill(static_cast<double>(j));

      warps.push_back(create_dbw_from_flow(flow, dni, dnj, 1, scale_factor, sigma,
                                           down_sample_averaging, bicubic_warping));
    }
  }

  return warps;
}

//Create downsampled frames from the warps
//that we will use to reconstruct to the original image
void
create_downsampled_frames(const vil_image_view<double> &high_res,
                          const vcl_vector<adjoint_image_ops_func<double> > &warps,
                          vcl_vector<vil_image_view<double> > &frames)
{
  frames.clear();
  for (unsigned i = 0; i< warps.size(); ++i)
  {
    vil_image_view<double> frame(warps[i].dst_ni(),
                                 warps[i].dst_nj(),
                                 warps[i].dst_nplanes());
    warps[i].apply_A(high_res, frame);
    frames.push_back(frame);
  }
}

void test_super_res(const vil_image_view<double> &img,
                    const super_res_params &srp,
                    unsigned int iterations,
                    bool down_sample_averaging,
                    bool bicubic_warping,
                    double eps,
                    unsigned int margin,
                    super_resolution_monitor *srm)
{
  //Creating downsampled frames
  vcl_vector<vil_image_view<double> > frames;

  vcl_vector<adjoint_image_ops_func<double> > warps;
  warps = create_warps(img.ni(), img.nj(), static_cast<unsigned int>(srp.scale_factor),
                      down_sample_averaging, bicubic_warping);
  create_downsampled_frames(img, warps, frames);

  vcl_cout << "Computing super resolution\n";

  vil_image_view<double> super_u;
  super_resolve(frames, warps, super_u, iterations, srp, srm);


  //The way the dataset is made produces edge artifacts, so only score the image in the center
  vil_image_view<double> super_u_crop = vil_crop(super_u, margin, super_u.ni() - 2 * margin,
                                                 margin, super_u.nj() - 2 * margin);
  vil_image_view<double> img_crop = vil_crop(img, margin, img.ni() - 2 * margin,
                                             margin, img.nj() - 2 * margin);

  double diff = vil_math_image_abs_difference(super_u_crop, img_crop);
  diff /= img.ni() * img.nj();

  TEST_NEAR( "Super Resolved Image", diff, 0.0, eps);
  TEST( "Callback Called", callback_called, srm != NULL);
  callback_called = false;
}

} // end anonymous namespace

void super_res_callback(super_resolution_monitor::update_data)
{
  callback_called = true;
}


int test_super_res( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    vcl_cerr << "Need the data directory as an argument!\n";
    return EXIT_FAILURE;
  }

  // load the input test image
  vcl_string dir = argv[1];
  vcl_string src_path = dir + "/ocean_city.png";
  vil_image_view<vxl_byte> input = vil_load(src_path.c_str());
  vil_image_view<double> dbl_input;
  vil_convert_cast<vxl_byte, double>(input, dbl_input);
  vil_image_view<double> img = vil_crop(dbl_input, 128, 60, 218, 60);
  vil_math_scale_values(img, 1.0/255.0);

  testlib_test_start( "super_res" );

  //Set parameters for the test
  super_res_params srp;
  srp.scale_factor = 2.0;
  srp.s_ni = img.ni();
  srp.s_nj = img.nj();
  srp.l_ni = static_cast<unsigned int>(srp.s_ni/srp.scale_factor);
  srp.l_nj = static_cast<unsigned int>(srp.s_nj/srp.scale_factor);
  srp.lambda = 0.005;
  srp.epsilon_data = 0.01;
  srp.epsilon_reg = 0.04;
  srp.sigma = 0.35;
  srp.tau = 0.35;

  boost::shared_ptr<bool> interrupt = boost::make_shared<bool>();
  super_resolution_monitor *srm = new super_resolution_monitor(super_res_callback, 100, interrupt);

  test_super_res(img, srp, 800, false, false, 0.01, 2, srm);
  test_super_res(img, srp, 800, true, true, 0.01, 8, NULL);

  delete srm;

  return testlib_test_summary();
}
