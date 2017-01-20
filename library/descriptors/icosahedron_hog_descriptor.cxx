/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/icosahedron_hog_descriptor.h>

#include <vil/vil_plane.h>
#include <vil/vil_fill.h>
#include <vil/vil_resample_bilin.h>
#include <vil/vil_math.h>

namespace vidtk
{


// Given 2 sequential planes of the same size, calculate the time
// partial derivative in the time direction for the 2 images
void calculate_t_differential( const vil_image_view< float >& plane1,
                               const vil_image_view< float >& plane2,
                               vil_image_view< float >& dst )
{
  vil_math_image_difference( plane1, plane2, dst );
}


// Same as above, images must already be allocated
template< typename FloatType >
void calculate_xy_differential( const vil_image_view< FloatType >& src,
                                vil_image_view< FloatType >& dst_x,
                                vil_image_view< FloatType >& dst_y )
{
  // Insure inputs are already allocated
  unsigned ni = src.ni(), nj = src.nj();
  assert(dst_x.ni()==ni && dst_x.nj()==nj && dst_x.nplanes()==src.nplanes());
  assert(dst_y.ni()==ni && dst_y.nj()==nj && dst_y.nplanes()==src.nplanes());
  assert( ni > 0 && nj > 0 && src.nplanes() == 1 );

  // Fill boundaries with 0
  vil_fill_row( dst_x, 0, static_cast<FloatType>(0.0) );
  vil_fill_row( dst_y, 0, static_cast<FloatType>(0.0) );
  vil_fill_row( dst_x, nj-1, static_cast<FloatType>(0.0) );
  vil_fill_row( dst_y, nj-1, static_cast<FloatType>(0.0) );
  vil_fill_col( dst_x, 0, static_cast<FloatType>(0.0) );
  vil_fill_col( dst_y, 0, static_cast<FloatType>(0.0) );
  vil_fill_col( dst_x, ni-1, static_cast<FloatType>(0.0) );
  vil_fill_col( dst_y, ni-1, static_cast<FloatType>(0.0) );

  // Get required steps
  std::ptrdiff_t istepS=src.istep(),jstepS=src.jstep();
  std::ptrdiff_t istepX=dst_x.istep(),jstepX=dst_x.jstep();
  std::ptrdiff_t istepY=dst_y.istep(),jstepY=dst_y.jstep();

  // Calculate derivatives
  const FloatType* rowS = src.top_left_ptr() + jstepS + istepS;
  FloatType* rowX = dst_x.top_left_ptr() + jstepX + istepX;
  FloatType* rowY = dst_y.top_left_ptr() + jstepY + istepY;
  for (unsigned j=1;j<nj-1;++j,rowS += jstepS,rowX += jstepX,rowY += jstepY)
  {
    const FloatType* pixelS = rowS;
    FloatType* pixelX = rowX;
    FloatType* pixelY = rowY;
    for (unsigned i=1;i<ni-1;++i,pixelS += istepS,pixelX += istepX,pixelY += istepY)
    {
      *pixelX = *(pixelS-istepS) - *(pixelS+istepS);
      *pixelY = *(pixelS-jstepS) - *(pixelS+jstepS);
    }
  }
}

// [unsafe] Takes the dot product of the 3D input with vectors to each side
// of an icosahedron, so output should point to a 20 bin descriptor
inline void map_vector_to_hedron( float *input, float *output )
{
  float &v1 = input[0];
  float &v2 = input[1];
  float &v3 = input[2];

  float gr_x_v1 = v1 * 1.61803f;
  float gr_x_v2 = v2 * 1.61803f;
  float gr_x_v3 = v3 * 1.61803f;

  float invgr_x_v1 = v1 * 0.61803f;
  float invgr_x_v2 = v2 * 0.61803f;
  float invgr_x_v3 = v3 * 0.61803f;

  output[0] = v1 + v2 + v3;
  output[1] = v1 + v2 - v3;
  output[2] = v1 - v2 + v3;
  output[3] = v1 - v2 - v3;

  output[4] = -v1 + v2 + v3;
  output[5] = -v1 + v2 - v3;
  output[6] = -v1 - v2 + v3;
  output[7] = -v1 - v2 - v3;

  output[8] = invgr_x_v2 + gr_x_v3;
  output[9] = invgr_x_v2 - gr_x_v3;
  output[10] = -invgr_x_v2 + gr_x_v3;
  output[11] = -invgr_x_v2 - gr_x_v3;

  output[12] =  invgr_x_v1 + gr_x_v2;
  output[13] =  invgr_x_v1 - gr_x_v2;
  output[14] =  -invgr_x_v1 + gr_x_v2;
  output[15] =  -invgr_x_v1 - gr_x_v2;

  output[16] = gr_x_v1 + invgr_x_v3;
  output[17] = gr_x_v1 - invgr_x_v3;
  output[18] = -gr_x_v1 + invgr_x_v3;
  output[19] = -gr_x_v1 - invgr_x_v3;
}

// From a grayscale data cube, calculate <dr, dc, dt> for each inner pixel
void calculate_gradient_cube( const std::vector< vil_image_view<float> >& images,
                              std::vector< vil_image_view<float> >& cube )
{
  assert( images.size() > 2 );

  cube.clear();
  const unsigned ni = images[0].ni();
  const unsigned nj = images[0].nj();

  // Allocate and fill gradient images
  for( size_t i = 0; i < images.size()-2; i++ )
  {
    vil_image_view<float> gradients( ni, nj, 1, 3 );

    vil_image_view<float> grd_x = vil_plane( gradients, 0 );
    vil_image_view<float> grd_y = vil_plane( gradients, 1 );
    vil_image_view<float> grd_t = vil_plane( gradients, 2 );

    calculate_xy_differential( images[i+1], grd_x, grd_y );
    calculate_t_differential( images[i], images[i+2], grd_t );

    cube.push_back( gradients );
  }
}

// Below script performs a semi-optimized:
//  1. Subtracts threshold t from each of the 20 bins
//  2. Thresholds entries < 0 to be 0
//  3. Normalizes the resultant vector
//  4. Scales this normalized vector by the gradient mag for this location
inline void subtract_thresh_and_normalize( float *bins, const float& t, const float& gr_mag )
{

  // Get points to start / end of bins array
  float *ptr = bins;
  float *end = bins + 20;

  // Create counter of entries^2 for normalization
  float sqr_sum = 0.0f;

  // Subtract t, threshold, then increment counter
  for( ; ptr != end; ptr++ )
  {
    *ptr = *ptr - t;
    if( *ptr < 0.0f )
    {
      *ptr = 0.0f;
    }
    sqr_sum += (*ptr)*(*ptr);
  }

  // Normalize the 20-bin map then scale it by the gradient magnitude
  if( sqr_sum != 0.0f )
  {
    sqr_sum = static_cast<float> (sqrt( sqr_sum ));
    for( ptr = bins; ptr != end; ptr++ )
    {
      *ptr *= gr_mag / sqr_sum;
    }
  }
}

// Merge n floats from array x into array y with weight 'weight'
inline void merge_x_into_y( float* x, float* y, const float& weight, const int& n )
{
  float *xEnd = x + n;
  for( ; x != xEnd; x++, y++ )
  {
    *y += *x * weight;
  }
}


// A struct containing information about a particular cell in the HoG
struct cell_properties
{
  // cell start position (pixels)
  int r, c, t;

