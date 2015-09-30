/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>

#include <klt/klt.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


vcl_string g_data_dir;


inline double sqr( double x ) { return x*x; }

void
test_expected_translation()
{
  vcl_cout << "Test image translation\n";

  vil_image_view<vxl_byte> img0 = vil_load( (g_data_dir+"/ocean_city.png").c_str() );
  vil_image_view<vxl_byte> img1 = vil_load( (g_data_dir+"/ocean_city_trans+16+6.png").c_str() );

  TEST( "Img 0 is grayscale", img0.nplanes(), 1 );
  TEST( "Img 0 is contiguous", img0.is_contiguous(), true );
  TEST( "Img 1 is grayscale", img1.nplanes(), 1 );
  TEST( "Img 1 is contiguous", img1.is_contiguous(), true );
  TEST( "Images are same size", img0.ni()==img1.ni() && img0.nj()==img1.nj(), true );

  KLT_TrackingContext tc;
  KLT_FeatureList fl;
  int nFeatures = 100;
  int i;

  tc = KLTCreateTrackingContext();
  KLTPrintTrackingContext(tc);
  fl = KLTCreateFeatureList(nFeatures);

  KLTSelectGoodFeatures(tc, img0.top_left_ptr(), img0.ni(), img0.nj(), fl);

  vcl_vector<float> xs, ys;
  unsigned found_count = 0;
  for (i = 0 ; i < fl->nFeatures ; i++)
  {
    xs.push_back( fl->feature[i]->x );
    ys.push_back( fl->feature[i]->y );
    if( fl->feature[i] >= 0 )
    {
      ++found_count;
    }
  }

  //KLTWriteFeatureListToPPM(fl, img1, ncols, nrows, "feat1.ppm");
  //KLTWriteFeatureList(fl, "feat1.txt", "%3d");

  KLTTrackFeatures(tc, img0.top_left_ptr(), img1.top_left_ptr(),
                   img0.ni(), img0.nj(), fl);

  bool good = true;
  unsigned tracked_count = 0;
  for (i = 0 ; i < fl->nFeatures ; i++)  {
    // expected translation: (16,6)
    float x = fl->feature[i]->x - 16;
    float y = fl->feature[i]->y - 6;
    if( fl->feature[i]->val >= 0 )
    {
      ++tracked_count;
      double e = vcl_sqrt( sqr(x-xs[i])+sqr(y-ys[i]) );
      if( e > 1 )
      {
        vcl_cout << "Pt " << i << ": ("<<xs[i]<<","<<ys[i]<<")->("
                 << x << "," << y << "); err = " << e << vcl_endl;
        good = false;
      }
    }
  }

  vcl_cout << "Found count = " << found_count << "\n";
  vcl_cout << "Tracked count = " << tracked_count << "\n";

  TEST( "Tracked features were tracked well", good, true );
  TEST( "Enough features tracked", tracked_count > 0.6 * found_count, true );

  //KLTWriteFeatureListToPPM(fl, img2, ncols, nrows, "feat2.ppm");
  //KLTWriteFeatureList(fl, "feat2.fl", NULL);      /* binary file */
  //KLTWriteFeatureList(fl, "feat2.txt", "%5.1f");  /* text file   */

}


} // end anonymous namespace

int test_klt( int argc, char* argv[] )
{
  testlib_test_start( "direct KLT" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    g_data_dir = argv[1];

    test_expected_translation();
  }

  return testlib_test_summary();
}
