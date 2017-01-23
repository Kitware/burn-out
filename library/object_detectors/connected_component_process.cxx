/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "connected_component_process.h"

#include <vil/algo/vil_blob.h>
#include <vgl/vgl_convex.h>
#include <vgl/vgl_area.h>
#include <vil/vil_crop.h>

#include <utilities/polygon_centroid.h>
#include <tracking_data/image_object_util.h>

#include <video_properties/extract_masked_region_properties.h>

#include <boost/lexical_cast.hpp>

#include <limits>

#include <logger/logger.h>

VIDTK_LOGGER("connected_component_process_cxx");

namespace vidtk
{


connected_component_process
::connected_component_process( std::string const& _name )
  : process( _name, "connected_component_process" ),
    world_units_per_pixel_( 1.0 ),
    is_fg_good_( true ),
    disabled_( false ),
    min_confidence_( 0 ),
    max_confidence_( 255 )
{
  config_.add_parameter( "disabled",
    "false",
    "Whether or not to disable this process" );
  config_.add_parameter( "min_size",
    "0",
    "Minimum object size in pixels" );
  config_.add_parameter( "max_size",
    "1000000",
    "Maximum object size in pixels" );
  config_.add_parameter( "location_type",
    "centroid",
    "Location of the target for tracking: bottom or centroid. "
    "This parameter is used in conn_comp_sp:conn_comp, "
    "tracking_sp:tracker, and tracking_sp:state_to_image" );
  config_.add_parameter( "connectivity",
    "4",
    "Pixel connectivity used to label components" );
  config_.add_parameter( "confidence_method",
    "none",
    "Method for assigning confidence values to connected "
    "component blobs from the input heatmap. Can either be: "
    "none, average, or min" );
  config_.add_parameter( "min_confidence",
    boost::lexical_cast<std::string>( min_confidence_),
    "Minimum object confidence value to report" );
  config_.add_parameter( "max_confidence",
    boost::lexical_cast<std::string>( max_confidence_ ),
    "Maximum object confidence value to report" );
}


config_block
connected_component_process
::params() const
{
  return config_;
}


bool
connected_component_process
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );
    min_size_ = blk.get<double>( "min_size" );
    max_size_ = blk.get<double>( "max_size" );

    std::string loc = blk.get<std::string>( "location_type" );
    std::string conf = blk.get<std::string>( "confidence_method" );

    unsigned int conn_int = blk.get<unsigned>( "connectivity" );

    if( conn_int == 4 )
    {
      blob_connectivity_ = vil_blob_4_conn;
    }
    else if( conn_int == 8 )
    {
      blob_connectivity_ = vil_blob_8_conn;
    }
    else
    {
      throw config_block_parse_error( "Invalid blob connectivity type "
                                      "\"" + boost::lexical_cast<std::string>(conn_int) + "\". "
                                      "Expect \"4\" or \"8\"." );
    }

    if( loc == "centroid" )
    {
      loc_type_ = centroid;
    }
    else if( loc == "bottom" )
    {
      loc_type_ = bottom;
    }
    else
    {
      throw config_block_parse_error( "Invalid location type \"" + loc + "\". "
                                      "Expect \"centroid\" or \"bottom\"." );
    }

    if( conf == "none" )
    {
      confidence_method_ = none;
    }
    else if( conf == "average" )
    {
      confidence_method_ = average;
    }
    else if( conf == "min" )
    {
      confidence_method_ = min;
    }
    else
    {
      throw config_block_parse_error( "Invalid confidence method " + conf );
    }
  }
  catch( const config_block_parse_error &e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );

  return true;
}


bool
connected_component_process
::initialize()
{
  return true;
}


