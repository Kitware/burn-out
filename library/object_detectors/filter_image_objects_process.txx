/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_image_objects_process.h"

#include <boost/bind.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <fstream>
#include <algorithm>

#include <vgl/vgl_area.h>
#include <vil/vil_crop.h>
#include <vil/vil_load.h>
#include <vgl/vgl_intersection.h>

#include <tracking_data/tracking_keys.h>
#include <object_detectors/ghost_detector.h>
#include <geographic/geo_coords.h>
#include <utilities/homography_util.h>
#include <utilities/blank_line_filter.h>
#include <utilities/shell_comments_filter.h>

#include <logger/logger.h>
VIDTK_LOGGER("filter_image_objects_process");

namespace vidtk
{

template <class PixType>
filter_image_objects_process< PixType >
::filter_image_objects_process( std::string const& _name )
  : process( _name, "filter_image_objects_process" ),
    grad_ghost_detector_( NULL ),
    src_objs_( NULL ),
    H_src2utm_(),
    sbf_valid_( false ),
    min_image_area_(-1),
    max_image_area_(-1),
    filter_used_in_track_( false ),
    used_in_track_( false ),
    ghost_gradients_( false ),
    min_ghost_var_( 0 ),
    pixel_padding_( 0 ),
    geofilter_enabled_( false ),
    max_occlusion_( 0.0 ),
    filter_unusable_frames_( false )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Turns the process OFF when 'true'." );
  config_.add_parameter(
    "min_area",
    "-1",
    "Drop all the detections with area (typically in m^2) LESS than 'min_area'."
    " Ignored for negative values. " );
  config_.add_parameter(
    "min_image_area",
    "-1",
    "Drop all the detections with pixel area LESS than 'min_area'."
    " Ignored for negative values. " );
  config_.add_parameter(
    "max_area",
    "-1",
    "Drop all the detections with area (typically in m^2) MORE than 'max_area'."
    " Ignored for negative values. " );
  config_.add_parameter(
    "max_image_area",
    "-1",
    "Drop all the detections with pixel area MORE than 'max_area'."
    " Ignored for negative values. " );
  config_.add_parameter(
    "max_aspect_ratio",
    "-1",
    "Drop all the detections with the aspect raio of image bounding box more than this value."
    " Ignored for negative values. " );
  config_.add_parameter(
    "min_occupied_bbox",
    "-1",
    "Drop all the detections with raio of boundary area to box area less than this value."
    " Ignored for negative values. " );
  config_.add_parameter(
    "filter_used_in_track",
    "false",
    "Whether to filter based on *used_in_track* field or not?" );
  config_.add_parameter(
    "used_in_track",
    "true",
    "Remove objects with specified (true/false) used_in_track key value. "
    "For example, if this value is set to TRUE, then all objects that *are* used in track will be removed." );
  config_.add_parameter(
    "ghost_detection:enabled",
    "false",
    "Remove objects that show gradient signature of a ghost/shadow detection.");
  config_.add_parameter(
    "ghost_detection:min_grad_mag_var",
    "15.0",
    "Threshold below which MODs are labelled as ghosts. See "
    "object_detectors/ghost_detector.h for detailed comments." );
  config_.add_parameter(
    "ghost_detection:pixel_padding",
    "2",
    "Thickness of pixel padding around the MOD." );
  config_.add_parameter(
    "filter_binary_image",
    "false",
    "Whether to filter based on a supplied binary image or not?" );
  config_.add_parameter(
    "geofilter_enabled",
    "false",
    "Use the polygon to exclude a spatial region\n");
  config_.add_parameter(
    "geofilter_polygon",
    "",
    "Defines the polygon region inside which, image_objects are excluded. "
    "Since this is a polygon, list the points in CCW order. "
    "Points should be in lon/lat. "
    "The polygon auto-closes, so don't repeat the first point. "
    "So, you only need four points to define a bounding box. "
    "p1X p1Y p2X p2Y .. pnX pnY.\n");
  config_.add_parameter(
    "depth_filelist",
    "",
    "File list of depth images for track filtering" );
  config_.add_parameter(
    "depth_filter_thresh",
    "-1",
    "Minimum depth value to keep a track (0-255)" );
  config_.add_parameter(
    "max_occlusion",
    "0.0",
    "Remove objects occluding others by more than this percentage of this occluded area (within [0,1])." );
  config_.add_parameter(
    "filter_unusable_frames",
    "false",
    "Remove detections from unusable frames." );
}


template <class PixType>
filter_image_objects_process< PixType >
::~filter_image_objects_process()
{
  delete grad_ghost_detector_;
}


