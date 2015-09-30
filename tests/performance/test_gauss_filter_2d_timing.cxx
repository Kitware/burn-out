/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <vil/algo/vil_gauss_filter.h>
#include <vil/vil_load.h>
#include <vil/vil_image_view.h>
#include <vcl_fstream.h>
#include <vil/vil_save.h>
#include <video/gauss_filter.h>

#include <utilities/checked_bool.h>
#include <ctime>

using namespace vidtk;

struct iter_results
{
  int iterations;
  time_t runtime;
};

int test_vil_convolve_2d_performance(
  vil_image_view<vxl_byte> & out_img_dbl,
  vil_image_view<vxl_byte> & out_img_int,
  vcl_string filename
  );

double test_pixel_error(
  vil_image_view<vxl_byte> & out_img_dbl,
  vil_image_view<vxl_byte> & out_img_int
  );

int main( int argc, char** argv)
{

  if ( argc != 2 )
  {
    vcl_cerr << "Expecting a text file with image paths as an argument" << vcl_endl;
    return 1;
  }

  vcl_string filename = argv[1];
  vil_image_view<vxl_byte> out_img_dbl;
  vil_image_view<vxl_byte> out_img_int;

  test_vil_convolve_2d_performance( out_img_dbl, out_img_int, filename );

  double error = test_pixel_error( out_img_dbl, out_img_dbl );
  vcl_cout << "Pixel error on the same image: " << error << vcl_endl;
  error = test_pixel_error( out_img_dbl, out_img_int );
  vcl_cout << "Pixel error on different images: " << error << vcl_endl;

  return 0;
}


double test_pixel_error(
  vil_image_view<vxl_byte> & lhs,
  vil_image_view<vxl_byte> & rhs
  )
{
  if (lhs.nplanes() != rhs.nplanes() ||
      lhs.nj() != rhs.nj() ||
      lhs.ni() != rhs.ni())
  {
    return -1;
  }

  double sum = 0.0;
  long pixel_count = 0;
  for (unsigned p = 0; p < rhs.nplanes(); ++p)
  {
    for (unsigned j = 0; j < rhs.nj(); ++j)
    {
      for (unsigned i = 0; i < rhs.ni(); ++i)
      {
        double diff =  rhs(i,j,p) - lhs(i,j,p) ;
        sum += diff*diff;
        ++pixel_count;
      }
    }
  }
  double error = sqrt( sum/pixel_count );
  return error;
}

int test_vil_convolve_2d_performance(
  vil_image_view<vxl_byte> & out_img_dbl,
  vil_image_view<vxl_byte> & out_img_int,
  vcl_string img_list
  )
{
  vcl_ifstream file;
  file.open( img_list.c_str() );

  if ( ! file.is_open() )
  {
    vcl_cerr << "Error reading: " << img_list << vcl_endl;
    return false;
  }

  int smoothing_iterations = 10;
  time_t st_time;
  time_t end_time;
  time_t runtime_orig = 0;
  time_t runtime_new = 0;

  int iter_array[] = {1};//,2,3,4,5};//,10,50,100,200,500,1000};
  vcl_vector<int> iterations( iter_array, iter_array + sizeof(iter_array) / sizeof(int) );

  vcl_vector<vcl_string> image_paths;
  vcl_string line;
  while( !file.eof() )
  {
    vcl_getline(file, line);
    image_paths.push_back( line );
  }
  file.close();

  vcl_vector< iter_results > orig_results;
  vcl_vector< iter_results > new_results;

  for (int iters = 0; iters < iterations.size(); iters++)
  {
    int run_length = iterations[iters];
    iter_results oldresults;
    iter_results newresults;
    oldresults.iterations = run_length;
    newresults.iterations = run_length;

    for ( int x = 0; x < run_length; x++)
    {
      vcl_string filename = image_paths[x];
      vil_image_view<vxl_byte> src_img = vil_load( filename.c_str() );

      out_img_dbl.clear();
      st_time = clock();
      for (int i = 0; i < smoothing_iterations; i++)
      {
        vil_gauss_filter_2d(src_img, out_img_dbl, 0.849, 4);
      }
      end_time = clock();
      runtime_orig += (end_time - st_time)/smoothing_iterations;

      //    vcl_string double_filename = filename + "DOUBLESMOOTHED.png";
      //   vil_save( out_img_dbl, double_filename.c_str() );


      out_img_int.clear();
      st_time = clock();
      for (int i = 0; i < smoothing_iterations; i++)
      {
        gauss_filter_2d_int(src_img, out_img_int, .849, 4);
      }
      end_time = clock();
      runtime_new += (end_time - st_time)/smoothing_iterations;
      //   vcl_string integer_filename = filename + "INTEGERSMOOTHEDPOST.png";
      //  vil_save( out_img_int, integer_filename.c_str() );

    }
    oldresults.runtime = runtime_orig;
    newresults.runtime = runtime_new;

    orig_results.push_back( oldresults );
    new_results.push_back( newresults );
  }

  for ( int r = 0; r < orig_results.size(); r++)
  {

    iter_results oldresults = orig_results[r];
    iter_results newresults = new_results[r];

    vcl_cout << "double multiplication runtime over " << oldresults.iterations << " images took " << (double)oldresults.runtime/CLOCKS_PER_SEC << " seconds"  << vcl_endl;
    vcl_cout << "integer multiplication runtime over " << newresults.iterations << " images took " << (double)newresults.runtime/CLOCKS_PER_SEC << " seconds " << vcl_endl;
  }

}