  // cell dimensions
  int height, width, depth;

  // pre-computer center coordinates of cell (w.r.t. start position)
  float cr, cc, ct;
};

// Calculate features for a particulate cell
void calculate_cell_features( std::vector< vil_image_view<float> >& cube, const cell_properties& prop, double *output )
{
  // Declare required variables
  float net_features[20] = { 0 };
  float features_for_pixel[20] = { 0 };

  // Populate weights for each pixel in cell
  float *r_weights = new float[ prop.height ];
  float *c_weights = new float[ prop.width ];
  float *t_weights = new float[ prop.depth ];
  for( int i = 0; i < prop.height; i++ )
  {
    r_weights[ i ] = 1.0f - 2.0f * std::abs(static_cast<float>(i) - prop.cr) / prop.height;
  }
  for( int i = 0; i < prop.width; i++ )
  {
    c_weights[ i ] = 1.0f - 2.0f * std::abs(static_cast<float>(i) - prop.cc) / prop.width;
  }
  for( int i = 0; i < prop.depth; i++ )
  {
    t_weights[ i ] = 1.0f - 2.0f * std::abs(static_cast<float>(i) - prop.ct) / prop.depth;
  }

  // Iterate through all images
  for( int img = prop.t; img < prop.t + prop.depth; img++ )
  {
    // Create pointer to lower left corner of cell BB
    float *img_row_iter = cube[ img ].top_left_ptr();
    long unsigned row_step = cube[ img ].jstep();
    img_row_iter = img_row_iter + 3 * prop.c;
    img_row_iter = img_row_iter + row_step * prop.r;

    // Cycle through BB in this image
    for( int r = 0; r < prop.height; r++, img_row_iter += row_step )
    {
      float *img_col_iter = img_row_iter;
      for( int c = 0; c < prop.width; c++, img_col_iter+=3 )
      {

        // Normalize Vector
        float &dr = img_col_iter[0];
        float &dc = img_col_iter[1];
        float &dt = img_col_iter[2];
        float vmag = dr*dr + dc*dc + dt*dt;

        // Check to make sure we don't have a 0 vector
        if( vmag != 0.0f )
        {

          // Divide out vector magnitude
          vmag = std::sqrt( vmag );
          dr /= vmag;
          dc /= vmag;
          dt /= vmag;

          // Map Vector to Hedron
          map_vector_to_hedron( img_col_iter, features_for_pixel );

          // Below script performs an optimized:
          //  1. Subtracts threshold t from each of the 20 bins
          //  2. Thresholds entries < 0 to be 0
          //  3. Normalizes the resultant vector
          //  4. Scales this normalized vector by the gradient mag for this location
          subtract_thresh_and_normalize( features_for_pixel, 1.29107f, vmag );

          // Scale bins by weighting function and add to cell sum
          float pixel_weight = std::fabs( r_weights[r]*c_weights[c]*t_weights[img-prop.t] );
          merge_x_into_y( features_for_pixel, net_features, pixel_weight, 20 );
        }
      }
    }
  }

  // Output features
  for( int i = 0; i < 20; i++ )
  {
    output[i] = static_cast<double>(net_features[i]);
  }

  // Deallocations
  delete[] r_weights;
  delete[] t_weights;
  delete[] c_weights;
}


bool calculate_icosahedron_hog_features( const std::vector< vil_image_view<float> >& images,
                                         std::vector<double>& features,
                                         const icosahedron_hog_parameters& options )
{
  // Validate input contents
  if( options.time_cells < 1 || images.size() < options.time_cells + 2 )
  {
    features.clear();
    return false;
  }
  if( images[0].ni() < 2*options.width_cells || images[0].nj() < 2*options.height_cells )
  {
    features.clear();
    return false;
  }

  // Initialize output vector
  int num_cells = options.width_cells * options.height_cells * options.time_cells;
  int descriptor_dim = 20 * num_cells;
  features.resize( descriptor_dim, 0.0 );

  // Calculate gradient cube
  std::vector< vil_image_view<float> > gradient_cube;
  calculate_gradient_cube( images, gradient_cube );

  // Perform descriptor computation
  int width = gradient_cube[0].ni();
  int height = gradient_cube[0].nj();
  int n = gradient_cube.size();
  std::vector< cell_properties > cp( num_cells );
  int pos = 0;
  for( unsigned t = 0; t < options.time_cells; t++ )
  {
    for( unsigned r = 0; r < options.height_cells; r++ )
    {
      for( unsigned c = 0; c < options.width_cells; c++ )
      {
        cell_properties cellInfo;
        cellInfo.r = (r * height) / options.height_cells;
        cellInfo.c = (c * width) / options.width_cells;
        cellInfo.t = (t * n) / options.time_cells;
        cellInfo.height = height / options.height_cells;
        cellInfo.width = width / options.width_cells;;
        cellInfo.depth = n / options.time_cells;;
        cellInfo.cr = (cellInfo.height-1.0f)/2.0f;
        cellInfo.cc = (cellInfo.width-1.0f)/2.0f;
        cellInfo.ct = (cellInfo.depth-1.0f)/2.0f;
        cp[ pos ] = cellInfo;
        pos++;
      }
    }
  }

  // Collect 20 features for each cell
  for( int cell = 0; cell < num_cells; cell++ )
  {
    calculate_cell_features( gradient_cube, cp[ cell ], &features[cell*20] );
  }

  // L2 normalize resultant vector comprised of all cell features
  double sum_squared = 0.0;
  for( unsigned i = 0; i < features.size(); i++ )
  {
    sum_squared += features[ i ] * features[ i ];
  }
  if( sum_squared != 0.0f )
  {
    sum_squared = sqrt( sum_squared );
    for( unsigned i = 0; i < features.size(); i++ )
    {
      features[ i ] /= sum_squared;
    }
  }
  else
  {
    return false;
  }

  return true;
}

} // end namespace vidtk
