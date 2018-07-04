/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pixel_feature_writer.h"

#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>

#include <fstream>
#include <map>

#include <utilities/point_view_to_region.h>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "pixel_feature_writer" );

template< typename PixType >
pixel_feature_writer< PixType >
::pixel_feature_writer()
{
  frame_counter_ = 0;
  border_pixels_to_ignore_ = 0;
  output_validation_image_ = true;
  blob_pixel_percentage_ = 0.5;
}

template< typename PixType >
bool
pixel_feature_writer< PixType >
::configure( const settings_t& settings )
{
  frame_counter_ = 0;
  border_pixels_to_ignore_ = settings.border_pixels_to_ignore;
  output_file_ = settings.output_file;
  output_validation_image_ = settings.output_validation_image;
  blob_pixel_percentage_ = settings.blob_pixel_percentage;
  return gt_loader_.configure( settings.gt_loader );
}

template< typename PixType >
bool
pixel_feature_writer< PixType >
::step( const feature_array_t& features, const image_border border )
{
  if( features.size() == 0 )
  {
    LOG_ERROR( "Feature vector length must exceed 0" );
    return false;
  }

  frame_counter_++;

  vil_image_view<unsigned> gt_image;

  if( !gt_loader_.get_annotation_for_frame( frame_counter_, gt_image ) )
  {
    LOG_INFO( "Could not find groundtruth for frame " << frame_counter_ );
    return false;
  }
  else
  {
    LOG_INFO( "Found groundtruth for frame " << frame_counter_ );
  }

  if( border.volume() > 0 )
  {
    gt_image = point_view_to_region( gt_image, border );
  }

  vil_image_view< vxl_byte > validation_frame;

  if( output_validation_image_ )
  {
    vil_convert_stretch_range( features[0], validation_frame );
  }

  // Open output file in append mode
  std::ofstream out( output_file_.c_str(), std::ios::app );

  // Write features and groundtruth to file
  const int ni = static_cast<int>(features[0].ni());
  const int nj = static_cast<int>(features[0].nj());
  const int ignore_count = static_cast<int>(border_pixels_to_ignore_);

  for( int j = ignore_count; j < nj - ignore_count; j++ )
  {
    for( int i = ignore_count; i < ni - ignore_count; i++ )
    {
      out << gt_image(i,j) << " ";

      if( output_validation_image_ && gt_image(i,j) > 0 )
      {
        validation_frame( i, j ) = 0;
      }

      for( unsigned k = 0; k < features.size(); k++ )
      {
        out << static_cast<int>(features[k](i,j)) << " ";
      }
      out << std::endl;
    }
  }

  // Print validation frame
  if( output_validation_image_ )
  {
    std::string val_fn = "validation-";
    val_fn = val_fn + boost::lexical_cast<std::string>( frame_counter_ );
    val_fn = val_fn + ".png";
    vil_save( validation_frame, val_fn.c_str() );
  }

  // Close filestream
  out.close();
  return true;
}

template< typename PixType >
bool
pixel_feature_writer< PixType >
::step( const std::vector< vil_blob_pixel_list >& blobs,
        const std::vector< std::vector< double > >& features,
        const image_border border )
{
  frame_counter_++;

  if( blobs.size() == 0 )
  {
    return false;
  }

  vil_image_view<unsigned> gt_image;

  if( !gt_loader_.get_annotation_for_frame( frame_counter_, gt_image ) )
  {
    LOG_INFO( "Could not find groundtruth for frame " << frame_counter_ );
    return false;
  }
  else
  {
    LOG_INFO( "Found groundtruth for frame " << frame_counter_ );
  }

  if( border.volume() > 0 )
  {
    gt_image = point_view_to_region( gt_image, border );
  }

  vil_image_view< vxl_byte > validation_frame;

  if( output_validation_image_ )
  {
    vil_convert_stretch_range( gt_image, validation_frame );
  }

  // Open output file in append mode
  std::ofstream out( output_file_.c_str(), std::ios::app );

  // Write features and groundtruth id to file
  for( unsigned i = 0; i < blobs.size(); ++i )
  {
    unsigned label = 0;

    std::map< unsigned, unsigned > label_counter;

    for( unsigned p = 0; p < blobs[i].size(); ++p )
    {
      label_counter[ gt_image( blobs[i][p].first, blobs[i][p].second ) ]++;

      if( output_validation_image_ )
      {
        validation_frame( blobs[i][p].first, blobs[i][p].second ) = 125;
      }
    }

    double max_ratio = 0.0;
    bool any_nonzero = false;

    for( std::map< unsigned, unsigned >::iterator p = label_counter.begin();
         p != label_counter.end(); ++p )
    {
      if( p->first == 0 )
      {
        continue;
      }
      else
      {
        any_nonzero = true;
      }

      double ratio = static_cast< double >( p->second ) / blobs[i].size();

      if( ratio > max_ratio && ratio > blob_pixel_percentage_ )
      {
        max_ratio = ratio;
        label = p->first;
      }
    }

    if( any_nonzero )
    {
      label = 2;
    }

    out << label << " ";
    for( unsigned f = 0; f < features[i].size(); ++f )
    {
      out << features[i][f] << " ";
    }
    out << std::endl;
  }

  // Print validation frame
  if( output_validation_image_ )
  {
    std::string val_fn = "validation-";
    val_fn = val_fn + boost::lexical_cast<std::string>( frame_counter_ );
    val_fn = val_fn + ".png";
    vil_save( validation_frame, val_fn.c_str() );
  }

  // Close filestream
  out.close();
  return true;
}

// Write feature images to a file
template< typename PixType >
bool
write_feature_images( const std::vector< vil_image_view< PixType> >& features,
                      const std::string prefix,
                      const std::string postfix,
                      const bool scale_features )
{
  bool ret_val = true;

  for( unsigned i = 0; i < features.size(); i++ )
  {
    vil_image_view<vxl_byte> writeme;

    std::string id = boost::lexical_cast<std::string>(i);
    std::string fn = prefix + id +  postfix;

    if( scale_features )
    {
      vil_convert_stretch_range( features[i], writeme );
    }
    else
    {
      writeme = features[i];
    }

    if( !vil_save( writeme, fn.c_str() ) )
    {
      ret_val = false;
    }
  }

  return ret_val;
}

}