bool
connected_component_process
::step()
{
  objs_.clear();

  if( disabled_ )
  {
    return true;
  }

  // Return nothing if input indicated as invalid
  if( !is_fg_good_ )
  {
    return true;
  }

  // Validate inputs
  if( !fg_img_ )
  {
    LOG_ERROR( "Invalid input mask image" );
    return false;
  }

  if( confidence_method_ != none && !heatmap_img_ )
  {
    LOG_ERROR( "Cannot produce confidence values without a valid input heatmap image" );
    return false;
  }

  // Perform connected components
  vil_image_view<unsigned> labels_whole;
  std::vector<vil_blob_region> blobs;
  std::vector<vil_blob_region>::iterator it_blobs;

  vil_blob_labels( fg_img_, blob_connectivity_, labels_whole );
  vil_blob_labels_to_regions( labels_whole, blobs );

  float conf_range = max_confidence_ - min_confidence_;
  // Fill in information about each blob
  for( it_blobs = blobs.begin(); it_blobs < blobs.end(); ++it_blobs )
  {
    image_object_sptr obj_sptr = new image_object();
    image_object& obj = *obj_sptr; // to avoid the costly dereference every time.

    std::vector< vgl_point_2d< float_type > > pts;
    vil_blob_region::iterator it_chords;

    vgl_box_2d< unsigned > bbox = obj.get_bbox();
    for( it_chords = it_blobs->begin(); it_chords < it_blobs->end(); ++it_chords )
    {
      bbox.add( image_object::image_point_type( it_chords->ilo, it_chords->j ) );
      bbox.add( image_object::image_point_type( it_chords->ihi, it_chords->j ) );

      // Add points to create sheet
      // Since polygon coordinates are point-based, need to add leftmost and rightmost points
      // (both top and bottom of each pixel).
      // Note there *can* be repeated points. We assume our boundary algorithm can handle this.
      pts.push_back( vgl_point_2d<float_type>( it_chords->ilo, it_chords->j ) );
      pts.push_back( vgl_point_2d<float_type>( it_chords->ilo, it_chords->j + 1 ) );
      pts.push_back( vgl_point_2d<float_type>( it_chords->ihi + 1, it_chords->j ) );
      pts.push_back( vgl_point_2d<float_type>( it_chords->ihi + 1, it_chords->j + 1) );
    }

    // Move the max point, so that min_point() is inclusive and
    // max_point() is exclusive.
    bbox.set_max_x( bbox.max_x() + 1 );
    bbox.set_max_y( bbox.max_y() + 1 );
    obj.set_bbox( bbox );

    //Computing the boundary. See library/tracking_data/image_object.h for a definition of
    //the boundary form.
    obj.set_boundary( vgl_convex_hull( pts ) );

    float_type area = vil_area( *it_blobs );
    obj.set_image_area( area );

    vil_image_view< bool > mask_chip;
    mask_chip.deep_copy( clip_image_for_detection( fg_img_, obj_sptr ) );
    obj.set_object_mask(
      mask_chip, image_object::image_point_type( obj.get_bbox().min_x(), obj.get_bbox().min_y() ));

    if( min_size_ <= area && area <= max_size_ )
    {
      switch( loc_type_ )
      {
        case centroid:
          obj.set_image_loc( polygon_centroid( obj.get_boundary() ) );
          break;

        case bottom:
          float_type x = ( obj.get_bbox().min_x() + obj.get_bbox().max_x() ) / float_type( 2 );
          float_type y = float_type( obj.get_bbox().max_y() );
          obj.set_image_loc( x, y );
          break;
      }

      obj.set_world_loc( obj.get_image_loc()[0], obj.get_image_loc()[1], 0 );

      if( confidence_method_ != none )
      {
        float confidence = 0.0f;

        vil_image_view< float > heatmap_chip =
          clip_image_for_detection( heatmap_img_, obj_sptr );

        if( confidence_method_ == average )
        {
          confidence = compute_average( heatmap_chip, mask_chip );
        }
        else if( confidence_method_ == min )
        {
          confidence = compute_minimum( heatmap_chip, mask_chip );
        }

        // Normalize confidence to [0,1] range
        if(conf_range > 0.0)
        {
          confidence = (confidence - min_confidence_) / conf_range;
        }
        // Truncate to make sure that user-supplied [min,max] is not violated
        confidence = std::min( std::max( confidence, min_confidence_ ), max_confidence_ );

        obj_sptr->set_confidence( confidence );
      }

      objs_.push_back( obj_sptr );
    }
  }

#ifdef PRINT_DEBUG_INFO
  LOG_INFO( this->name() << ": Produced " << objs_.size() << " objects." );
#endif

  return true;
}


void
connected_component_process
::set_fg_image( vil_image_view< bool > const& img )
{
  fg_img_ = img;
}


void
connected_component_process
::set_heatmap_image( vil_image_view< float > const& img )
{
  heatmap_img_ = img;
}


std::vector< image_object_sptr >
connected_component_process
::objects() const
{
  return objs_;
}


void
connected_component_process
::set_world_units_per_pixel( double val )
{
  world_units_per_pixel_ = val;
}

void
connected_component_process
::set_is_fg_good( bool val )
{
  is_fg_good_ = val;
}

} // end namespace vidtk
