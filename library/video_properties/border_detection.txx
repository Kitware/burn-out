/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "border_detection.h"

#include <video_transforms/automatic_white_balancing.h>

#include <utilities/point_view_to_region.h>

#include <vgl/vgl_intersection.h>
#include <vgl/vgl_point_2d.h>

#include <vil/vil_transpose.h>

#include <limits>
#include <algorithm>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "border_detection_txx" );

// A helper function which approximates the percentage of pixels near some
// value in some line. Unsafe if used incorrectly.
//
// Inputs: start - pointer to the start of the data
//         step - pointer step while iterating through the line
//         length - number of points to scan
//         value - center value we are searching for
//         variance - allowed variance from this center value
//
template< typename T >
inline double scan_line_single( const T* start,
                                const std::ptrdiff_t& step,
                                const int& length,
                                const T& value,
                                const T& allowed_variance )
{
  LOG_ASSERT( start != NULL, "Received NULL pointer" );
  LOG_ASSERT( length != 0 , "Length must exceed 0" );

  // Initialize variables required for pass
  int count = 0;
  const T* ptr = start;
  const T lower = value - allowed_variance;
  const T upper = value + allowed_variance;

  // Decide which bounds we need to check (depends on over/underflow)
  bool check_top_bound = ( upper > value );
  bool check_bot_bound = ( lower < value );

  // Don't check upper or lower bound if not required
  if( check_top_bound && check_bot_bound )
  {
    for( int i = 0; i < length; i++, ptr += step )
    {
      if( *ptr >= lower && *ptr <= upper )
      {
        count++;
      }
    }
  }
  else if( check_top_bound && !check_bot_bound )
  {
    for( int i = 0; i < length; i++, ptr += step )
    {
      if( *ptr <= upper )
      {
        count++;
      }
    }
  }
  else if( !check_top_bound && check_bot_bound )
  {
    for( int i = 0; i < length; i++, ptr += step )
    {
      if( *ptr >= lower )
      {
        count++;
      }
    }
  }
  else
  {
    return 1.0;
  }
  return static_cast<double>(count) / length;
}

// Detect some border of a certain color. Will exit quickly if the detected
// border is not found. The color pointer must point to a valid color, with
// the same number of planes as the input image. Threshold is the percentage
// of pixels required to be near the color in order for a row/col to be
// considered a border.
template <typename PixType>
void detect_colored_border( const vil_image_view< PixType >& src,
                            const PixType* color,
                            const PixType allowed_variance,
                            const double percent_threshold,
                            const unsigned invalid_count,
                            image_border& output )
{
  // Retrieve image properties
  const unsigned ni = src.ni();
  const unsigned nj = src.nj();
  const unsigned np = src.nplanes();
  const std::ptrdiff_t istep = src.istep();
  const std::ptrdiff_t jstep = src.jstep();

  // Validate input image size - returning when one of the dimensions is
  // 2 or less is a perfectly valid exit case, the image is just too small
  // to have a valid border.
  if( ni < 3 || nj < 3 )
  {
    return;
  }

  // Currently, the image is expected to be a 1 channel image, however it
  // will also function on multi-channeled ones by averaging the estimation
  // from each channel. Could be further improved but functions good enough
  // for now for just detecting rectangular colored components

  image_border net_border( 0, ni, 0, nj );

  for( unsigned p = 0; p < np; p++ )
  {
    unsigned borders[4] = { 0, ni, 0, nj };  // { left, right, top, bottom }

    // Handle left
    unsigned invalid_counter = 0;

    for( unsigned i = 0; i < ni/4; i++ )
    {
      const PixType* col_start = &src(i,0,p);
      if( scan_line_single( col_start, jstep, nj, color[p], allowed_variance ) > percent_threshold )
      {
        borders[0] = i+1;
      }
      else
      {
        invalid_counter++;

        if( invalid_counter >= invalid_count )
        {
          break;
        }
      }
    }
    // Handle right
    invalid_counter = 0;

    for( unsigned i = ni-1; i >= 3*ni/4; i-- )
    {
      const PixType* col_start = &src(i,0,p);
      if( scan_line_single( col_start, jstep, nj, color[p], allowed_variance ) > percent_threshold )
      {
        borders[1] = i;
      }
      else
      {
        invalid_counter++;

        if( invalid_counter >= invalid_count )
        {
          break;
        }
      }
    }
    // Handle top
    invalid_counter = 0;

    for( unsigned j = 0; j < nj/4; j++ )
    {
      const PixType* row_start = &src(0,j,p);
      if( scan_line_single( row_start, istep, ni, color[p], allowed_variance ) > percent_threshold )
      {
        borders[2] = j+1;
      }
      else
      {
        invalid_counter++;

        if( invalid_counter >= invalid_count )
        {
          break;
        }
      }
    }
    // Handle bottom
    invalid_counter = 0;

    for( unsigned j = nj-1; j >= 3*nj/4; j-- )
    {
      const PixType* row_start = &src(0,j,p);
      if( scan_line_single( row_start, istep, ni, color[p], allowed_variance ) > percent_threshold )
      {
        borders[3] = j;
      }
      else
      {
        invalid_counter++;

        if( invalid_counter >= invalid_count )
        {
          break;
        }
      }
    }

    // Formulate inner area estimate and intersect with other channels
    image_border planar_border_est( borders[0], borders[1], borders[2], borders[3] );
    net_border = vgl_intersection( planar_border_est, net_border );
  }

  output = net_border;
}

