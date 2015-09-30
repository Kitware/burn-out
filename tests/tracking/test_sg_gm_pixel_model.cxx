/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>

#include <vnl/vnl_random.h>

#include <tracking/sg_gm_pixel_model.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
test_foreground_detection_and_transition()
{
  vcl_cout << "Test foreground detection and transition to background\n";

  vidtk::sg_gm_pixel_model_params * param = new vidtk::sg_gm_pixel_model_params();
  param->mahan_dist_sqr_thres_ = 2.5 * 2.5;
  float init_sig = 10;
  param->initial_variance_ = init_sig * init_sig;
  param->initial_weight_ = 0.01f;
  param->alpha_weights_ = 0.01f;
  param->alpha_means_ = 0.01f;
  param->alpha_variance_ = 0.01f;
  param->learning_alpha_weights_ = 0.01f;
  param->learning_alpha_means_ = 0.01f;
  param->learning_alpha_variance_ = 0.01f;
  param->rho_mean_ = -1;
  param->rho_variance_ = -1;
  param->background_thres_ = 0.9f;
  param->order_by_weight_only_ = false;
  param->num_mixture_comp_ = 2;
  param->min_samples_for_detection_ = 0;
  param->no_update_threshold_ = -1;
  param->num_pix_comp_ = 3;

  vidtk::sg_gm_pixel_model gmm(param);
  float values[] = {0,0,0};

  for(unsigned int i = 0; i < 100; ++i)
  {
    if(i == 70)
    {
      values[0] = 255;
      values[1] = 255;
      values[2] = 255;
    }

    bool foreground = gmm.update(values);
    if(i < 70)
    {
      TEST( "First 70 are background", foreground, false );
      //vcl_cout << "   foreground" << vcl_endl;
    }
    else if( i < 80 )
    {
      TEST( "70-79 should be foreground", foreground, true );
    }
    else
    {
      TEST( "Rest should be learnt as background", foreground, false);
    }
  }

  delete param;

}

void test_weight_initialization()
{
  vcl_cout << "Test the initialization of the weight parameter\n";
  vidtk::sg_gm_pixel_model_params * param = new vidtk::sg_gm_pixel_model_params();
  param->mahan_dist_sqr_thres_ = 2.5 * 2.5;
  float init_sig = 10;
  param->initial_variance_ = init_sig * init_sig;
  param->initial_weight_ = 0.01f;
  param->alpha_weights_ = 0.01f;
  param->alpha_means_ = 0.01f;
  param->alpha_variance_ = 0.01f;
  param->learning_alpha_weights_ = 0.01f;
  param->learning_alpha_means_ = 0.01f;
  param->learning_alpha_variance_ = 0.01f;
  param->rho_mean_ = -1;
  param->rho_variance_ = -1;
  param->background_thres_ = 0.98f;
  param->order_by_weight_only_ = false;
  param->num_mixture_comp_ = 2;
  param->min_samples_for_detection_ = 0;
  param->no_update_threshold_ = -1;
  param->num_pix_comp_ = 3;

  vidtk::sg_gm_pixel_model gmm(param);
  float values[] = {0,0,0};
  unsigned int fg_count = 0;

  for(unsigned int i = 0; i < 709; ++i)
  {
    if(i == 700)
    {
      values[0] = 255;
      values[1] = 255;
      values[2] = 255;
    }
    else if(i < 700 && i % 2)
    {
      values[0] = 255*0.5;
      values[1] = 255*0.5;
      values[2] = 255*0.5;
    }
    else if(i < 700)
    {
      values[0] = 0;
      values[1] = 0;
      values[2] = 0;
    }

    if (gmm.update(values))
    {
      fg_count++;
    }
  }
/*  vcl_cout << fg_count << vcl_endl;*/
  TEST( "Based on the parameters, it should take 4 interations to become background", fg_count, 4 );
}


void test_gaussian_random_pixel()
{
  vcl_cout << "Tests three backgournd pixels radomly generated in a gaussian distobution and one forground pixel\n";
  vidtk::sg_gm_pixel_model_params * param = new vidtk::sg_gm_pixel_model_params();
  param->mahan_dist_sqr_thres_ = 2.5 * 2.5;
  float init_sig = 4;
  param->initial_variance_ = init_sig * init_sig;
  param->initial_weight_ = 0.01f;
  param->alpha_weights_ = 0.01f;
  param->alpha_means_ = 0.01f;
  param->alpha_variance_ = 0.01f;
  param->learning_alpha_weights_ = 0.01f;
  param->learning_alpha_means_ = 0.01f;
  param->learning_alpha_variance_ = 0.01f;
  param->rho_mean_ = param->rho_variance_ = -1;
  param->background_thres_ = 0.9f;//0.9;
  param->order_by_weight_only_ = false;
  param->num_mixture_comp_ = 4;
  param->min_samples_for_detection_ = 20;
  param->learning_type_ = vidtk::sg_gm_pixel_model_params::useSlackWeight;
  param->no_update_threshold_ = -1;
  param->num_pix_comp_ = 3;

  vnl_random rand_back[] = {vnl_random(10), vnl_random(23), vnl_random(43), vnl_random(44)};
  float shift_back[3][4] = {{5,  20, 46,  66}, {20,  44, 0, 64}, {10, 30, 155, 50} };
  float scale_back[] = {2, 5, 2, 3};
  vnl_random rand_for = vnl_random(55);
  double sift_front[] = {55, 150, 250};
  double scale_front = 13;
  vnl_random rand(100);

  vidtk::sg_gm_pixel_model gmm(param);
  float values[] = {0,0,0};
  //unsigned int count = 0;
  //unsigned int fired = 0;

  for(unsigned int i = 0; i < 1000; ++i)
  {
    double r = rand.drand32();
    if(r<param->background_thres_)
    {
      int a = rand.lrand32(2);
      for(unsigned int j = 0; j < 3; ++j)
      {
        values[j] = static_cast<float>( (rand_back[a].normal() )*scale_back[a] + shift_back[j][a] );
      }
    }
    else
    {
      for(unsigned int j = 0; j < 3; ++j)
      {
        values[j] = static_cast<float>( (rand_for.normal() )*scale_front +sift_front[j] );
      }
    }
    gmm.update(values);
  }

  for(unsigned int i = 0; i < 3; ++i)
  {
    for(unsigned int j = 0; j < 3; ++j)
    {
      values[j] = shift_back[j][i];
    }
    TEST( "Should be background", gmm.update(values), false );
  }
  for(unsigned int j = 0; j < 3; ++j)
  {
    values[j] = static_cast<float>( sift_front[j] );
  }
  TEST( "Should be foreground", gmm.update(values), true );
}


} // end anonymous namespace

int test_sg_gm_pixel_model( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "Stauffer-Grimson Gaussian Mixture Models" );

  test_foreground_detection_and_transition();
  test_weight_initialization();
  test_gaussian_random_pixel();

  return testlib_test_summary();
}
