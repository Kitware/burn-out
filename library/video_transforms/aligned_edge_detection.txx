/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "aligned_edge_detection.h"

#include <vil/vil_transform.h>
#include <vil/algo/vil_sobel_3x3.h>

#include <cmath>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "aligned_edge_detection_txx" );


// Perform NMS on the input gradient images in horizontal and vert directions only
template< typename InputType, typename OutputType >
void calculate_aligned_edges( const vil_image_view<InputType>& grad_i,
                              const vil_image_view<InputType>& grad_j,
                              const InputType edge_threshold,
                              vil_image_view<OutputType>& output )
{
  LOG_ASSERT( grad_i.ni() == grad_j.ni(), "Input image dimensions must be equivalent" );
  LOG_ASSERT( grad_i.nj() == grad_j.nj(), "Input image dimensions must be equivalent" );

  const unsigned ni = grad_i.ni();
  const unsigned nj = grad_i.nj();

  // Allocate a new output image if necessary
  if( output.ni() != ni || output.nj() != nj )
  {
    output.set_size( ni, nj, 2 );
  }

  // Perform non-maximum suppression
  for( unsigned j = 1; j < nj-1; j++ )
  {
    for( unsigned i = 1; i < ni-1; i++ )
    {
      const InputType val_i = grad_i( i, j );
      const InputType val_j = grad_j( i, j );

      if( val_i > edge_threshold )
      {
        if( val_i >= grad_i( i-1, j ) && val_i >= grad_i( i+1, j ) )
        {
          output( i, j, 0 ) = static_cast< OutputType >( val_i );
        }
      }
      if( val_j > edge_threshold )
      {
        if( val_j >= grad_j( i, j-1 ) && val_j >= grad_j( i, j+1 ) )
        {
          output( i, j, 1 ) = static_cast< OutputType >( val_j );
        }
      }
    }
  }
}

// Calculate potential edges
template< typename PixType, typename GradientType >
void calculate_aligned_edges( const vil_image_view< PixType >& input,
                              const GradientType edge_threshold,
                              vil_image_view< PixType >& output,
                              vil_image_view< GradientType >& grad_i,
                              vil_image_view< GradientType >& grad_j )
{
  // Calculate sobel approx in x/y directions
  vil_sobel_3x3( input, grad_i, grad_j );

  // Allocate a new output image if necessary
  if( output.ni() != input.ni() || output.nj() != input.nj() )
  {
    output.set_size( input.ni(), input.nj(), 2 );
  }

  output.fill( 0 );

  // Take absolute value of gradients
  vil_transform< GradientType, GradientType (GradientType)>( grad_i, std::abs );
  vil_transform< GradientType, GradientType (GradientType)>( grad_j, std::abs );

  // Perform NMS in vert/hori directions and threshold magnitude
  calculate_aligned_edges( grad_i, grad_j, edge_threshold, output );
}

} // end namespace vidtk
