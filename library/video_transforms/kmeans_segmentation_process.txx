/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "kmeans_segmentation.h"
#include "kmeans_segmentation_process.h"

#include <utilities/point_view_to_region.h>

#include <logger/logger.h>


namespace vidtk
{


VIDTK_LOGGER ("kmeans_segmentation_process");


template < typename PixType, typename LabelType>
kmeans_segmentation_process<PixType,LabelType>
::kmeans_segmentation_process( std::string const& _name )
  : process( _name, "kmeans_segmentation_process" ),
    src_image_(NULL),
    src_border_(NULL),
    clusters_(6),
    sampling_points_(1000)
{
  config_.add_parameter(
    "clusters",
    "6",
    "Number of cluster centers to use." );

  config_.add_parameter(
    "sampling_points",
    "1000",
    "Number of pixels in the image to evenly sample." );
}


template < typename PixType, typename LabelType>
kmeans_segmentation_process<PixType,LabelType>
::~kmeans_segmentation_process()
{
}

template < typename PixType, typename LabelType>
config_block
kmeans_segmentation_process<PixType,LabelType>
::params() const
{
  return config_;
}


template < typename PixType, typename LabelType>
bool
kmeans_segmentation_process<PixType,LabelType>
::set_params( config_block const& blk )
{
  try
  {
    clusters_ = blk.get<unsigned>("clusters");
    sampling_points_ = blk.get<unsigned>("sampling_points");
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "  << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}

template < typename PixType, typename LabelType>
bool
kmeans_segmentation_process<PixType,LabelType>
::initialize()
{
  return true;
}

template < typename PixType, typename LabelType>
bool
kmeans_segmentation_process<PixType,LabelType>
::step()
{
  if( !src_image_ || src_image_->nplanes() != 1 )
  {
    LOG_ERROR( this->name() << ": Invalid input provided." );
    return false;
  }

  dst_image_ = vil_image_view<LabelType>( src_image_->ni(), src_image_->nj() );

  vil_image_view<PixType> input_region, output_region;

  if( src_border_ )
  {
    input_region = point_view_to_region( *src_image_, *src_border_ );
    output_region = point_view_to_region( dst_image_, *src_border_ );
  }
  else
  {
    input_region = *src_image_;
    output_region = dst_image_;
  }

  bool success = segment_image_kmeans( input_region,
                                       output_region,
                                       clusters_,
                                       sampling_points_ );

  return success;
}

// Input/Output Accessors
template <typename PixType, typename LabelType>
void
kmeans_segmentation_process<PixType,LabelType>
::set_source_image( vil_image_view<PixType> const& img )
{
  src_image_ = &img;
}

template <typename PixType, typename LabelType>
void
kmeans_segmentation_process<PixType,LabelType>
::set_source_border( image_border const& border )
{
  src_border_ = &border;
}

template <typename PixType, typename LabelType>
vil_image_view<LabelType>
kmeans_segmentation_process<PixType,LabelType>
::segmented_image() const
{
  return dst_image_;
}

} // end namespace vidtk
