/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "metadata_text_parser_process.h"

#include <utilities/config_block.h>

#include <logger/logger.h>

#include <string>
#include <cmath>
#include <algorithm>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_math.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

namespace vidtk
{

VIDTK_LOGGER( "metadata_text_parser_process" );

const static double invalid_gsd = -1.0;


template <typename PixType>
metadata_text_parser_process<PixType>
::metadata_text_parser_process( std::string const& _name )
  : process( _name, "metadata_text_parser_process" ),
    adjust_fov_( true ),
    remove_existing_metadata_( false ),
    parse_on_valid_metadata_only_( false ),
    forced_target_width_multiplier_( -1.0 ),
    fov_multiplier_( 1.0 ),
    required_full_width_frame_count_( 10 ),
    frames_without_brackets_( 0 ),
    border_pixel_threshold_( 10 ),
    frame_consistency_threshold_( 1 ),
    consistency_measure_( 0.10 ),
    max_nonreport_frame_limit_( 0 ),
    backup_gsd_( invalid_gsd ),
    backup_gsd_enabled_( false ),
    frames_since_gsd_output_( 0 )
{
  config_.merge( text_parser_settings().config() );

  config_.add_parameter( "adjust_fov",
    "true",
    "Should the field of view parameters in the metadata "
    "packet be adjusted based on parsed values?" );

  config_.add_parameter( "remove_existing_metadata",
    "false",
    "Should all existing metadata (besides newly parsed values) be "
    "removed from the input packet?" );

  config_.add_parameter( "parse_on_valid_metadata_only",
    "false",
    "Should we only parse values when we have a valid "
    "metadata input packet?" );

  config_.add_parameter( "required_full_width_frame_count",
    "10",
    "Required number of frames without detecting any brackets "
    "before we can assume that the target width corresponds to "
    "the full image width as opposed to bracket width." );

  config_.add_parameter( "forced_target_width_multiplier",
    "-1.0",
    "If set to a positive value, this value will be used "
    "instead of any detected target width sizes, and represents "
    "how much we need to scale the target width to get to the  "
    "full image width." );

  config_.add_parameter( "fov_multiplier",
    "1.0",
    "Multiplier to scale the FOV values added into metadata packets. "
    "This is useful if the tracker is benefited by scaling it by them "
    "by some amount." );

  config_.add_parameter( "border_pixel_threshold",
    "20",
    "If the number of border pixels on either side of the image is greater "
    "than this number, the border will be used for computing GSDs "
    "from target widths when there are no brackets present." );

  config_.add_parameter( "frame_consistency_threshold",
    "1",
    "Number of frames required to have consistent GSD values in order "
    "to report one. This is used to drop potential errors in parsing." );

  config_.add_parameter( "consistency_measure",
    "0.10",
    "Required percentage that GSD values must be within w.r.t. each other "
    "in order to be considered consistent across frames." );

  config_.add_parameter( "max_nonreport_frame_limit",
    "0",
    "If there are no valid GSDs detected after this number of frames then "
    "either the last reported GSD within this number of frames (if present) "
    "followed by the backup GSD will be reported as the GSD." );

  config_.add_parameter( "backup_gsd",
    boost::lexical_cast< std::string >( invalid_gsd ),
    "If set to a positive number, this GSD will be used when no text "
    "is found on the screen after a certain number of frames." );
}


template <typename PixType>
metadata_text_parser_process<PixType>
::~metadata_text_parser_process()
{
}


template <typename PixType>
config_block
metadata_text_parser_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
metadata_text_parser_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    text_parser_settings settings( blk );

    if( !algorithm_.configure( settings ) )
    {
      throw config_block_parse_error( "Unable to configure algorithm" );
    }

    adjust_fov_ = blk.get<bool>( "adjust_fov" );
    remove_existing_metadata_ = blk.get<bool>( "remove_existing_metadata" );
    parse_on_valid_metadata_only_ = blk.get<bool>( "parse_on_valid_metadata_only" );
    required_full_width_frame_count_ = blk.get<unsigned>( "required_full_width_frame_count" );
    forced_target_width_multiplier_ = blk.get<double>( "forced_target_width_multiplier" );
    fov_multiplier_ = blk.get<double>( "fov_multiplier" );
    border_pixel_threshold_ = blk.get<int>( "border_pixel_threshold" );
    frame_consistency_threshold_ = blk.get<unsigned>( "frame_consistency_threshold" );
    consistency_measure_ = blk.get<double>( "consistency_measure" );
    max_nonreport_frame_limit_ = blk.get<unsigned>( "max_nonreport_frame_limit" );
    backup_gsd_ = blk.get<double>( "backup_gsd" );
    backup_gsd_enabled_ = ( backup_gsd_ > 0.0 || max_nonreport_frame_limit_ > 1 );

