/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "color_commonality_filter.h"

#include <utilities/point_view_to_region.h>

#include <vil/vil_fill.h>

#include <limits>
#include <vgl/vgl_box_2d.h>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>

namespace vidtk
{

// Simple helper functions
inline bool is_power_of_two( const unsigned num )
{
  return ( (num > 0) && ((num & (num - 1)) == 0) );
}

inline unsigned int integer_log2( unsigned int value )
{
  unsigned int l = 0;
  while( (value >> l) > 1 ) ++l;
  return l;
}

// An optimized (unsafe if used incorrectly) function which populates
// a n^p dimensional histogram from integer image 'input' given the
// resolution of each channel of the histogram, and a bitshift value
// which maps each value to its channel step for the histogram
template< class InputType >
void populate_image_histogram( const vil_image_view<InputType>& input,
                               unsigned *hist_top_left,
                               const unsigned bitshift,
                               const std::ptrdiff_t *hist_steps )
{
  // Get image properties
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  const std::ptrdiff_t istep = input.istep();
  const std::ptrdiff_t jstep = input.jstep();
  const std::ptrdiff_t pstep = input.planestep();

  // Filter image
  const InputType* row = input.top_left_ptr();
  for (unsigned j=0;j<nj;++j,row+=jstep)
  {
    const InputType* pixel = row;
    for (unsigned i=0;i<ni;++i,pixel+=istep)
    {
      unsigned step = 0;
      const InputType* plane = pixel;
      for (unsigned p=0;p<np;++p,plane+=pstep)
      {
        step += hist_steps[p] * ( *plane >> bitshift );
      }
      (*(hist_top_left + step))++;
    }
  }
}

// Integer-typed filtering main loop
template< class InputType, class OutputType >
typename boost::enable_if<boost::is_integral<OutputType> >::type
filter_color_image( const vil_image_view<InputType>& input,
                    vil_image_view<OutputType>& output,
                    std::vector<unsigned>& histogram,
                    const color_commonality_filter_settings& options )
{
  assert( input.ni() == output.ni() && input.nj() == output.nj() );
  assert( is_power_of_two( options.resolution_per_channel ) );

  if( input.ni() == 0 || input.nj() == 0 )
  {
    return;
  }

  // Configure output scaling settings based on the output type and user settings
  const InputType input_type_max = std::numeric_limits<InputType>::max();
  const OutputType output_type_max = std::numeric_limits<OutputType>::max();
  const unsigned histogram_threshold = static_cast<unsigned>(output_type_max);
  unsigned histogram_scale_factor = options.output_scale_factor;

  // Use type default options if no scale factor specified
  if( histogram_scale_factor == 0 )
  {
    histogram_scale_factor = histogram_threshold;
  }

  // Populate histogram steps for each channel of the hist
  std::vector< std::ptrdiff_t > histsteps( input.nplanes() );
  histsteps[0] = 1;
  for( unsigned p = 1; p < input.nplanes(); p++ )
  {
    histsteps[p] = histsteps[p-1] * options.resolution_per_channel;
  }

  // Fill in histogram of the input image
  const unsigned bitshift = integer_log2( (static_cast<unsigned>(input_type_max)+1)
                                          / options.resolution_per_channel );

  unsigned* hist_top_left = &histogram[0];

  populate_image_histogram( input, hist_top_left, bitshift, &histsteps[0] );

  // Normalize histogram to the output types range
  unsigned sum = 0;
  for (unsigned i=0;i<histogram.size();i++)
  {
    sum += histogram[i];
  }

  // Fill in color commonality image from the compiled histogram
  for (unsigned i=0;i<histogram.size();i++)
  {
    unsigned value = ( histogram_scale_factor * histogram[i] )/sum;
    histogram[i] = ( value > histogram_threshold ? histogram_threshold : value );
  }

  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  const std::ptrdiff_t istep = input.istep();
  const std::ptrdiff_t jstep = input.jstep();
  const std::ptrdiff_t pstep = input.planestep();

  const InputType* row = input.top_left_ptr();
  for (unsigned j=0;j<nj;++j,row+=jstep)
  {
    const InputType* pixel = row;
    for (unsigned i=0;i<ni;++i,pixel+=istep)
    {
      const InputType* plane = pixel;
      unsigned step = 0;
      for (unsigned p=0;p<np;++p,plane+=pstep)
      {
        step += histsteps[p] * ( *plane >> bitshift );
      }
      output(i,j) = *(hist_top_left+step);
    }
  }
}

// Float-typed output filtering main loop
template< class InputType, class OutputType >
typename boost::disable_if<boost::is_integral<OutputType> >::type
filter_color_image( const vil_image_view<InputType>& input,
                    vil_image_view<OutputType>& output,
                    std::vector<unsigned>& histogram,
                    const color_commonality_filter_settings& options )
{
  assert( input.ni() == output.ni() && input.nj() == output.nj() );
  assert( is_power_of_two( options.resolution_per_channel ) );

  if( input.ni() == 0 || input.nj() == 0 )
  {
    return;
  }

  // Configure output scaling settings based on the output type and user settings
  const InputType input_type_max = std::numeric_limits<InputType>::max();

  // Populate histogram steps for each channel of the hist
  std::vector< std::ptrdiff_t > histsteps( input.nplanes() );
  histsteps[0] = 1;
  for( unsigned p = 1; p < input.nplanes(); p++ )
  {
    histsteps[p] = histsteps[p-1] * options.resolution_per_channel;
  }

  // Fill in histogram of the input image
  const unsigned bitshift = integer_log2( (static_cast<unsigned>(input_type_max)+1)
                                          / options.resolution_per_channel );

  unsigned* hist_top_left = &histogram[0];

  populate_image_histogram( input, hist_top_left, bitshift, &histsteps[0] );

  // Normalize histogram to the output types range
  unsigned sum = 0;
  for (unsigned i=0;i<histogram.size();i++)
  {
    sum += histogram[i];
  }

  // Use type default options if no scale factor specified
  OutputType scale_factor = 1.0;
  if( options.output_scale_factor != 0 )
  {
    scale_factor = static_cast<OutputType>(options.output_scale_factor);
  }
  scale_factor = scale_factor / sum;

  // Fill in color commonality image from the compiled histogram
  const unsigned ni = input.ni();
  const unsigned nj = input.nj();
  const unsigned np = input.nplanes();
  const std::ptrdiff_t istep = input.istep();
  const std::ptrdiff_t jstep = input.jstep();
  const std::ptrdiff_t pstep = input.planestep();

  const InputType* row = input.top_left_ptr();
  for (unsigned j=0;j<nj;++j,row+=jstep)
  {
    const InputType* pixel = row;
    for (unsigned i=0;i<ni;++i,pixel+=istep)
    {
      const InputType* plane = pixel;
      unsigned step = 0;
      for (unsigned p=0;p<np;++p,plane+=pstep)
      {
        step += histsteps[p] * (*plane >> bitshift);
      }
      output(i,j) = scale_factor * (*(hist_top_left+step));
    }
  }
}

// Main function
template< class InputType, class OutputType >
void color_commonality_filter( const vil_image_view<InputType>& input,
                               vil_image_view<OutputType>& output,
                               const color_commonality_filter_settings& options )
{
  assert( std::numeric_limits<InputType>::is_integer );
  assert( is_power_of_two( options.resolution_per_channel ) );

  // Set output image size
  output.set_size( input.ni(), input.nj() );

  // If we are in grid mode, simply call this function recursively
  if( options.grid_image )
  {
    color_commonality_filter_settings recursive_options = options;
    recursive_options.grid_image = false;

    // Formulate grided regions
    unsigned ni = input.ni();
    unsigned nj = input.nj();

    for( unsigned j = 0; j < options.grid_resolution_height; j++ )
    {
      for( unsigned i = 0; i < options.grid_resolution_width; i++ )
      {
        // Top left point for region
        int ti = ( i * ni ) / options.grid_resolution_width;
        int tj = ( j * nj ) / options.grid_resolution_height;

        // Bottom right
        int bi = ( (i+1) * ni ) / options.grid_resolution_width;
        int bj = ( (j+1) * nj ) / options.grid_resolution_height;

        // Formulate rect region
        vgl_box_2d<int> region( ti, bi, tj, bj );

        vil_image_view<InputType> region_data_ptr = point_view_to_region( input, region );
        vil_image_view<OutputType> output_data_ptr = point_view_to_region( output, region );

        // Process rect region independent of one another
        color_commonality_filter( region_data_ptr, output_data_ptr, recursive_options );
      }
    }

    // We are done, we already processed the entire image
    return;
  }

  // Reset histogram (use internal or external version)
  unsigned int hist_size = options.resolution_per_channel;

  if( input.nplanes() == 3 )
  {
    hist_size = options.resolution_per_channel *
                options.resolution_per_channel *
                options.resolution_per_channel;
  }

  bool use_external_hist = ( options.histogram != NULL );

  std::vector<unsigned>* histogram = options.histogram;

  if( !use_external_hist )
  {
    histogram = new std::vector<unsigned>(hist_size);
  }

  histogram->resize( hist_size );

  std::fill( histogram->begin(), histogram->end(), 0 );

  // Fill in a color/intensity histogram of the input
  filter_color_image( input, output, *histogram, options );

  if( !use_external_hist )
  {
    delete histogram;
  }
}

}
