/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "moving_burnin_detector_process.h"

#include <vul/vul_file.h>

#include <fstream>
#include <set>

#ifdef USE_OPENCV
#include "moving_burnin_detector_opencv.h"
#endif

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_moving_burnin_detector_process_cxx__
VIDTK_LOGGER("moving_burnin_detector_process_cxx");


namespace vidtk
{

config_block
moving_burnin_detector_impl
::params()
{
  config_block config_;

  config_.add_parameter(
    "disabled",
    "true",
    "Disable this process, causing the image to pass through unmodified." );

  config_.add_parameter(
    "cross_threshold",
    "-1",
    "Minimum coverage of the cross with edge detection (negative to disable)" );

  config_.add_parameter(
    "bracket_threshold",
    "-1",
    "Minimum coverage of the bracket with edge detection (negative to disable)" );

  config_.add_parameter(
    "rectangle_threshold",
    "-1",
    "Minimum coverage of the rectangle with edge detection (negative to disable)" );

  config_.add_parameter(
    "text_threshold",
    "-1",
    "Minimum coverage of the text with edge detection (negative to disable)" );

  config_.add_parameter(
    "highest_score_only",
    "false",
    "If there are multiple detections of a given category, should only the one "
    "with the highest score be reported as a detection?" );

  config_.add_parameter(
    "line_width",
    "3",
    "Width of lines in the metadata" );

  config_.add_parameter(
    "draw_line_width",
    "3",
    "Width of lines in the mask output" );

  config_.add_parameter(
    "min_roi_ratio",
    "0.1",
    "Minimum proportion of the width of the frame size (centered) to look for burnin" );

  config_.add_parameter(
    "roi_ratio",
    "0.5",
    "Proportion of the width of the frame size (centered) to look for burnin" );

  config_.add_parameter(
    "roi_aspect",
    "0",
    "Aspect ratio of the brackets. (height/width; 0 for same as frame)" );

  config_.add_parameter(
    "off_center:x",
    "0",
    "Horizontal offset of the center of the brackets from the center of the frame" );

  config_.add_parameter(
    "off_center:y",
    "0",
    "Vertical offset of the center of the brackets from the center of the frame" );

  config_.add_parameter(
    "cross_gap:x",
    "6",
    "Horizontal gap between cross segments" );

  config_.add_parameter(
    "cross_gap:y",
    "6",
    "Vertical gap between cross segments" );

  config_.add_parameter(
    "cross_length:x",
    "14",
    "Length of horizontal cross segments" );

  config_.add_parameter(
    "cross_length:y",
    "14",
    "Length of vertical cross segments" );

  config_.add_parameter(
    "cross_ends_ratio",
    "-1.0",
    "The 'ends' of cross are the perpendicular lines at the outer ends of the cross,"
    " e.g. if this is ratio of length of '|' to '---' in one the left leg '|---' "
    " of the cross-hair. This is a ratio between the length of this end to the inner"
    " segment" );

  config_.add_parameter(
    "bracket_length:x",
    "18",
    "Horizontal length of bracket corners" );

  config_.add_parameter(
    "bracket_length:y",
    "18",
    "Vertical length of bracket corners" );

  config_.add_parameter(
    "dark_pixel_threshold",
    "25",
    "Threshold to control edge map generation (smaller == more sensitive)" );

  config_.add_parameter(
    "off_center_jitter",
    "1",
    "Offset from center to search for brackets and the cross" );

  config_.add_parameter(
    "bracket_aspect_jitter",
    "5",
    "Offset from 1:1 aspect with the frame to search for bracket corners (in pixels)" );

  config_.add_parameter(
    "text_detect_threshold",
    ".2",
    "The minimum value to be considered a match" );

  config_.add_parameter(
    "text_repeat_threshold",
    ".15",
    "The minimum value to be considered a match if a match has been found in the vicinity before" );

  config_.add_parameter(
    "text_memory",
    "6",
    "The size of the temporal memory when searching for text" );

  config_.add_parameter(
    "text_consistency_threshold",
    "0.5",
    "The minumum ratio of found to not found frames in the smaller ROI to trigger resetting the search for the template" );

  config_.add_parameter(
    "text_range",
    "10",
    "The distance to check around the previous match for text in successive frames" );

  config_.add_parameter(
    "max_text_dist",
    "20",
    "Maximum distance between color and edge detection locations" );

  config_.add_parameter(
    "edge_template",
    "",
    "Image template for moving metadata components in edge space" );

  config_.add_parameter(
    "text_templates",
    "",
    "File containing the list of images to use for text detection (paths are relative to the file's location)" );

  config_.add_parameter(
    "target_resolution_x",
    "0",
    "Image column resolution that these settings were designed for, if known." );

  config_.add_parameter(
    "target_resolution_y",
    "0",
    "Image row resolution that these settings were designed for, if known." );

  config_.add_parameter(
    "verbose",
    "false",
    "Enable additional log messages about detection that are useful in parameter tuning." );

  config_.add_parameter(
    "write_intermediates",
    "false",
    "Write intermediate image detections." );

  return config_;
}


bool
moving_burnin_detector_impl
::set_params( config_block const& blk )
{
  std::string text_templates;

  try
  {
    disabled_ = blk.get<bool>( "disabled" );
    cross_threshold_ = blk.get<double>( "cross_threshold" );
    bracket_threshold_ = blk.get<double>( "bracket_threshold" );
    rectangle_threshold_ = blk.get<double>( "rectangle_threshold" );
    text_threshold_ = blk.get<double>( "text_threshold" );
    highest_score_only_ = blk.get<bool>( "highest_score_only" );
    line_width_ = blk.get<double>( "line_width" );
    draw_line_width_ = blk.get<double>( "draw_line_width" );
    min_roi_ratio_ = blk.get<double>( "min_roi_ratio" );
    roi_ratio_ = blk.get<double>( "roi_ratio" );
    roi_aspect_ = blk.get<double>( "roi_aspect" );
    off_center_x_ = blk.get<int>( "off_center:x" );
    off_center_y_ = blk.get<int>( "off_center:y" );
    cross_gap_x_ = blk.get<int>( "cross_gap:x" );
    cross_gap_y_ = blk.get<int>( "cross_gap:y" );
    cross_length_x_ = blk.get<int>( "cross_length:x" );
    cross_length_y_ = blk.get<int>( "cross_length:y" );
    bracket_length_x_ = blk.get<int>( "bracket_length:x" );
    bracket_length_y_ = blk.get<int>( "bracket_length:y" );
    dark_pixel_threshold_ = blk.get<int>( "dark_pixel_threshold" );
    off_center_jitter_ = blk.get<int>( "off_center_jitter" );
    bracket_aspect_jitter_ = blk.get<int>( "bracket_aspect_jitter" );
    text_repeat_threshold_ = blk.get<double>( "text_repeat_threshold" );
    text_memory_ = blk.get<unsigned long>( "text_memory" );
    text_consistency_threshold_ = blk.get<double>( "text_consistency_threshold" );
    text_range_ = blk.get<unsigned long>( "text_range" );
    target_resolution_x_ = blk.get<unsigned>( "target_resolution_x" );
    target_resolution_y_ = blk.get<unsigned>( "target_resolution_y" );
    max_text_dist_ = blk.get<double>( "max_text_dist" );
    edge_template_ = blk.get<std::string>( "edge_template" );
    text_templates = blk.get<std::string>( "text_templates" );
    verbose_ = blk.get<bool>( "verbose" );
    write_intermediates_ = blk.get<bool>( "write_intermediates" );

    cross_ends_ratio_ = blk.get<float>( "cross_ends_ratio" );
    // The only use of it needs to have half of the len
    cross_ends_ratio_ *= 0.5;
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( name_ << ": set_params failed: " << e.what() );
    return false;
  }

  templates_.clear();

  std::ifstream fin;
  fin.open( text_templates.c_str() );

  std::string fname1;
  std::string fname2;

  while ( std::getline(fin, fname1) && std::getline(fin, fname2) )
  {
    if ( fname1.empty() || fname2.empty() )
    {
      continue;
    }

    templates_.push_back( std::make_pair( fname1, fname2 ) );
  }

  fin.close();

  return true;
}


bool
moving_burnin_detector_impl
::initialize()
{
  return true;
}


bool
moving_burnin_detector_impl
::step()
{
  mask_ = vil_image_view<bool>( input_image_.ni(), input_image_.nj(), 1, 1 );

  return true;
}

void
moving_burnin_detector_impl
::set_input_image( vil_image_view< vxl_byte > const& img )
{
  input_image_ = vil_image_view<vxl_byte>( img.ni(), img.nj(), 1, img.nplanes() );
}

void
moving_burnin_detector_impl
::scale_params_for_image( vil_image_view< vxl_byte > const& img )
{
  if( target_resolution_x_ == 0 ||
      target_resolution_y_ == 0 ||
      ( target_resolution_x_ == img.ni() &&
        target_resolution_y_ == img.nj() ) )
  {
    return;
  }

  double scale_factor_x = static_cast<double>( img.ni() ) / target_resolution_x_;
  double scale_factor_y = static_cast<double>( img.nj() ) / target_resolution_y_;
  double avg_scale_factor = ( scale_factor_x + scale_factor_y ) / 2.0;

  double min_draw_line_width_ = ( draw_line_width_ >= 3.0 ? 3.0 : draw_line_width_ );
  double min_line_width_ = ( line_width_ >= 1.0 ? 1.0 : line_width_ );

  line_width_ = std::max( line_width_ * avg_scale_factor, min_line_width_ );
  draw_line_width_ = std::max( draw_line_width_ * avg_scale_factor, min_draw_line_width_ );
  off_center_x_ = static_cast<int>( off_center_x_ * scale_factor_x + 0.5 );
  off_center_y_ = static_cast<int>( off_center_y_ * scale_factor_y + 0.5 );
  cross_gap_x_ = static_cast<int>( cross_gap_x_ * scale_factor_x + 0.5 );
  cross_gap_y_ = static_cast<int>( cross_gap_y_ * scale_factor_y + 0.5 );
  cross_length_x_ = static_cast<int>( cross_length_x_ * scale_factor_x + 0.5 );
  cross_length_y_ = static_cast<int>( cross_length_y_ * scale_factor_y + 0.5 );
  bracket_length_x_ = static_cast<int>( bracket_length_x_ * scale_factor_x + 0.5 );
  bracket_length_y_ = static_cast<int>( bracket_length_y_ * scale_factor_y + 0.5 );

  target_resolution_x_ = img.ni();
  target_resolution_y_ = img.nj();
}


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
moving_burnin_detector_process
::moving_burnin_detector_process( std::string const& _name )
  : process( _name, "moving_burnin_detector_process" ),
    force_disable_( false )
{
}


moving_burnin_detector_process
::~moving_burnin_detector_process()
{
}


config_block
moving_burnin_detector_process
::params() const
{
  config_block config_ = moving_burnin_detector_impl::params();

  config_.add_parameter( "impl",
    "null",
    "Implementation type, can either be null or opencv." );

  return config_;
}


bool
moving_burnin_detector_process
::set_params( config_block const& blk )
{
  try
  {
    if( blk.get<std::string>( "disabled" ) == "forced" )
    {
      force_disable_ = true;
      impl_.reset( new moving_burnin_detector_impl );
      return true;
    }

    std::string impl_type = blk.get<std::string>( "impl" );

    if( impl_type == "null" )
    {
      impl_.reset( new moving_burnin_detector_impl );
    }
#ifdef USE_OPENCV
    else if( impl_type == "opencv" )
    {
      impl_.reset( new moving_burnin_detector_opencv );
    }
#endif
    else
    {
      throw config_block_parse_error( "Unknown burnin detection implementation: " + impl_type );
    }

    if( impl_ )
    {
      impl_->name_ = name();
      impl_->set_params( blk );
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( name() << ": set_params failed: " << e.what() );
    return false;
  }

  return true;
}


bool
moving_burnin_detector_process
::initialize()
{
  return true;
}


bool
moving_burnin_detector_process
::step()
{
  if( force_disable_ )
  {
    return true;
  }

  if( !parameter_file_.empty() && parameter_file_ != last_parameter_file_ )
  {
    LOG_DEBUG( this->name() << " switching to use " << parameter_file_ );

    try
    {
      config_block new_params = this->params();

      if( parameter_file_ == "DISABLE" )
      {
        new_params.set( "disabled", "true" );
      }
      else if( !new_params.parse( parameter_file_.c_str() ) )
      {
        throw config_block_parse_error( "Unable to load: " + parameter_file_ );
      }

      if( !set_params( new_params ) )
      {
        throw config_block_parse_error( "Unable switch parameter set" );
      }

      last_parameter_file_ = parameter_file_;
    }
    catch( config_block_parse_error const& e )
    {
      LOG_ERROR( e.what() );
      return false;
    }
  }

  if( !impl_->disabled_ )
  {
    impl_->set_input_image( input_image_ );
  }

  if( impl_->disabled_ || !impl_->input_image_ )
  {
    impl_->mask_ = vil_image_view<bool>(NULL);
    return true;
  }

  return impl_->step();
}


void
moving_burnin_detector_process
::set_source_image( vil_image_view<vxl_byte> const& img )
{
  input_image_ = img;
}


void
moving_burnin_detector_process
::set_parameter_file( std::string const& file )
{
  parameter_file_ = file;
}


vil_image_view<bool>
moving_burnin_detector_process
::metadata_mask() const
{
  return impl_->mask_;
}


std::vector<unsigned>
moving_burnin_detector_process
::target_pixel_widths() const
{
  return impl_->target_widths_;
}


} // end namespace vidtk