template <typename PixType>
void single_edge_col_search( const vil_image_view< PixType >& image,
                             const unsigned start_i, const unsigned end_i,
                             const unsigned start_j, const unsigned end_j,
                             std::vector< double >& percent_edges,
                             const PixType edge_threshold )
{
  const unsigned scan_width = end_i - start_i;
  const std::ptrdiff_t istep = image.istep();

  if( scan_width < 3 )
  {
    return;
  }

  std::vector< PixType > col_edges( scan_width );
  std::vector< unsigned > edge_counters( scan_width - 2, 0 );
  percent_edges = std::vector< double >( scan_width - 2, 0.0 );

  for( unsigned j = start_j; j < end_j; ++j )
  {
    const PixType *pos = &image( start_i, j );
    const PixType *next_pos;

    for( unsigned i = 0; i < scan_width; ++i, pos += istep )
    {
      next_pos = pos + istep;
      col_edges[i] = ( *pos > *next_pos ? *pos - *next_pos : *next_pos - *pos );
    }

    for( unsigned i = 0; i < edge_counters.size(); ++i )
    {
      if( col_edges[i+1] > col_edges[i] &&
          col_edges[i+1] > col_edges[i+2] &&
          col_edges[i+1] > edge_threshold )
      {
        edge_counters[i]++;
      }
    }
  }

  for( unsigned i = 0; i < percent_edges.size(); ++i )
  {
    percent_edges[i] = static_cast< double >( edge_counters[i] ) / ( end_j - start_j );
  }
}

bool compute_edge_adjustment( const std::vector< double >& measurements,
                              const double required_edge_percentage,
                              const int start_position,
                              int& new_position )
{
  double max_value = 0;
  unsigned max_position = 0;

  for( unsigned i = 0; i < measurements.size(); ++i )
  {
    if( measurements[i] > max_value )
    {
      max_value = measurements[i];
      max_position = i;
    }
  }

  if( max_value > required_edge_percentage )
  {
    new_position = max_position + start_position;
    return true;
  }

  return false;
}

