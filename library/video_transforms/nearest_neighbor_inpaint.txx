/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "nearest_neighbor_inpaint.h"

#include <limits>
#include <vector>
#include <algorithm>

#include <vgl/vgl_point_2d.h>

namespace vidtk
{

// Checks to see if all of some pixels 4 neighbors is 1 in a binary mask
static inline bool is_inner_pixel( const bool* pos, const std::ptrdiff_t istep, const std::ptrdiff_t jstep )
{
  return !( *(pos+istep) && *(pos-jstep) && *(pos+jstep) && *(pos-istep) );
}

// Given the sum of values from neighboring pixels, estimate a given value for some pixel.
// For boolean type, simply check if most of the known neighboring pixels were true.
template< typename PixType >
static inline void estimate_pixel_value( const unsigned sum, const unsigned count, PixType& output )
{
  output = static_cast<PixType>(sum / count);
}

template <>
inline void estimate_pixel_value<bool>( const unsigned sum, const unsigned count, bool& output )
{
  output = ( 2 * sum > count );
}

// Helper function for accumulating some nearby RGB pixel's value into
// a buffer which contains the sum of every valid nearby pixel value
template< typename PixType >
static inline void accumulate_rgb_value_into_sum( const PixType* location,
                                                  const std::ptrdiff_t p1step,
                                                  const std::ptrdiff_t p2step,
                                                  unsigned* output_sum )
{
  output_sum[0] += *(location);
  output_sum[1] += *(location + p1step);
  output_sum[2] += *(location + p2step);
}

// A (relatively) fast simple nearest neighbor inpainting scheme
template< typename PixType >
void nn_inpaint( vil_image_view<PixType>& image,
                 const vil_image_view<bool>& mask,
                 vil_image_view<unsigned> status )
{
  assert( image.ni() == mask.ni() );
  assert( image.nj() == mask.nj() );
  assert( mask.nplanes() == 1 );
  assert( image.nplanes() == 1 || image.nplanes() == 3 );

  // Reset the status image. The status image contains a flag indicating
  // if a pixel needs inpainting, and if it should be inpainted on the
  // current iteration. A value of 2 * iteration indicates that it needs
  // to be inpainted on the specified iteration, and 2 * iteration - 1
  // if it has already been addressed on said iteration. A value of max(int)
  // indicates the pixel still needs inpainting on some future iteration.
  status.set_size( mask.ni(), mask.nj() );
  status.fill( 0 );

  const unsigned ni = image.ni();
  const unsigned ni_minus_1 = ni - 1;
  const unsigned nj = image.nj();
  const unsigned nj_minus_1 = nj - 1;
  const unsigned np = image.nplanes();

  const std::ptrdiff_t distep = image.istep(), djstep = image.jstep(), dpstep = image.planestep();
  const std::ptrdiff_t mistep = mask.istep(), mjstep = mask.jstep();
  const std::ptrdiff_t sistep = status.istep(), sjstep = status.jstep();

  const unsigned max_value = std::numeric_limits<unsigned>::max();

  // Pass 1 - establish a list of pixels to process on the first iteration
  std::vector< vgl_point_2d<unsigned> > points_to_process[2];
  const bool* mask_row = mask.top_left_ptr() + mjstep + mistep;
  for( unsigned j=1; j<nj_minus_1; ++j, mask_row += mjstep )
  {
    const bool* mask_pixel = mask_row;
    for( unsigned i=1; i<ni_minus_1; ++i, mask_pixel+=mistep )
    {
      if( *mask_pixel )
      {
        status(i,j) = max_value;
        if( is_inner_pixel( mask_pixel, mistep, mjstep ) )
        {
          points_to_process[0].push_back( vgl_point_2d<unsigned>(i,j) );
          status(i,j) = 2;
        }
      }
    }
  }

  // Pass 2 - iterate until we've filled in every point
  unsigned iteration = 1;
  unsigned buffer_pos = 0;
  unsigned next_buffer_pos = 1;

  if( np == 1 )
  {
    while( points_to_process[buffer_pos].size() > 0 )
    {
      // Get pointers to our points to process on this iteration, and next iteration
      std::vector< vgl_point_2d<unsigned> >& this_itr = points_to_process[buffer_pos];
      std::vector< vgl_point_2d<unsigned> >& next_itr = points_to_process[next_buffer_pos];
      next_itr.clear();

      const unsigned next_iteration_todo_id = 2 * (iteration + 1);
      const unsigned this_iteration_todo_id = 2 * iteration;
      const unsigned this_iteration_processed_id = this_iteration_todo_id - 1;

      unsigned new_pixel_value_sum = 0;
      unsigned new_pixel_value_count = 0;
      unsigned *left_status, *right_status, *top_status, *bot_status;

      // Process points, and accumulate points to process on the next itr
      for( unsigned p = 0; p < this_itr.size(); p++ )
      {
        const unsigned i = this_itr[p].x();
        const unsigned j = this_itr[p].y();

        unsigned *pixel_status = &status( i, j );

        // Indicates this pixel still needs to be filled on this iteration
        if( *pixel_status == this_iteration_todo_id )
        {
          // Set flag indicating this pixel was processed
          *pixel_status = this_iteration_processed_id;

          // Get pointer to value we want to fill
          PixType *value = &image( i, j );

          // Scan all 4 neigbors and interpolate values
          new_pixel_value_sum = 0;
          new_pixel_value_count = 0;

          left_status = pixel_status - sistep;
          right_status = pixel_status + sistep;
          top_status = pixel_status - sjstep;
          bot_status = pixel_status + sjstep;

          if( i > 0 )
          {
            if( *left_status < this_iteration_processed_id )
            {
              new_pixel_value_sum += *(value - distep);
              new_pixel_value_count++;
            }
            else if( *left_status > this_iteration_todo_id )
            {
              *left_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i-1,j) );
            }
          }

          if( i < ni_minus_1 )
          {
            if( *right_status < this_iteration_processed_id )
            {
              new_pixel_value_sum += *(value + distep);
              new_pixel_value_count++;
            }
            else if( *right_status > this_iteration_todo_id )
            {
              *right_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i+1,j) );
            }
          }

          if( j > 0 )
          {
            if( *top_status < this_iteration_processed_id )
            {
              new_pixel_value_sum += *(value - djstep);
              new_pixel_value_count++;
            }
            else if( *top_status > this_iteration_todo_id )
            {
              *top_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i,j-1) );
            }
          }

          if( j < nj_minus_1 )
          {
            if( *bot_status < this_iteration_processed_id )
            {
              new_pixel_value_sum += *(value + djstep);
              new_pixel_value_count++;
            }
            else if( *bot_status > this_iteration_todo_id )
            {
              *bot_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i,j+1) );
            }
          }

          // Fill in new pixel value according to some type specific rule
          estimate_pixel_value( new_pixel_value_sum, new_pixel_value_count, *value );
        }
      }

      // Swap buffer positions
      std::swap( buffer_pos, next_buffer_pos );

      // Increment itr counter
      iteration++;
    }
  }
  else if( np == 3 )
  {
    while( points_to_process[buffer_pos].size() > 0 )
    {
      // Get pointers to our points to process on this iteration, and next iteration
      std::vector< vgl_point_2d<unsigned> >& this_itr = points_to_process[buffer_pos];
      std::vector< vgl_point_2d<unsigned> >& next_itr = points_to_process[next_buffer_pos];
      next_itr.clear();

      const unsigned next_iteration_todo_id = 2 * (iteration + 1);
      const unsigned this_iteration_todo_id = 2 * iteration;
      const unsigned this_iteration_processed_id = this_iteration_todo_id - 1;

      unsigned new_pixel_value_sum[3] = {0};
      unsigned new_pixel_value_count = 0;

      const std::ptrdiff_t dp2step = dpstep + dpstep;

      unsigned *left_status, *right_status, *top_status, *bot_status;

      // Process points, and accumulate points to process on the next itr
      for( unsigned p = 0; p < this_itr.size(); p++ )
      {
        const unsigned i = this_itr[p].x();
        const unsigned j = this_itr[p].y();

        unsigned *pixel_status = &status( i, j );

        // Indicates this pixel still needs to be filled on this iteration
        if( *pixel_status == this_iteration_todo_id )
        {
          // Set flag indicating this pixel was processed
          *pixel_status = this_iteration_processed_id;

          // Get pointer to value we want to fill
          PixType *value = &image( i, j, 0 );

          // Don't fill border pixels

          // Scan all 4 neigbors and interpolate values
          new_pixel_value_sum[0] = 0;
          new_pixel_value_sum[1] = 0;
          new_pixel_value_sum[2] = 0;
          new_pixel_value_count = 0;

          left_status = pixel_status - sistep;
          right_status = pixel_status + sistep;
          top_status = pixel_status - sjstep;
          bot_status = pixel_status + sjstep;

          if( i > 0 )
          {
            if( *left_status < this_iteration_processed_id )
            {
              accumulate_rgb_value_into_sum( value-distep, dpstep, dp2step, new_pixel_value_sum );
              new_pixel_value_count++;
            }
            else if( *left_status > this_iteration_todo_id )
            {
              *left_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i-1,j) );
            }
          }

          if( i < ni_minus_1 )
          {
            if( *right_status < this_iteration_processed_id )
            {
              accumulate_rgb_value_into_sum( value+distep, dpstep, dp2step, new_pixel_value_sum );
              new_pixel_value_count++;
            }
            else if( *right_status > this_iteration_todo_id )
            {
              *right_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i+1,j) );
            }
          }

          if( j > 0 )
          {
            if( *top_status < this_iteration_processed_id )
            {
              accumulate_rgb_value_into_sum( value-djstep, dpstep, dp2step, new_pixel_value_sum );
              new_pixel_value_count++;
            }
            else if( *top_status > this_iteration_todo_id )
            {
              *top_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i,j-1) );
            }
          }

          if( j < nj_minus_1 )
          {
            if( *bot_status < this_iteration_processed_id )
            {
              accumulate_rgb_value_into_sum( value+djstep, dpstep, dp2step, new_pixel_value_sum );
              new_pixel_value_count++;
            }
            else if( *bot_status > this_iteration_todo_id )
            {
              *bot_status = next_iteration_todo_id;
              next_itr.push_back( vgl_point_2d<unsigned>(i,j+1) );
            }
          }

          // Fill in new pixel value according to some type specific rule
          estimate_pixel_value<PixType>( new_pixel_value_sum[0], new_pixel_value_count, *value );
          estimate_pixel_value<PixType>( new_pixel_value_sum[1], new_pixel_value_count, *(value+dpstep) );
          estimate_pixel_value<PixType>( new_pixel_value_sum[2], new_pixel_value_count, *(value+dp2step) );
        }
      }

      // Swap buffer positions
      std::swap( buffer_pos, next_buffer_pos );

      // Increment itr counter
      iteration++;
    }
  }

  // Pass 3 - Fill in boundary positions with nearest inside pixels
  if( image.ni() > 2 && image.nj() > 2 )
  {
    for( unsigned p = 0; p < image.nplanes(); ++p )
    {
      // Handle top and bottom
      for( unsigned i = 0; i < image.ni(); ++i )
      {
        if( mask( i, 0 ) )
        {
          image( i, 0 ) = image( i, 1 );
        }

        if( mask( i, nj_minus_1 ) )
        {
          image( i, nj_minus_1 ) = image( i, nj_minus_1 - 1 );
        }
      }

      // Handle left and right
      for( unsigned j = 0; j < image.nj(); ++j )
      {
        if( mask( 0, j ) )
        {
          image( 0, j ) = image( 1, j );
        }

        if( mask( ni_minus_1, j ) )
        {
          image( ni_minus_1, j ) = image( ni_minus_1 - 1, j );
        }
      }
    }
  }
}

} // end namespace vidtk
