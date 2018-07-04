/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "detection_consolidator_process.h"

#include <vgl/vgl_intersection.h>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "detection_consolidator_process" );


detection_consolidator_process
::detection_consolidator_process( std::string const& _name )
  : process( _name, "detection_consolidator_process" ),
    config_(),
    disabled_( false ),
    mode_( MERGE )
{
  // Merging Parameters
  config_.add_parameter( "disabled",
                         "false",
                         "Should this process be disabled? When disabled no "
                         "detection will be passed downstream." );
  config_.add_parameter( "mode",
                         "prefer_appearance",
                         "Operation mode of the filter. Can either be merge or "
                         "prefer_appearance, the later of which removes any "
                         "motion detections which overlap appearance detections "
                         "instead of simply taking the union of detections.");
  config_.add_parameter( "suppr_overlap_per",
                         "0.30",
                         "If a detection overlaps more than this percent with "
                         "a prefered detection, do not send it downstream." );

  // Labeling Parameters
  config_.add_parameter( "motion_detection_label",
                         "motion_detector",
                         "Label given to input motion detections." );
  config_.add_parameter( "appearance1_detection_label",
                         "vehicle_dpm_detector",
                         "Label given to input appearance detections on the "
                         "first input port." );
  config_.add_parameter( "appearance2_detection_label",
                         "person_dpm_detector",
                         "Label given to input appearance detections on the "
                         "second input port." );
  config_.add_parameter( "appearance3_detection_label",
                         "vehicle_dpm_detector",
                         "Label given to input appearance detections on the "
                         "third input port." );
  config_.add_parameter( "appearance4_detection_label",
                         "person_dpm_detector",
                         "Label given to input appearance detections on the "
                         "fourth input port." );
}


detection_consolidator_process
::~detection_consolidator_process()
{
}


config_block
detection_consolidator_process
::params() const
{
  return config_;
}