template <class PixType>
config_block
filter_image_objects_process< PixType >
::params() const
{
  return config_;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    min_image_area_ = blk.get<double>( "min_image_area" );
    max_image_area_ = blk.get<double>( "max_image_area" );
    min_area_ = blk.get<double>( "min_area" );
    max_area_ = blk.get<double>( "max_area" );
    max_aspect_ratio_ = blk.get<double>( "max_aspect_ratio" );
    min_occupied_bbox_ = blk.get<double>( "min_occupied_bbox" );
    filter_used_in_track_ = blk.get<bool>( "filter_used_in_track" );
    filter_binary_ = blk.get<bool>( "filter_binary_image" );
    used_in_track_ = blk.get<bool>( "used_in_track" );
    disabled_ = blk.get<bool>( "disabled" );
    ghost_gradients_ = blk.get<bool>( "ghost_detection:enabled" );
    min_ghost_var_ = blk.get<float>( "ghost_detection:min_grad_mag_var" );
    pixel_padding_ = blk.get<unsigned>( "ghost_detection:pixel_padding" );
    geofilter_enabled_ = blk.get<bool>( "geofilter_enabled" );
    if ( geofilter_enabled_ )
    {
      set_aoi( blk.get<std::string>( "geofilter_polygon" ) );
    }
    depth_filter_thresh_ = blk.get<double>( "depth_filter_thresh" );

    std::string depth_file_list = blk.get<std::string> ( "depth_filelist" );
    setup_depth_filtering(depth_file_list);

    max_occlusion_ = blk.get<float>( "max_occlusion" );
    filter_unusable_frames_ = blk.get<bool>( "filter_unusable_frames" );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
void
filter_image_objects_process< PixType >
::setup_depth_filtering( std::string const& depth_file_list )
{
  if (!depth_file_list.empty())
  {
    depth_file_names_.clear();
    std::ifstream infile(depth_file_list.c_str());
    if (!infile)
    {
      throw config_block_parse_error( ": depth file specified and not found: " + depth_file_list );
    }

    while (infile.good())
    {
      std::string line;
      std::getline(infile, line);
      depth_file_names_.push_back(line);
    }
  }
  else
  {
    //If no depth file list then turn off depth filtering
    depth_filter_thresh_ = -1.0;
  }
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::initialize()
{
  if( ghost_gradients_ )
  {
    grad_ghost_detector_ = new ghost_detector< PixType >( min_ghost_var_,
                                                          pixel_padding_ );
  }

  depth_file_index_ = 0;

  return true;
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::reset()
{
  delete grad_ghost_detector_;

  //Needs to be set to NULL because ghost_gradients_ could have changed.
  grad_ghost_detector_ = NULL;
  return this->initialize();
}


template <class PixType>
bool
filter_image_objects_process< PixType >
::step()
{
  if( disabled_ )
  {
    //send the src objects through
    objs_ = *src_objs_;
    return true;
  }

  LOG_ASSERT( src_objs_ != NULL, "Source objects not set" );
  LOG_ASSERT( !ghost_gradients_ || src_img_, "Source image required when "
              "performing ghost detection." );
  LOG_ASSERT( !filter_binary_ || binary_img_ != NULL, "Binary image "
              "required when performing binary filtering." );

  objs_ = *src_objs_;
  std::vector< image_object_sptr >::iterator begin = objs_.begin();
  std::vector< image_object_sptr >::iterator end = objs_.end();

  if( min_image_area_ >= 0.0 )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_min_image_area, this, _1 ) );
  }

  if( max_image_area_ >= 0.0 )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_max_image_area, this, _1 ) );
  }

  if( min_area_ >= 0.0 )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_min_area, this, _1 ) );
  }

  if( max_area_ >= 0.0 )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_max_area, this, _1 ) );
  }

  if( max_aspect_ratio_ >= 0.0 )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_max_aspect_ratio, this, _1 ) );
  }

  if( min_occupied_bbox_ >= 0.0 )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_min_occupied_bbox, this, _1 ) );
  }

  if( filter_binary_ )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_binary_image, this, _1 ) );
  }

  if( filter_used_in_track_ )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_used_in_track, this, _1 ) );
  }

  if( ghost_gradients_ )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_ghost_gradients, this, _1 ) );
  }

  if ( geofilter_enabled_ )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_intersects_region, this, _1 ) );
  }

  if (depth_filter_thresh_ >= 0.0)
  {
    vil_image_view<vxl_byte> depth_byte;
    assert(depth_file_index_ < depth_file_names_.size());
    depth_byte = vil_load(depth_file_names_[depth_file_index_++].c_str());

    end = std::remove_if( begin, end,
      boost::bind( &self_type::filter_out_with_depth, this, _1, depth_byte) );
  }

  if( max_occlusion_ > 0 )
  {
    end = std::remove_if( begin, end,
                          boost::bind( &self_type::filter_out_max_occlusion, this, _1 ) );
  }

  // If frame is not usable, then remove all objects
  if ( filter_unusable_frames_
       && this->sbf_valid_
       && (! this->shot_break_flags_.is_frame_usable() ) )
  {
    end = begin;
  }

  // Actually remove the filtered out elements.
  objs_.erase( end, objs_.end() );

  // prepare for the next set
  src_objs_ = NULL;

  return true;
}