    computed_gsds_.clear();
    raw_frame_gsds_.clear();

    if( frame_consistency_threshold_ > 1 )
    {
      computed_gsds_.set_capacity( frame_consistency_threshold_ );
    }

    if( max_nonreport_frame_limit_ > 1 )
    {
      raw_frame_gsds_.set_capacity( max_nonreport_frame_limit_ );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
str_to_double( const std::string& str, double& out, const std::string ignore_chars = "" )
{
  std::string to_convert = str;

  for( unsigned i = 0; i < ignore_chars.size(); ++i )
  {
    std::string char_to_erase;
    char_to_erase.push_back( ignore_chars[i] );
    boost::erase_all( to_convert, char_to_erase );
  }

  try
  {
    out = boost::lexical_cast< double >( to_convert );
  }
  catch( const boost::bad_lexical_cast& )
  {
    return false;
  }

  return true;
}


template <typename PixType>
bool
metadata_text_parser_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
bool
metadata_text_parser_process<PixType>
::step()
{
  meta_out_ = meta_in_;
  parsed_strings_.clear();

  // Not inputing an image to run template matching on is a valid way
  // to say we don't want to do any parsing on the current frame.
  if( image_.size() == 0 )
  {
    return true;
  }

  const bool is_metadata_valid =
    !meta_in_.empty() &&
    meta_in_[0].is_valid() &&
    meta_in_[0].has_valid_time() &&
    meta_in_[0].has_valid_slant_range();

  if( parse_on_valid_metadata_only_ && !is_metadata_valid )
  {
    return true;
  }

  if( remove_existing_metadata_ )
  {
    meta_out_.resize( 1 );
    meta_out_[0] = video_metadata();
  }

  double gsd = invalid_gsd;
  std::string fov_level = "";

  for( unsigned i = 0; i < instructions_.size(); ++i )
  {
    std::string text = algorithm_.parse_text( image_, instructions_[i] );

    switch( instructions_[i].type )
    {
      case instruction_t::GROUND_SAMPLE_DISTANCE:
      {
        LOG_INFO( "Parsed GSD: " << text );

        // Note, ignoring characters "abcde" here because OSD character templates
        // may typically come in multiple variants such as "6.png", "6b.png",
        // "6c.png" and this allows them to still be casted to a double value
        // successfully.
        if( !str_to_double( text, gsd, "abcde" ) )
        {
          gsd = invalid_gsd;
        }

        break;
      }

      case instruction_t::TARGET_WIDTH:
      {
        LOG_INFO( "Parsed TWD: " << text );

        double twd;

        if( str_to_double( text, twd, "abcde" ) )
        {
          if( forced_target_width_multiplier_ > 0.0 )
          {
            gsd = twd * forced_target_width_multiplier_ / image_.ni();
          }
          else if( !widths_.empty()  )
          {
            unsigned max_width = *std::max_element( widths_.begin(), widths_.end() );

            LOG_INFO( "Using target pixel width: " << max_width );

            if( max_width != 0 )
            {
              gsd = twd / max_width;
            }

            frames_without_brackets_ = 0;
          }
          else if( frames_without_brackets_ > required_full_width_frame_count_ )
          {
            if( border_.area() > 0 && ( border_.min_x() > border_pixel_threshold_ ||
                border_.max_x() < static_cast<int>( image_.ni() ) - border_pixel_threshold_ ) )
            {
              gsd = twd / border_.width();
            }
            else
            {
              gsd = twd / image_.ni();
            }
          }
          else
          {
            frames_without_brackets_++;
          }
        }

        break;
      }

      case instruction_t::FIELD_OF_VIEW_LEVEL:
      {
        LOG_TRACE( "Parsed FOV: " << text );
        fov_level = text;
        break;
      }

      default:
      {
        LOG_TRACE( "Parsed text: " << text );
        break;
      }
    }

    if( !text.empty() )
    {
      parsed_strings_.push_back( text );
    }
  }

  // Determine if there is a new FOV, if so reset computed GSD buffer
  if( last_fov_level_ != fov_level )
  {
    computed_gsds_.clear();
  }

  // Add FOV level string to metadata
  if( !meta_out_.empty() )
  {
    meta_out_[0].camera_mode( fov_level );
  }

  // Record raw frame GSD for this frame, even if invalid.
  if( raw_frame_gsds_.capacity() > 0 )
  {
    raw_frame_gsds_.insert( gsd );
  }

  // Measure GSD consistency property, invalid GSD if not met
  if( gsd > 0.0 && computed_gsds_.capacity() > 0 )
  {
    computed_gsds_.insert( gsd );

    if( computed_gsds_.size() < computed_gsds_.capacity() )
    {
      gsd = invalid_gsd;
    }
    else
    {
      for( unsigned i = 1; i < computed_gsds_.size(); ++i )
      {
        double gsd1 = computed_gsds_.datum_at( i-1 );
        double gsd2 = computed_gsds_.datum_at( i );

        double diff = gsd2 - gsd1;

        if( gsd1 == 0.0 || gsd2 == 0.0 ||
            diff / gsd1 > consistency_measure_ ||
            diff / gsd2 > consistency_measure_ )
        {
          gsd = invalid_gsd;
          break;
        }
      }
    }
  }

  // Add GSD to metadata packet if it satisfies all criteria
  if( !meta_out_.empty() && gsd > 0.0 )
  {
    LOG_INFO( "Using Converted GSD: " << gsd );

    meta_out_[0].horizontal_gsd( gsd );
    meta_out_[0].vertical_gsd( gsd );

    frames_since_gsd_output_ = 0;
  }
  else
  {
    frames_since_gsd_output_++;
  }

  // Add backup GSD if there is one specified, depends on temporal constraint
  if( backup_gsd_enabled_ && gsd <= 0.0 )
  {
    double backup_gsd_to_use = invalid_gsd;

    if( raw_frame_gsds_.capacity() > 0 )
    {
      if( frames_since_gsd_output_ > raw_frame_gsds_.capacity() )
      {
        backup_gsd_to_use = backup_gsd_;

        for( unsigned i = 0; i < raw_frame_gsds_.size(); ++i )
        {
          if( raw_frame_gsds_.datum_at( i ) > 0.0 )
          {
            backup_gsd_to_use = raw_frame_gsds_.datum_at( i );
            break;
          }
        }
      }
    }
    else
    {
      backup_gsd_to_use = backup_gsd_;
    }

    if( !meta_out_.empty() && backup_gsd_to_use > 0.0 )
    {
      meta_out_[0].horizontal_gsd( backup_gsd_ );
      meta_out_[0].vertical_gsd( backup_gsd_ );
    }
  }

  // Optionally adjust FOV to parsed values
  if( adjust_fov_ )
  {
    if( gsd > 0.0 && is_metadata_valid )
    {
      double gsd_to_use = gsd * fov_multiplier_;
      double slant = meta_in_[0].slant_range();
      double hfov = 2.0 * std::atan2( ( gsd_to_use * image_.ni() / 2.0 ), slant ) * 180.0 / vnl_math::pi;

      for( unsigned i = 0; i < meta_out_.size(); ++i )
      {
        meta_out_[i].sensor_horiz_fov( hfov );
        meta_out_[i].sensor_vert_fov( 0.0 );
      }
    }
    else
    {
      for( unsigned i = 0; i < meta_out_.size(); ++i )
      {
        meta_out_[i].sensor_horiz_fov( 0.0 );
        meta_out_[i].sensor_vert_fov( 0.0 );
      }
    }
  }

  // Set is valid flag if we are over-riding external metadata
  if( remove_existing_metadata_ && ( gsd > 0.0 || backup_gsd_ > 0.0 ) )
  {
    meta_out_[0].is_valid( true );
  }

  // Set last FOV level
  last_fov_level_ = fov_level;

  return true;
}


template <typename PixType>
void
metadata_text_parser_process<PixType>
::set_image( image_t const& img )
{
  image_ = img;
}


template <typename PixType>
void
metadata_text_parser_process<PixType>
::set_metadata( video_metadata::vector_t const& meta )
{
  meta_in_ = meta;
}


template <typename PixType>
void
metadata_text_parser_process<PixType>
::set_border( image_border const& border )
{
  border_ = border;
}


template <typename PixType>
void
metadata_text_parser_process<PixType>
::set_instructions( instruction_set_t const& insts )
{
  instructions_ = insts;
}


template <typename PixType>
void
metadata_text_parser_process<PixType>
::set_target_widths( width_set_t const& widths )
{
  widths_ = widths;
}


template <typename PixType>
std::vector< std::string >
metadata_text_parser_process<PixType>
::parsed_strings() const
{
  return parsed_strings_;
}


template <typename PixType>
video_metadata::vector_t
metadata_text_parser_process<PixType>
::output_metadata() const
{
  return meta_out_;
}


} // end namespace vidtk