bool
detection_consolidator_process
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );

    if( !disabled_ )
    {
      // Parse merging parameters
      suppr_overlap_per_ = blk.get<double>( "suppr_overlap_per" );

      std::string mode_str = blk.get<std::string>( "mode" );

      if( mode_str == "merge" )
      {
        mode_ = MERGE;
      }
      else if( mode_str == "prefer_appearance" )
      {
        mode_ = PREFER_APPEARANCE;
      }
      else
      {
        throw config_block_parse_error( "Invalid mode:" + mode_str );
      }

      // Parse labeling parameters
      motion_labeler_.reset( new set_type_functor(
        image_object::MOTION, blk.get<std::string>( "motion_detection_label" ) ) );

      appearance_labeler1_.reset( new set_type_functor(
        image_object::APPEARANCE, blk.get<std::string>( "appearance1_detection_label" ) ) );

      appearance_labeler2_.reset( new set_type_functor(
        image_object::APPEARANCE, blk.get<std::string>( "appearance2_detection_label" ) ) );

      appearance_labeler3_.reset( new set_type_functor(
        image_object::APPEARANCE, blk.get<std::string>( "appearance3_detection_label" ) ) );

      appearance_labeler4_.reset( new set_type_functor(
        image_object::APPEARANCE, blk.get<std::string>( "appearance4_detection_label" ) ) );
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( name() << ": couldn't set parameters, " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
detection_consolidator_process
::initialize()
{
  return true;
}


void
apply_set_type_functor( set_type_functor& f, std::vector< image_object_sptr>& vec )
{
  for( unsigned i = 0; i < vec.size(); ++i )
  {
    vec[i] = f( vec[i] );
  }
}


void
merge_detections_into( const std::vector< image_object_sptr >& source,
                       std::vector< image_object_sptr >& dest )
{
  dest.insert( dest.end(), source.begin(), source.end() );
}


void
deep_copy_detections( const std::vector< image_object_sptr >& source,
                      std::vector< image_object_sptr >& dest )
{
  for( unsigned i = 0; i < source.size(); ++i )
  {
    dest.push_back( image_object_sptr( new image_object( *source[i] ) ) );
  }
}


void
merge_if_small_overlap( const std::vector< image_object_sptr >& source,
                        const std::vector< image_object_sptr >& to_check,
                        std::vector< image_object_sptr >& output,
                        double percent_threshold )
{
  for( unsigned i = 0; i < source.size(); ++i )
  {
    const vgl_box_2d< unsigned >& source_rect = source[i]->get_bbox();
    bool found_overlap = false;

    for( unsigned j = 0; j < to_check.size(); ++j )
    {
      const vgl_box_2d< unsigned >& check_rect = to_check[j]->get_bbox();
      const vgl_box_2d< unsigned >& box_intersect = vgl_intersection( source_rect, check_rect );

      double source_area = static_cast<double>( source_rect.volume() );
      double check_area = static_cast<double>( check_rect.volume() );
      double intersect_area = static_cast<double>( box_intersect.volume() );

      if( ( check_area != 0.0 && intersect_area / check_area > percent_threshold ) ||
          ( source_area != 0.0 && intersect_area / source_area > percent_threshold ) )
      {
        found_overlap = true;
        break;
      }
    }

    if( !found_overlap )
    {
      output.push_back( source[i] );
    }
  }
}


bool
detection_consolidator_process
::step()
{
  merged_dets_.clear();
  merged_proj_dets_.clear();

  // Return no detections when disabled
  if( disabled_ )
  {
    return true;
  }

  // Assign labels to image objects (hard-coded for now)
  apply_set_type_functor( *motion_labeler_, motion_dets_ );
  apply_set_type_functor( *appearance_labeler1_, appearance_dets1_ );
  apply_set_type_functor( *appearance_labeler2_, appearance_dets2_ );
  apply_set_type_functor( *appearance_labeler3_, appearance_dets3_ );
  apply_set_type_functor( *appearance_labeler4_, appearance_dets4_ );

  // Performing merging operation
  if( mode_ == MERGE || mode_ == PREFER_APPEARANCE )
  {
    detection_list_t appearance_dets;
    detection_list_t copied_appearance_dets;

    merge_detections_into( appearance_dets1_, appearance_dets );
    merge_detections_into( appearance_dets2_, appearance_dets );
    merge_detections_into( appearance_dets3_, appearance_dets );
    merge_detections_into( appearance_dets4_, appearance_dets );

    deep_copy_detections( appearance_dets, copied_appearance_dets );

    merge_detections_into( appearance_dets, merged_dets_ );
    merge_detections_into( copied_appearance_dets, merged_proj_dets_ );

    if( mode_ == PREFER_APPEARANCE )
    {
      merge_if_small_overlap( motion_dets_, appearance_dets,
                              merged_dets_, suppr_overlap_per_ );
      merge_if_small_overlap( motion_proj_dets_, copied_appearance_dets,
                              merged_proj_dets_, suppr_overlap_per_ );
    }
    else
    {
      merge_detections_into( motion_dets_, merged_dets_ );
      merge_detections_into( motion_proj_dets_, merged_proj_dets_ );
    }
  }
  else
  {
    LOG_ERROR( "Invalid operating mode!" );
    return false;
  }

  return true;
}


void
detection_consolidator_process
::set_motion_detections( detection_list_t const& dets )
{
  motion_dets_ = dets;
}


void
detection_consolidator_process
::set_projected_motion_detections( detection_list_t const& dets )
{
  motion_proj_dets_ = dets;
}


void
detection_consolidator_process
::set_appearance_detections_1( detection_list_t const& dets )
{
  appearance_dets1_ = dets;
}


void
detection_consolidator_process
::set_appearance_detections_2( detection_list_t const& dets )
{
  appearance_dets2_ = dets;
}


void
detection_consolidator_process
::set_appearance_detections_3( detection_list_t const& dets )
{
  appearance_dets3_ = dets;
}


void
detection_consolidator_process
::set_appearance_detections_4( detection_list_t const& dets )
{
  appearance_dets4_ = dets;
}


detection_consolidator_process::detection_list_t
detection_consolidator_process
::merged_detections() const
{
  return merged_dets_;
}


detection_consolidator_process::detection_list_t
detection_consolidator_process
::merged_projected_detections() const
{
  return merged_proj_dets_;
}


} // end namespace vidtk