// Refine a border based on detected edges near it
template <typename PixType>
void edge_refinement( const vil_image_view< PixType >& image,
                      image_border& border,
                      const unsigned input_search_radius,
                      const double required_edge_percentage,
                      const PixType edge_threshold )
{
  const int search_radius = static_cast< int >( input_search_radius + 2 );
  const int image_width = static_cast< int >( image.ni() );
  const int image_height = static_cast< int >( image.nj() );

  vil_image_view< PixType > image_tp = vil_transpose( image );
  std::vector< double > measurements;
  int adj_position;

  int left_start = ( border.min_x() > search_radius ? border.min_x() - search_radius : 0 );
  int left_end = std::min( border.min_x() + search_radius, image_width ) - 1;

  single_edge_col_search( image, left_start, left_end,
                          border.min_y(), border.max_y(),
                          measurements, edge_threshold );

  if( compute_edge_adjustment( measurements, required_edge_percentage,
                               left_start + 1, adj_position ) )
  {
    border.set_min_x( adj_position );
  }
  else
  {
    border.set_min_x( 0 );
  }

  int right_start = ( border.max_x() > search_radius ? border.max_x() - search_radius : 0 );
  int right_end = std::min( border.max_x() + search_radius, image_width ) - 1;

  single_edge_col_search( image, right_start, right_end,
                          border.min_y(), border.max_y(),
                          measurements, edge_threshold );

  if( compute_edge_adjustment( measurements, required_edge_percentage,
                               right_start + 1, adj_position ) )
  {
    border.set_max_x( adj_position );
  }
  else
  {
    border.set_max_x( image_width );
  }

  int top_start = ( border.min_y() > search_radius ? border.min_y() - search_radius : 0 );
  int top_end = std::min( border.min_y() + search_radius, image_height ) - 1;

  single_edge_col_search( image_tp, top_start, top_end,
                          border.min_x(), border.max_x(),
                          measurements, edge_threshold );

  if( compute_edge_adjustment( measurements, required_edge_percentage,
                               top_start + 1, adj_position ) )
  {
    border.set_min_y( adj_position );
  }
  else
  {
    border.set_min_y( 0 );
  }

  int bottom_start = ( border.max_y() > search_radius ? border.max_y() - search_radius : 0 );
  int bottom_end = std::min( border.max_y() + search_radius, image_height ) - 1;

  single_edge_col_search( image_tp, bottom_start, bottom_end,
                          border.min_x(), border.max_x(),
                          measurements, edge_threshold );

  if( compute_edge_adjustment( measurements, required_edge_percentage,
                               bottom_start + 1, adj_position ) )
  {
    border.set_max_y( adj_position );
  }
  else
  {
    border.set_max_y( image_height );
  }
}

// Core function to try and detect solid coloured image borders.
template< typename PixType >
void detect_solid_borders( const vil_image_view< PixType >& color_image,
                           const vil_image_view< PixType >& gray_image,
                           const border_detection_settings< PixType >& settings,
                           image_border& output )
{
  // Store property for later
  const bool is_hd = ( color_image.nj() > 480 );

  // Estimate desired border colors to scan for
  std::vector< std::vector< PixType > > colors_to_use;
  colors_to_use.resize( 1, std::vector< PixType >( color_image.nplanes() ) );

  switch( settings.detection_method_ )
  {
    case border_detection_settings< PixType >::AUTO:
    case border_detection_settings< PixType >::BLACK:
    {
      std::fill( colors_to_use[0].begin(),
                 colors_to_use[0].end(),
                 0 );
    }
    break;
    case border_detection_settings< PixType >::WHITE:
    {
      std::fill( colors_to_use[0].begin(),
                 colors_to_use[0].end(),
                 default_white_point<PixType>::value() );
    }
    break;
    case border_detection_settings< PixType >::SPECIFIED_COLOR:
    {
      LOG_ASSERT( settings.color_.size() == color_image.nplanes(), "Received invalid color" );
      colors_to_use[0] = settings.color_;
    }
    break;
  }

  // First, detect any potential coloured borders: detect colored border
  // will exit fast if a border of the specified color is not present (and
  // returns a rectangle the size of the image). For auto mode, first test
  // for black borders, then quickly estimates other potential colors.
  if( settings.detection_method_ == border_detection_settings< PixType >::AUTO &&
      color_image.ni() > 1 && color_image.nj() > 1 )
  {
    std::vector< PixType > side_color( color_image.nplanes() );
    std::vector< PixType > top_color( color_image.nplanes() );

    for( unsigned p = 0; p < color_image.nplanes(); p++ )
    {
      double sum1 = 0.0, sum2 = 0.0;

      sum1 += static_cast<double>( color_image( 0, color_image.nj() / 3, p ) );
      sum1 += static_cast<double>( color_image( 0, 2 * color_image.nj() / 3, p ) );
      sum1 += static_cast<double>( color_image( color_image.ni()-1, color_image.nj() / 3, p ) );
      sum1 += static_cast<double>( color_image( color_image.ni()-1, 2 * color_image.nj() / 3, p ) );

      sum2 += static_cast<double>( color_image( color_image.ni() / 3, 0, p ) );
      sum2 += static_cast<double>( color_image( 2 * color_image.ni() / 3, 0, p ) );
      sum2 += static_cast<double>( color_image( color_image.ni() / 3, color_image.nj()-1, p ) );
      sum2 += static_cast<double>( color_image( 2 * color_image.ni() / 3, color_image.nj()-1, p ) );

      side_color[p] = static_cast<PixType>( sum1 / 4.0 );
      top_color[p] = static_cast<PixType>( sum2 / 4.0 );
    }

    if( side_color != colors_to_use[0] )
    {
      colors_to_use.push_back( side_color );
    }

    if( top_color != side_color )
    {
      colors_to_use.push_back( top_color );
    }
  }

  if( settings.common_colors_only_ )
  {
    std::vector< std::vector< PixType > > colors_to_filter = colors_to_use;
    colors_to_use.clear();

    for( unsigned i = 0; i < colors_to_filter.size(); ++i )
    {
      std::vector< PixType >& color = colors_to_filter[i];

      if( color.size() == 3 )
      {
        PixType diff1 = ( color[1] > color[0] ? color[1] - color[0] : color[0] - color[1] );
        PixType diff2 = ( color[2] > color[0] ? color[2] - color[0] : color[0] - color[2] );
        PixType diff3 = ( color[2] > color[1] ? color[2] - color[1] : color[1] - color[2] );

        if( diff1 < 8 && diff2 < 8 && diff3 < 8 )
        {
          if( color[1] < 30 || ( color[1] > 80 && color[1] < 110 ) )
          {
            colors_to_use.push_back( color );
          }
        }
      }
      else
      {
        colors_to_use.push_back( color );
      }
    }
  }

  output = image_border( 0, color_image.ni(), 0, color_image.nj() );

  for( unsigned i = 0; i < colors_to_use.size(); ++i )
  {
    image_border detected_border;

    detect_colored_border( color_image,
                           &(colors_to_use[i][0]),
                           ( is_hd ? settings.high_res_variance_
                                   : settings.default_variance_ ),
                           settings.required_percentage_,
                           settings.invalid_count_,
                           detected_border );

    output = vgl_intersection( output, detected_border );
  }

  // Run edge refinement on detected border
  if( settings.edge_refinement_ )
  {
    edge_refinement( gray_image, output,
                     settings.edge_search_radius_,
                     settings.edge_percentage_req_,
                     settings.edge_threshold_ );
  }

  // Dilate detected border if enabled
  if( settings.side_dilation_ != 0 )
  {
    output.set_min_x( output.min_x() + settings.side_dilation_ );
    output.set_min_y( output.min_y() + settings.side_dilation_ );
    output.set_max_x( output.max_x() - settings.side_dilation_ );
    output.set_max_y( output.max_y() - settings.side_dilation_ );
  }
}

