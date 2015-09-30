/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "identify_metadata_burnin_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vul/vul_file.h>

#include <vcl_fstream.h>

#ifdef USE_OPENCV
#include "identify_metadata_burnin_process_impl_opencv.h"
#endif

namespace vidtk
{

config_block
identify_metadata_burnin_process_impl
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
    "Minimum oroportion of the width of the frame size (centered) to look for burnin" );

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
    "" );

  config_.add_parameter(
    "text_templates",
    "",
    "File containing the list of images to use for text detection (paths are relative to the file's location)" );

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
identify_metadata_burnin_process_impl
::set_params( config_block const& blk )
{
  vcl_string text_templates;

  try
  {
    blk.get( "disabled", disabled_ );
    blk.get( "cross_threshold", cross_threshold_ );
    blk.get( "bracket_threshold", bracket_threshold_ );
    blk.get( "rectangle_threshold", rectangle_threshold_ );
    blk.get( "text_threshold", text_threshold_ );
    blk.get( "line_width", line_width_ );
    blk.get( "draw_line_width", draw_line_width_ );
    blk.get( "min_roi_ratio", min_roi_ratio_ );
    blk.get( "roi_ratio", roi_ratio_ );
    blk.get( "roi_aspect", roi_aspect_ );
    blk.get( "off_center:x", off_center_x_ );
    blk.get( "off_center:y", off_center_y_ );
    blk.get( "cross_gap:x", cross_gap_x_ );
    blk.get( "cross_gap:y", cross_gap_y_ );
    blk.get( "cross_length:x", cross_length_x_ );
    blk.get( "cross_length:y", cross_length_y_ );
    blk.get( "bracket_length:x", bracket_length_x_ );
    blk.get( "bracket_length:y", bracket_length_y_ );
    blk.get( "dark_pixel_threshold", dark_pixel_threshold_ );
    blk.get( "off_center_jitter", off_center_jitter_ );
    blk.get( "bracket_aspect_jitter", bracket_aspect_jitter_ );
    blk.get( "text_repeat_threshold", text_repeat_threshold_ );
    blk.get( "text_memory", text_memory_ );
    blk.get( "text_consistency_threshold", text_consistency_threshold_ );
    blk.get( "text_range", text_range_ );
    blk.get( "max_text_dist", max_text_dist_ );
    blk.get( "edge_template", edge_template_ );
    blk.get( "text_templates", text_templates );
    blk.get( "verbose", verbose_ );
    blk.get( "write_intermediates", write_intermediates_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( name_ << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  vcl_ifstream fin;
  fin.open( text_templates.c_str() );

  while ( fin.good() )
  {
    vcl_string fname1;
    vcl_string fname2;

    vcl_getline( fin, fname1 );
    vcl_getline( fin, fname2 );

    if ( fname1.empty() || fname2.empty() )
    {
      continue;
    }

    templates_.push_back( vcl_make_pair( fname1, fname2 ) );
  }

  fin.close();

  return true;
}


bool
identify_metadata_burnin_process_impl
::initialize()
{
  return true;
}


bool
identify_metadata_burnin_process_impl
::step()
{
  mask_ = vil_image_view<bool>( input_image_.ni(), input_image_.nj(), 1, 1 );

  return true;
}

void
identify_metadata_burnin_process_impl
::set_input_image( vil_image_view< vxl_byte > const& img )
{
  input_image_ = vil_image_view<vxl_byte>( img.ni(), img.nj(), 1, img.nplanes() );
}


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
identify_metadata_burnin_process
::identify_metadata_burnin_process( vcl_string const& name )
  : process( name, "identify_metadata_burnin_process" ),
    impl_( NULL )
{
}


identify_metadata_burnin_process
::~identify_metadata_burnin_process()
{
  delete impl_;
}


config_block
identify_metadata_burnin_process
::params() const
{
  config_block config_ = identify_metadata_burnin_process_impl::params();

  config_.add( "impl", "null" );

  return config_;
}


bool
identify_metadata_burnin_process
::set_params( config_block const& blk )
{
  try
  {
    vcl_string impl_type;

    blk.get( "impl", impl_type );

    if ( impl_type == "null" )
    {
      impl_ = new identify_metadata_burnin_process_impl;
    }
#ifdef USE_OPENCV
    else if ( impl_type == "opencv" )
    {
      impl_ = new identify_metadata_burnin_process_impl_opencv;
    }
#endif
    else
    {
      log_error( this->name() << ": Unknown burnin detection implementation: "<< impl_type <<"\n" );

      return false;
    }

    if ( impl_ )
    {
      impl_->name_ = name();
      impl_->set_params( blk );
    }
  }
  catch( unchecked_return_value& e )
  {
    log_error( name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  return true;
}


bool
identify_metadata_burnin_process
::initialize()
{
  return true;
}


bool
identify_metadata_burnin_process
::step()
{
  if( impl_->disabled_ )
  {
    impl_->mask_ = vil_image_view<bool>(NULL);
    return true;
  }

  return impl_->step();
}


void
identify_metadata_burnin_process
::set_source_image( vil_image_view<vxl_byte> const& img )
{
  if( !impl_->disabled_ )
  {
    impl_->set_input_image( img );
  }
}


vil_image_view<bool> const&
identify_metadata_burnin_process
::metadata_mask() const
{
  return impl_->mask_;
}

} // end namespace vidtk
