/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "integral_hog_descriptor.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <limits>

#include <boost/math/constants/constants.hpp>

#include <vil/vil_plane.h>
#include <vil/vil_math.h>
#include <vil/algo/vil_sobel_3x3.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER( "integral_hog_descriptor" );


// Internal Constants
static const unsigned int II_ORIENTATION_BINS = custom_ii_hog_model::bins_per_cell;
static const double II_PI = boost::math::constants::pi<double>();


// Create required integral images
template< typename PixType >
integral_hog_descriptor<PixType>
::integral_hog_descriptor( const input_image_t& input )
{
  this->set_grayscale_ref_image( input );
}

template< typename PixType >
integral_hog_descriptor<PixType>
::integral_hog_descriptor( const vil_image_view< double >& grad_i,
                           const vil_image_view< double >& grad_j )
{
  this->set_gradients_ref_image( grad_i, grad_j );
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::set_grayscale_ref_image( const input_image_t& input )
{
  vil_image_view<double> grad_i, grad_j;
  vil_sobel_3x3( input, grad_i, grad_j );
  this->set_gradients_ref_image( grad_i, grad_j );
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::set_gradients_ref_image( const vil_image_view< double >& grad_i,
                           const vil_image_view< double >& grad_j )
{
  LOG_ASSERT( grad_i.ni() == grad_j.ni(), "Input col sizes do not match" );
  LOG_ASSERT( grad_i.nj() == grad_j.nj(), "Input row sizes do not match" );

  // Reset image chains
  directional_gradients_.set_size( grad_i.ni(), grad_i.nj(), II_ORIENTATION_BINS );
  directional_gradients_.fill( 0 );
  integral_gradients_.set_size( grad_i.ni()+1, grad_i.nj()+1, II_ORIENTATION_BINS );
  integral_gradients_.fill( 0 );

  std::ptrdiff_t gi_istep = grad_i.istep();
  std::ptrdiff_t gj_istep = grad_j.istep();

  // The default image allocation method should create non-interlaced planes so this
  // statement should be true unless VXL changes, because we are handling the allocation
  // of all internal images above.
  LOG_ASSERT( directional_gradients_.istep() == 1, "Optimizations require an istep of 1" );

  // Factor which scales our pixel gradients to their respective bin mappings
  const double gradient_adj = II_PI / 2;
  const double gradient_factor = II_ORIENTATION_BINS / II_PI;

  // Calculate actual integral image chains (1 per dt bin)
  for( unsigned j = 0; j < grad_i.nj(); j++ )
  {
    const double* i_ptr = &grad_i(0,j);
    const double* j_ptr = &grad_j(0,j);
    double** dir_ptrs = new double*[II_ORIENTATION_BINS];

    for( unsigned p = 0; p < II_ORIENTATION_BINS; p++ )
    {
      dir_ptrs[p] = &directional_gradients_(0,j,p);
    }

    for( unsigned i = 0; i < grad_j.ni(); i++, i_ptr += gi_istep, j_ptr += gj_istep )
    {
      // Derivatives in i and j direction for this pixel
      const double& di = *i_ptr;
      const double& dj = *j_ptr;

      // Place this gradient in the correct gradient bin image
      const double pt_grad = ( di == 0 ? 0 : std::atan(dj / di) );
      const double pt_mag = std::sqrt( (di * di) + (dj * dj) );
      const unsigned pt_bin = static_cast<unsigned>( ( gradient_adj + pt_grad ) * gradient_factor );
      dir_ptrs[pt_bin][i] = pt_mag;
    }

    delete[] dir_ptrs;
  }

  for( unsigned p = 0; p < II_ORIENTATION_BINS; p++ )
  {
    vil_image_view< double > dir_plane = vil_plane( directional_gradients_, p );
    vil_image_view< double > ii_plane = vil_plane( integral_gradients_, p );
    vil_math_integral_image( dir_plane, ii_plane );
  }
}

// Calculate features for an individual hog cell using II [semi-optimized, unsafe]
inline void calculate_hog_cell_features( const double* pixel,
                                         const std::ptrdiff_t istep,
                                         const std::ptrdiff_t jstep,
                                         const std::ptrdiff_t pstep,
                                         double* output )
{
  for( unsigned p = 0; p < II_ORIENTATION_BINS; p++, pixel += pstep )
  {
    output[p] = ( ( pixel[0] + pixel[jstep + istep] )-( pixel[jstep] + pixel[istep] ) );
  }
}

// Calculate features for an individual hog cell using II [semi-optimized, unsafe]
inline void calculate_hog_cell_features( const double* pixel,
                                         const std::ptrdiff_t istep,
                                         const std::ptrdiff_t jstep,
                                         const std::ptrdiff_t pstep,
                                         double* output,
                                         const double scale_factor )
{
  for( unsigned p = 0; p < II_ORIENTATION_BINS; p++, pixel += pstep )
  {
    output[p] = scale_factor * ( ( pixel[0] + pixel[jstep + istep] ) -
                                 ( pixel[jstep] + pixel[istep] ) );
  }
}

// Compute features for a single cell
template< typename PixType >
void
integral_hog_descriptor<PixType>
::cell_descriptor( const unsigned i,
                   const unsigned j,
                   const unsigned width,
                   const unsigned height,
                   double* output ) const
{
  LOG_ASSERT( i + width < integral_gradients_.ni(), "Request out of bounds" );
  LOG_ASSERT( j + height < integral_gradients_.nj(), "Request out of bounds" );
  LOG_ASSERT( width > 0, "Zero width received" );
  LOG_ASSERT( height > 0, "Zero height received" );

  calculate_hog_cell_features( &integral_gradients_(i,j),
                               width,
                               height * integral_gradients_.jstep(),
                               integral_gradients_.planestep(),
                               output );
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::cell_descriptor( const image_region_t& region,
                   double* output ) const
{
  this->cell_descriptor( region.min_x(),
                         region.min_y(),
                         region.width(),
                         region.height(),
                         output);
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::cell_descriptor( const image_region_t& region,
                   descriptor_t& output ) const
{
  output.resize( II_ORIENTATION_BINS ),
  this->cell_descriptor( region.min_x(),
                         region.min_y(),
                         region.width(),
                         region.height(),
                         &output[0] );
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::normalized_cell_descriptor( const unsigned i,
                              const unsigned j,
                              const unsigned width,
                              const unsigned height,
                              double* output ) const
{
  LOG_ASSERT( i + width < integral_gradients_.ni(), "Request out of bounds" );
  LOG_ASSERT( j + height < integral_gradients_.nj(), "Request out of bounds" );
  LOG_ASSERT( width > 0, "Zero width received" );
  LOG_ASSERT( height > 0, "Zero height received" );

  calculate_hog_cell_features( &integral_gradients_(i,j,0),
                               width * integral_gradients_.istep(),
                               height * integral_gradients_.jstep(),
                               integral_gradients_.planestep(),
                               output,
                               1.0 / ( width * height ) );
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::normalized_cell_descriptor( const image_region_t& region,
                              double* output ) const
{
  this->normalized_cell_descriptor( region.min_x(),
                                    region.min_y(),
                                    region.width(),
                                    region.height(),
                                    output);
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::normalized_cell_descriptor( const image_region_t& region,
                              descriptor_t& output ) const
{
  output.resize( II_ORIENTATION_BINS ),
  this->normalized_cell_descriptor( region.min_x(),
                                    region.min_y(),
                                    region.width(),
                                    region.height(),
                                    &output[0] );
}

// Generate a HoG descriptor
template< typename PixType >
void
integral_hog_descriptor<PixType>
::generate_hog_descriptor( const image_region_t& region,
                           const custom_ii_hog_model& model,
                           descriptor_t& output ) const
{
  output.resize( model.feature_count(), 0.0 );

  this->generate_hog_descriptor( region,
                                 model,
                                 &output[0] );

}

vgl_box_2d< unsigned > convert_norm_to_image_loc( const vgl_box_2d< double > norm_loc,
                                                  const vgl_box_2d< unsigned >& region )
{
  return vgl_box_2d< unsigned >(
    static_cast<unsigned>( region.min_x() + norm_loc.min_x() * region.width() ),
    static_cast<unsigned>( region.min_x() + norm_loc.max_x() * region.width() ),
    static_cast<unsigned>( region.min_y() + norm_loc.min_y() * region.height() ),
    static_cast<unsigned>( region.min_y() + norm_loc.max_y() * region.height() ) );
}

inline void normalize_block( double *block_start, unsigned len )
{
  double sum_sqr = 0;
  double *ptr;
  const double *block_end = block_start + len;

  for( ptr = block_start; ptr != block_end; ptr++ )
  {
    sum_sqr += ( (*ptr) * (*ptr) );
  }

  sum_sqr = std::sqrt( sum_sqr );

  if( sum_sqr != 0 )
  {
    for( ptr = block_start; ptr != block_end; ptr++ )
    {
      *ptr = *ptr / sum_sqr;
    }
  }
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::generate_hog_descriptor( const image_region_t& region,
                           const custom_ii_hog_model& model,
                           double* output ) const
{
  LOG_ASSERT( region.max_x() < integral_gradients_.ni(), "Specified region is OOB." );
  LOG_ASSERT( region.max_y() < integral_gradients_.nj(), "Specified region is OOB." );
  LOG_ASSERT( output != NULL, "Input pointer is invalid." );

  // Size of an individual cell
  const unsigned cell_size = II_ORIENTATION_BINS;
  const unsigned cell_size_bytes = cell_size * sizeof(double);

  // First, compute all cell features
  double *cell_features = new double[ model.cells_.size() * cell_size ];

  image_region_t cell_loc;
  double *cell_features_itr = cell_features;
  for( unsigned i = 0; i < model.cells_.size(); i++, cell_features_itr+=cell_size )
  {
    // Convert normalized location to actual location in image plane
    const vgl_box_2d< double >& norm_cell_loc = model.cells_[i];
    cell_loc = convert_norm_to_image_loc( norm_cell_loc, region );

    // Compute cell features
    this->normalized_cell_descriptor( cell_loc, cell_features_itr );
  }

  // Now, compute normalized block features
  double *output_itr = output;
  for( unsigned i = 0; i < model.blocks_.size(); i++ )
  {
    const std::vector< unsigned >& block_indices = model.blocks_[i];
    const unsigned block_size = cell_size * block_indices.size();

    // Copy cell features into output region for this block
    double *block_itr = output_itr;
    for( unsigned j = 0; j < block_indices.size(); j++, block_itr+=cell_size )
    {
      std::memcpy( block_itr,
                  cell_features + cell_size * block_indices[j],
                  cell_size_bytes );
    }

    // L2 normalize all of the copied cell features
    normalize_block( output_itr, block_size );
    output_itr += block_size;
  }

  delete[] cell_features;
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::generate_hog_descriptors( const std::vector< image_region_t >& regions,
                            const custom_ii_hog_model& model,
                            std::vector< descriptor_t >& descriptors ) const
{
  for( unsigned i = 0; i < regions.size(); i++ )
  {
    descriptors.push_back( std::vector<double>( model.feature_count(), 0.0 ) );

    this->generate_hog_descriptor( regions[i],
                                   model,
                                   &(descriptors.back()[0]) );
  }
}

template< typename PixType >
void
integral_hog_descriptor<PixType>
::generate_hog_descriptors( const unsigned cell_pixel_size,
                            const unsigned cells_per_hog_dim,
                            std::vector< image_region_t >& regions,
                            std::vector< descriptor_t >& descriptors,
                            bool normalize_descriptors ) const
{
  LOG_ASSERT( cell_pixel_size >= 1, "cell pixel size must not be 0" );

  const unsigned ni = directional_gradients_.ni();
  const unsigned nj = directional_gradients_.nj();

  const unsigned cells_i = ni / cell_pixel_size;
  const unsigned cells_j = nj / cell_pixel_size;

  const unsigned total_cells = cells_i * cells_j;
  const unsigned bins_per_cell = II_ORIENTATION_BINS;

  const unsigned hog_pixel_size = cells_per_hog_dim * cell_pixel_size;

  const unsigned hogs_i = cells_i - cells_per_hog_dim;
  const unsigned hogs_j = cells_j - cells_per_hog_dim;

  const unsigned cells_per_descriptor = cells_per_hog_dim * cells_per_hog_dim;
  const unsigned descriptor_size = cells_per_descriptor * bins_per_cell;

  if( hog_pixel_size > ni || hog_pixel_size > nj )
  {
    return;
  }

  descriptor_t cell_features( total_cells * bins_per_cell , 0.0 );

  double *cell_ptr = &cell_features[0];

  // Compute all cell features
  for( unsigned j = 0; j + cell_pixel_size <= nj; j += cell_pixel_size )
  {
    for( unsigned i = 0; i + cell_pixel_size <= ni; i += cell_pixel_size )
    {
      this->cell_descriptor( i, j, cell_pixel_size, cell_pixel_size, cell_ptr );
      cell_ptr += bins_per_cell;
    }
  }

  // Compute all full descriptors
  const double *cell_row_ptr = &cell_features[0];
  const std::ptrdiff_t cell_row_step = cells_i * bins_per_cell;

  const unsigned desc_row_length = cells_per_hog_dim * bins_per_cell;
  const unsigned desc_row_length_bytes = desc_row_length * sizeof( double );

  std::vector< std::ptrdiff_t > cell_steps( cells_per_hog_dim, 0 );

  for( unsigned i = 0; i < cells_per_hog_dim; i++ )
  {
    cell_steps[i] = cell_row_step * i;
  }

  regions.resize( hogs_j * hogs_i );
  descriptors.resize( hogs_j * hogs_i, descriptor_t( descriptor_size, 0.0 ) );

  unsigned hog_index = 0;

  for( unsigned j = 0; j < hogs_j; j++, cell_row_ptr += cell_row_step )
  {
    const double *cell_col_ptr = cell_row_ptr;

    for( unsigned i = 0; i < hogs_i; i++, cell_col_ptr += bins_per_cell, hog_index++ )
    {
      const unsigned hog_i = i * cell_pixel_size;
      const unsigned hog_j = j * cell_pixel_size;

      regions[hog_index] = image_region_t( hog_i, hog_i + hog_pixel_size,
                                           hog_j, hog_j + hog_pixel_size );

      double *desc_ptr = &(descriptors[hog_index][0]);
      double *desc_row_ptr = desc_ptr;

      for( unsigned k = 0; k < cells_per_hog_dim; k++, desc_row_ptr += desc_row_length )
      {
        std::memcpy( desc_row_ptr,
                    cell_col_ptr + cell_steps[k],
                    desc_row_length_bytes );
      }

      if( normalize_descriptors )
      {
        normalize_block( desc_ptr, descriptor_size );
      }
    }
  }
}

} // end namespace vidtk