template< typename PixType >
void detect_solid_borders( const vil_image_view< PixType >& input_image,
                           const border_detection_settings< PixType >& settings,
                           image_border& output )
{
  detect_solid_borders( input_image, input_image, settings, output );
}

// Helper function for the below - checks if a pixel's value is in our
// desired fill range (near white or near black). For arbitrary number
// of planes, but could be optimized when np is known.
template< typename PixType >
inline bool bw_extrema_check( const PixType* pixel,
                              const unsigned nplanes,
                              const std::ptrdiff_t pstep,
                              const PixType black_thresh,
                              const PixType white_thresh )
{
  // Check if white
  if( *pixel < black_thresh )
  {
    for( unsigned p = 1; p < nplanes; p++ )
    {
      pixel += pstep;
      if( *pixel >= black_thresh )
      {
        return false;
      }
    }
    return true;
  }
  else if( *pixel > white_thresh )
  {
    for( unsigned p = 1; p < nplanes; p++ )
    {
      pixel += pstep;
      if( *pixel <= white_thresh )
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

// An alternative border detection function specialized for detecting a specific
// type of image border, one which can't be described by a rectangle and where all
// pure black or white pixels touching the image boundaries are considered border
// pixels.
template< typename PixType >
void detect_bw_nonrect_borders( const vil_image_view< PixType >& img,
                                image_border_mask& output,
                                const PixType tolerance )
{
  // Get image properties and reset the output mask
  const unsigned ni = img.ni();
  const unsigned ni_minus_1 = ni - 1;
  const unsigned nj = img.nj();
  const unsigned nj_minus_1 = nj - 1;
  const unsigned np = img.nplanes();

  output.set_size( ni, nj );
  output.fill( false );

  const std::ptrdiff_t spstep = img.planestep();

  const PixType black_thresh = tolerance + 1;
  const PixType white_thresh = default_white_point<PixType>::value() - tolerance - 1;

  // Exit case - image size too small
  if( ni < 4 || nj < 4 )
  {
    return;
  }

  // Pass 1 - establish a list of pixels to process on the first iteration,
  // and fill in any direct border pixels that are white or black
  std::vector< vgl_point_2d<unsigned> > points_to_process[2];
  for( unsigned i = 0; i < ni; i++ )
  {
    const PixType *top_pos = &img(i,0);
    const PixType *bot_pos = &img(i,nj_minus_1);
    if( bw_extrema_check( top_pos, np, spstep, black_thresh, white_thresh ) )
    {
      output(i,0) = true;
      points_to_process[0].push_back( vgl_point_2d<unsigned>(i,1) );
    }
    if( bw_extrema_check( bot_pos, np, spstep, black_thresh, white_thresh ) )
    {
      output(i,nj_minus_1) = true;
      points_to_process[0].push_back( vgl_point_2d<unsigned>(i,nj_minus_1-1) );
    }
  }
  for( unsigned j = 0; j < nj; j++ )
  {
    const PixType *left_pos = &img(0,j);
    const PixType *right_pos = &img(ni_minus_1,j);
    if( bw_extrema_check( left_pos, np, spstep, black_thresh, white_thresh ) )
    {
      output(0,j) = true;
      points_to_process[0].push_back( vgl_point_2d<unsigned>(1,j) );
    }
    if( bw_extrema_check( right_pos, np, spstep, black_thresh, white_thresh ) )
    {
      output(ni_minus_1,j) = true;
      points_to_process[0].push_back( vgl_point_2d<unsigned>(ni_minus_1-1,j) );
    }
  }

  // Pass 2 - iterate until we've filled in every point, could be made more
  // efficient later by removing operator() image accessor calls if required.
  unsigned buffer_pos = 0;
  unsigned next_buffer_pos = 1;

  while( points_to_process[buffer_pos].size() > 0 )
  {
    // Get pointers to our points to process on this iteration, and next iteration
    std::vector< vgl_point_2d<unsigned> >& this_itr = points_to_process[buffer_pos];
    std::vector< vgl_point_2d<unsigned> >& next_itr = points_to_process[next_buffer_pos];
    next_itr.clear();

    // Process points, and accumulate points to process on the next itr
    for( unsigned p = 0; p < this_itr.size(); p++ )
    {
      const unsigned i = this_itr[p].x();
      const unsigned j = this_itr[p].y();

      bool *pixel_status = &output( i, j );

      // Indicates this pixel still needs to be checked on this iteration
      if( !(*pixel_status) )
      {
        const PixType *pos = &img( i, j );

        if( bw_extrema_check( pos, np, spstep, black_thresh, white_thresh ) )
        {
          *pixel_status = true;
          next_itr.push_back( vgl_point_2d<unsigned>(i,j-1) );
          next_itr.push_back( vgl_point_2d<unsigned>(i,j+1) );
          next_itr.push_back( vgl_point_2d<unsigned>(i-1,j) );
          next_itr.push_back( vgl_point_2d<unsigned>(i+1,j) );
        }
      }
    }

    // Swap buffer positions
    std::swap( buffer_pos, next_buffer_pos );
  }
}

// Fill all pixels in some image border with a specified value.
template< typename PixType >
void fill_border_pixels( vil_image_view< PixType >& img,
                         const image_border& border,
                         const PixType value )
{
  // Cast image dimensions to int
  const int ni = static_cast<int>( img.ni() );
  const int nj = static_cast<int>( img.nj() );

  // Early exit case
  if( border.min_x() == 0 && border.min_y() == 0 &&
      border.max_x() == ni && border.max_y() == nj )
  {
    return;
  }

  // Identify 4 regions to fill
  image_border region1( 0, ni, 0, border.min_y() );
  image_border region2( 0, ni, border.max_y(), nj );
  image_border region3( 0, border.min_x(), border.min_y(), border.max_y() );
  image_border region4( border.max_x(), ni, border.min_y(), border.max_y() );

  // Fill regions
  if( region1.area() > 0 )
  {
    point_view_to_region( img, region1 ).fill( value );
  }
  if( region2.area() > 0 )
  {
    point_view_to_region( img, region2 ).fill( value );
  }
  if( region3.area() > 0 )
  {
    point_view_to_region( img, region3 ).fill( value );
  }
  if( region4.area() > 0 )
  {
    point_view_to_region( img, region4 ).fill( value );
  }
}

} // end namespace vidtk
