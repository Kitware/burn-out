/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "extract_masked_region_properties.h"

#include <limits>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "extract_masked_region_properties_txx" );


template< typename PixType >
PixType compute_average( const vil_image_view< PixType >& input,
                         const vil_image_view< bool >& mask )
{
  LOG_ASSERT( input.ni() == mask.ni(), "Input widths don't match" );
  LOG_ASSERT( input.nj() == mask.nj(), "Input heights don't match" );
  LOG_ASSERT( input.nplanes() == mask.nplanes(), "Input channels don't match" );

  if( input.size() == 0 )
  {
    return std::numeric_limits< PixType >::min();
  }

  double sum = 0.0;
  unsigned counter = 0;

  unsigned ni = input.ni();
  unsigned nj = input.nj();
  unsigned np = input.nplanes();

  for( unsigned p=0; p<np; ++p )
  {
    for( unsigned j=0; j<nj; ++j )
    {
      for( unsigned i=0; i<ni; ++i )
      {
        if( mask( i, j, p ) )
        {
          sum += static_cast< double >( input( i, j, p ) );
          counter++;
        }
      }
    }
  }

  if( counter != 0 )
  {
    sum = sum / counter;
  }

  return static_cast< PixType >( sum );
}

template< typename PixType >
PixType compute_minimum( const vil_image_view< PixType >& input,
                         const vil_image_view< bool >& mask )
{
  LOG_ASSERT( input.ni() == mask.ni(), "Input widths don't match" );
  LOG_ASSERT( input.nj() == mask.nj(), "Input heights don't match" );
  LOG_ASSERT( input.nplanes() == mask.nplanes(), "Input channels don't match" );

  if( input.size() == 0 )
  {
    return std::numeric_limits< PixType >::min();
  }

  PixType min_value = *( input.top_left_ptr() );

  unsigned ni = input.ni();
  unsigned nj = input.nj();
  unsigned np = input.nplanes();

  for( unsigned p=0; p<np; ++p )
  {
    for( unsigned j=0; j<nj; ++j )
    {
      for( unsigned i=0; i<ni; ++i )
      {
        if( mask( i, j, p ) && input( i, j, p ) < min_value )
        {
          min_value = input( i, j, p );
        }
      }
    }
  }

  return min_value;
}

}
