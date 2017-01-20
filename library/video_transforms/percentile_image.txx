/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "percentile_image.h"

#include <vil/vil_math.h>
#include <vil/vil_plane.h>
#include <vil/algo/vil_threshold.h>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER("percentile_image");


// Calculate the values of our image percentiles from x sampling points
template <typename PixType>
void sample_and_sort_image( const vil_image_view< PixType >& src,
                            std::vector< PixType >& dst,
                            unsigned int sampling_points )
{
  LOG_ASSERT( src.nplanes() == 1, "Input must contain a single plane" );

  if( src.size() < sampling_points )
  {
    sampling_points = src.size();
  }

  const unsigned scanning_area = src.size();
  const unsigned ni = src.ni();
  const unsigned nj = src.nj();
  const unsigned pixel_step = scanning_area / sampling_points;

  dst.resize( sampling_points );

  unsigned position = 0;

  for( unsigned pt = 0; pt < sampling_points; pt++, position += pixel_step )
  {
    unsigned i = position % ni;
    unsigned j = ( position / ni ) % nj;
    dst[pt] = src( i, j );
  }

  std::sort( dst.begin(), dst.end() );
}

template <typename PixType>
void get_image_percentiles( const vil_image_view< PixType >& src,
                            const std::vector< double >& percentiles,
                            std::vector< PixType >& dst,
                            unsigned sampling_points )
{
  LOG_ASSERT( percentiles.size() > 0, "No percentiles provided" );
  LOG_ASSERT( src.size() > 0, "Image is empty" );

  std::vector< PixType > sorted_samples;
  sample_and_sort_image( src, sorted_samples, sampling_points );
  dst.resize( percentiles.size() );

  double sampling_points_m1 = static_cast<double>(sorted_samples.size()-1);

  for( unsigned i = 0; i < percentiles.size(); i++ )
  {
    LOG_ASSERT( percentiles[i] >= 0 && percentiles[i] <= 1.0, "Percentile must be in range [0,1]" );

    unsigned ind = static_cast<unsigned>( sampling_points_m1 * percentiles[i] + 0.5 );
    dst[i] = sorted_samples[ind];
  }
}

template<typename PixType>
void percentile_threshold_above( const vil_image_view< PixType >& src,
                                 const std::vector< double >& percentiles,
                                 vil_image_view< bool >& dst,
                                 unsigned sampling_points )
{
  // Calculate thresholds
  std::vector< PixType > thresholds;
  get_image_percentiles( src, percentiles, thresholds, sampling_points );
  dst.set_size( src.ni(), src.nj(), percentiles.size() );

  // Perform thresholding
  for( unsigned i = 0; i < thresholds.size(); i++ )
  {
    vil_image_view< bool > output_plane = vil_plane( dst, i );
    vil_threshold_above( src, output_plane, thresholds[i] );
  }
}

template<typename PixType>
void percentile_threshold_above( const vil_image_view< PixType >& src,
                                 const double percentile,
                                 vil_image_view< bool >& dst,
                                 unsigned sampling_points )
{
  percentile_threshold_above( src,
                              std::vector<double>( 1, percentile ),
                              dst,
                              sampling_points );
}

} // end namespace vidtk