// -- INPUTS --
template <class PixType>
void
filter_image_objects_process< PixType >
::set_source_objects( std::vector< image_object_sptr > const& objs )
{
  src_objs_ = &objs;
}

template <class PixType>
void
filter_image_objects_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  src_img_ = &img;
}

template <class PixType>
void
filter_image_objects_process< PixType >
::set_binary_image( vil_image_view< bool > const& img )
{
  binary_img_ = &img;
}

template <class PixType >
void
filter_image_objects_process<PixType>
::set_src_to_utm_homography( image_to_utm_homography const& H )
{
  H_src2utm_ = H;
}

template <class PixType>
void
filter_image_objects_process< PixType >
::set_shot_break_flags( shot_break_flags const& sbf )
{
  this->shot_break_flags_ = sbf;
  this->sbf_valid_ = true;
}

// -- OUTPUTS --
template <class PixType>
std::vector< image_object_sptr >
filter_image_objects_process< PixType >
::objects() const
{
  return objs_;
}


// -- support methods/predicates --
template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_min_image_area( image_object_sptr const& obj ) const
{
  return obj->get_image_area() < min_image_area_;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_max_image_area( image_object_sptr const& obj ) const
{
  return obj->get_image_area() > max_image_area_;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_min_area( image_object_sptr const& obj ) const
{
  return obj->get_area() < min_area_;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_max_area( image_object_sptr const& obj ) const
{
  return obj->get_area() > max_area_;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_max_aspect_ratio( image_object_sptr const& obj ) const
{
  vgl_box_2d< unsigned > const& bbox = obj->get_bbox();
  double maxWH = std::max( bbox.width(), bbox.height() );
  double minWH = std::min( bbox.width(), bbox.height() );

  if( minWH == 0 )
  {
    return true;
  }
  if( ( maxWH / minWH ) > max_aspect_ratio_ )
  {
    return true;
  }

  return false;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_min_occupied_bbox( image_object_sptr const& obj ) const
{
  //Both areas have to be in pixels or meters.
  double area_ratio = vgl_area( obj->get_boundary() ) / obj->get_bbox().volume();

  return area_ratio < min_occupied_bbox_;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_used_in_track( image_object_sptr const& obj ) const
{
  //Remove the object if it matches the specified (used_in_track_) criteria
  return (obj->is_used_in_track() == used_in_track_);
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_ghost_gradients( image_object_sptr const& obj ) const
{
  //Remove the object if gradient signature matches that of a ghost/shadow MOD.
  return ( grad_ghost_detector_->has_ghost_gradients( obj, *src_img_ ) );
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_binary_image( image_object_sptr const& obj ) const
{
  //Crop the section of the diff image corresponding to this object
  vgl_box_2d< unsigned > const& bbox = obj->get_bbox();
  vil_image_view< bool > crop_mask = vil_crop( *binary_img_,
                                               bbox.min_x(),
                                               bbox.width(),
                                               bbox.min_y(),
                                               bbox.height() );

  //Remove object if no pixel in the object is above the given threshold
  //(we're dealing with the diff image  data)
  vil_image_view< bool > obj_mask;
  image_object::image_point_type origin;
  obj->get_object_mask( obj_mask, origin );
  for( unsigned int j = 0; j < crop_mask.nj(); j++ )
  {
    for( unsigned int i = 0; i < crop_mask.ni(); i++ )
    {
      if( obj_mask( i, j ) == true && crop_mask( i, j ) == true )
      {
        return false;
      }
    }
  }

  //If didn't find any pixels above threshold
  return true;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_intersects_region( image_object_sptr const& obj ) const
{
  if (! H_src2utm_.is_valid() )
  {
    LOG_WARN("No image_to_utm_homography provided for geofiltering."
             " Either provide one or disable geofiltering.");
    return false;
  }

  // @todo: Remove use of geographic
  // @todo: Currently this calc is throwaway. Persist for later use.
  vidtk_pixel_coord_type const& img_loc = obj->get_image_loc();
  vgl_homg_point_2d< double > image_loc( img_loc[0], img_loc[1] );
  geographic::geo_coords geo_pt = H_src2utm_ * image_loc;
  vgl_point_2d<double> pt( geo_pt.longitude(), geo_pt.latitude() );

  // Remove when point is inside polygon
  if ( geofilter_polygon_.contains(pt) )
  {
    LOG_TRACE( this->name() << " Filtering point: " << pt );
    return true;
  }

  return false;
}

/*
  @todo:
  perhaps it would be much better to describe the aoi using json
  that would let us use utm/latlon/image/...

  @todo:
  This code is stolen from kinematic_track_linking_functor, take both bits and move to aoi?
*/
template <class PixType>
void
filter_image_objects_process< PixType >
::set_aoi( std::string const& regions )
{
  LOG_DEBUG( this->name() << " Geofiltering enabled" );
  if( regions.empty() )
  {
    throw config_block_parse_error(
      this->name() + " Enabled geofiltering but didn't provide a region" );
  }

  // Start with an empty polygon every time we're called. Perhaps regions has changed.
  geofilter_polygon_.clear();

  // if the first character is not a digit, '+', or '-' then it
  // is a file name.
  if (! isdigit(regions[0]) && (regions[0] != '-') && (regions[0] != '+') )
  {
    LOG_DEBUG( this->name() << " Geofilter region provided by file: " << regions );
    // open file
    std::ifstream reg_file( regions.c_str() );
    if (!reg_file)
    {
      throw config_block_parse_error(
        this->name() + " Unable to open bounding_region file '" + regions + "'" );
    }

    boost::iostreams::filtering_istream in_stream;
    in_stream.push (vidtk::blank_line_filter());
    in_stream.push (vidtk::shell_comments_filter());
    in_stream.push (reg_file);

    // read file until eof
    std::string line;
    while ( std::getline(in_stream, line) )
    {
      // pass each line to add the region as a separate sheet
      LOG_TRACE( this->name() << " Adding polygon: " << line );
      this->add_region(line);
    }
  }
  else
  {
    LOG_DEBUG( this->name() << " Geofilter region provided directly by config" );
    LOG_TRACE( this->name() << " Adding polygon: " << regions );
    this->add_region(regions);
  }
}

template <class PixType>
void
filter_image_objects_process< PixType >
::add_region( std::string const & points )
{
  std::stringstream point_stream;
  point_stream.setf(std::ios::fixed, std::ios::floatfield);
  point_stream.precision(14);
  point_stream.str( points );
  geofilter_polygon_.new_sheet();

  while( !point_stream.eof() )
  {
    double x,y;
    point_stream >> x;
    if( !point_stream.good() )
    {
      throw config_block_parse_error("Could not parse the XY pair in filter region.");
    }
    point_stream >> y;

    geofilter_polygon_.push_back(x, y);
  } // end processing points
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_with_depth( image_object_sptr const& obj, const vil_image_view<vxl_byte> &depth_byte ) const
{
  vgl_box_2d<unsigned int> const& box = obj->get_bbox();
  for (unsigned int j = box.min_y(); j < box.max_y(); j++)
  {
    for (unsigned int i = box.min_x(); i < box.max_x(); i++)
    {
      if (depth_byte(i,j) > depth_filter_thresh_)
      {
        return true;
      }
    }
  }

  return false;
}

template <class PixType>
bool
filter_image_objects_process< PixType >
::filter_out_max_occlusion( image_object_sptr const& obj ) const
{
  std::vector< image_object_sptr >::const_iterator begin = objs_.begin();
  std::vector< image_object_sptr >::const_iterator end = objs_.end();
  for(std::vector< image_object_sptr >::const_iterator i=begin; i != end; ++i)
  {
    if( *i == obj ) // Exclude self-match
    {
      continue;
    }

    vgl_box_2d<unsigned> intersection = vgl_intersection((*i)->get_bbox(), obj->get_bbox());
    if (!intersection.is_empty()
        && (1.0 * intersection.volume() / (*i)->get_bbox().volume()) > max_occlusion_ )
    {
      // Remove 'obj' because we want to remove the *bigger* box occludding the smaller (*i->get_bbox()).
      return true;
    }
  }
  return false;
}

} // end namespace vidtk
