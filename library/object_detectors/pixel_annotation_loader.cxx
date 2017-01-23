/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pixel_annotation_loader.h"

#include <fstream>

#include <vil/vil_load.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "pixel_annotation_loader" );

// Does a pixel in the image border a pure green pixel?
inline bool borders_green_pixel( const vil_image_view<vxl_byte>& gt,
                                 const int i,
                                 const int j )
{
  for( int dj = -1; dj <= 1; dj++ )
  {
    for( int di = -1; di <= 1; di++ )
    {
      int pi = i + di;
      int pj = j + dj;

      if( pi >= 0 && pi < static_cast<int>(gt.ni()) &&
          pj >= 0 && pj < static_cast<int>(gt.nj()) &&
          gt(pi,pj,0) == 0 && gt(pi,pj,1) == 255 && gt(pi,pj,2) == 0 )
      {
        return true;
      }
    }
  }
  return false;
}

// For grountruth images specified in RGB format, compute the correct gt id
inline unsigned convert_rgb_to_id( const vil_image_view<vxl_byte>& img, int i, int j )
{
  enum
  {
    BACKGROUND_ID = 0,
    GREEN_PIXEL_ID = 1,
    GREEN_PIXEL_BORDER_ID = 2,
    YELLOW_PIXEL_ID = 3,
    BLUE_PIXEL_ID = 4,
    RED_PIXEL_ID = 5,
    PINK_PIXEL_ID = 6
  };

  if( img(i,j,0) == 0 && img(i,j,1) == 255 && img(i,j,2) == 0 )
  {
    return GREEN_PIXEL_ID;
  }
  else if( img(i,j,0) == 255 && img(i,j,1) == 255 && img(i,j,2) == 0 )
  {
    return YELLOW_PIXEL_ID;
  }
  else if( img(i,j,0) == 0 && img(i,j,1) == 0 && img(i,j,2) == 255 )
  {
    return BLUE_PIXEL_ID;
  }
  else if( img(i,j,0) == 255 && img(i,j,1) == 0 && img(i,j,2) == 0 )
  {
    return RED_PIXEL_ID;
  }
  else if( img(i,j,0) == 255 && img(i,j,1) == 0 && img(i,j,2) == 255 )
  {
    return PINK_PIXEL_ID;
  }
  else if( borders_green_pixel( img, i, j ) )
  {
    return GREEN_PIXEL_BORDER_ID;
  }
  return BACKGROUND_ID;
}


bool
pixel_annotation_loader
::configure( const pixel_annotation_loader_settings& settings )
{
  if( settings.load_mode == pixel_annotation_loader_settings::FILELIST )
  {
    std::ifstream input( settings.path.c_str() );

    if( !input )
    {
      LOG_ERROR( "Unable to open " << settings.path );
      return false;
    }

    while( !input.eof() )
    {
      unsigned frame_id;
      std::string filename;

      input >> frame_id >> filename;

      if( !filename.empty() )
      {
        frame_to_filename_[ frame_id ] = filename;
      }
    }

    input.close();
  }
  else
  {
    pattern_ = boost::format( settings.pattern );
  }
  settings_ = settings;
  return true;
}

bool
load_gt_image( const std::string& filename,
               vil_image_view<unsigned>& ids,
               const bool is_mask_image )
{
  vil_image_view<vxl_byte> input_image = vil_load( filename.c_str() );

  if( !input_image )
  {
    return false;
  }

  ids.set_size( input_image.ni(), input_image.nj() );

  for( unsigned j = 0; j < ids.nj(); j++ )
  {
    for( unsigned i = 0; i < ids.ni(); i++ )
    {
      if( is_mask_image )
      {
        if( input_image( i, j, 0 ) > 0 )
        {
          ids( i, j ) = 1;
        }
        else
        {
          ids( i, j ) = 0;
        }
      }
      else
      {
        ids( i, j ) = convert_rgb_to_id( input_image, i, j );
      }
    }
  }

  return true;
}

bool
pixel_annotation_loader
::get_annotation_for_frame( const unsigned frame_number,
                            vil_image_view<unsigned>& ids )
{
  ids.clear();

  std::string filename;

  if( settings_.load_mode == pixel_annotation_loader_settings::FILELIST )
  {
    if( frame_to_filename_.find( frame_number ) != frame_to_filename_.end() )
    {
      filename = frame_to_filename_[ frame_number ];
    }
    else
    {
      return false;
    }
  }
  else
  {
    filename = settings_.path + "/" + boost::str( pattern_ % frame_number );
  }

  return load_gt_image(
    filename,
    ids,
    settings_.image_type == pixel_annotation_loader_settings::MASK );
}


}
