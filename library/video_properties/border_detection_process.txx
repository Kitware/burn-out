/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "border_detection_process.h"

#include <utilities/config_block.h>

#include <video_transforms/automatic_white_balancing.h>

#include <vgl/vgl_intersection.h>

#include <vil/vil_fill.h>
#include <vil/vil_image_view.h>

#include <limits>

#include <logger/logger.h>


VIDTK_LOGGER("border_detection_process");


namespace vidtk
{

// Process Definition
template <typename PixType>
border_detection_process<PixType>
::border_detection_process( std::string const& _name )
  : process( _name, "border_detection_process" ),
    mode_( DETECT_SOLID_RECT ),
    reset_flag_( false ),
    disabled_( false ),
    use_history_( true ),
    history_length_( 5 ),
    frame_hold_( true ),
    hold_count_( 0 )
{
  config_.add_parameter( "disabled",
                         "false",
                         "Whether or not this process should be disabled." );
  config_.add_parameter( "type",
                         "black",
                         "Operating mode of this filter, possible values: black, auto,"
                         "white, bw, or fixed. If set to fixed, the output border will be "
                         "equivalent to the minumum border size for every iteration. Type bw "
                         "indicates we want to mark all pure black and white pixels touching "
                         "the border, as opposed to just detecting solid colored rectangular ones." );
  config_.add_parameter( "min_border_left",
                         "0",
                         "Minimum border for the left side of the image" );
  config_.add_parameter( "min_border_right",
                         "0",
                         "Minimum border for the right side of the image" );
  config_.add_parameter( "min_border_top",
                         "0",
                         "Minimum border for the top of the image" );
  config_.add_parameter( "min_border_bottom",
                         "0",
                         "Minimum border for the bottom of the image" );
  config_.add_parameter( "max_border_width",
                         "300",
                         "Maximum border for the left and right sides of the image" );
  config_.add_parameter( "max_border_height",
                         "200",
                         "Maximum border for the top and bottom of the image" );
  config_.add_parameter( "use_history",
                         "true",
                         "Should we average the border size over some history" );
  config_.add_parameter( "history_length",
                         "2",
                         "Length of history to take into consideration" );
  config_.add_parameter( "side_dilation",
                         "1",
                         "Number of extra pixels to dilate borders by" );
  config_.add_parameter( "default_variance",
                         "10",
                         "Allowable variance from pure white or black when "
                         "doing a pure-color based border detection." );
  config_.add_parameter( "high_res_variance",
                         "10",
                         "Allowable variance from pure white or black when "
                         "doing a pure-color based border detection." );
  config_.add_parameter( "required_percentage",
                         "0.90",
                         "Required percentage of border pixels required to satisfy "
                         "border constraints." );
  config_.add_parameter( "invalid_count",
                         "10",
                         "Number of invalid rows or columns required before we consider "
                         "the border terminated" );
  config_.add_parameter( "edge_refinement",
                         "false",
                         "Whether or not we should run edge detection to refine detected "
                         "borders" );
  config_.add_parameter( "edge_search_radius",
                         "6",
                         "Edge search radius in pixels" );
  config_.add_parameter( "edge_percentage_req",
                         "0.25",
                         "Percentage of edge pixels required for border qualifications" );
  config_.add_parameter( "edge_threshold",
                         "5",
                         "Edge threshold for border qualifications" );
  config_.add_parameter( "common_colors_only",
                         "false",
                         "Whether or not to only use detected borders of certain known colors "
                         "(certain shades of gray).");
  config_.add_parameter( "fix_border",
                         "false",
                         "If the border is fixed, after initial detection its size will "
                         "never be modified." );
  config_.add_parameter( "initial_hold_count",
                         "0",
                         "Number of frames to delay at the beginning of a video for using "
                         "the history to come up with a better border approximation." );
}


template <typename PixType>
border_detection_process<PixType>
::~border_detection_process()
{
}


template <typename PixType>
config_block
border_detection_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
border_detection_process<PixType>
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    disabled_ = blk.get< bool >( "disabled" );

    std::string type_string;
    type_string = blk.get< std::string >( "type" );

    // Possible functions for this node
    if( type_string == "fixed")
    {
      mode_ = FIXED_MODE;
    }
    else if( type_string == "black" )
    {
      mode_ = DETECT_SOLID_RECT;
      algorithm_settings_.detection_method_ = border_detection_settings< PixType>::BLACK;
    }
    else if( type_string == "white" )
    {
      mode_ = DETECT_SOLID_RECT;
      algorithm_settings_.detection_method_ = border_detection_settings< PixType>::WHITE;
    }
    else if( type_string == "auto" )
    {
      mode_ = DETECT_SOLID_RECT;
      algorithm_settings_.detection_method_ = border_detection_settings< PixType>::AUTO;
    }
    else if( type_string == "bw" )
    {
      mode_ = DETECT_BW;
    }
    else
    {
      throw config_block_parse_error( "Invalid border detection mode: " + type_string );
    }

    // Capture min / max border settings
    min_border_dim_[0] = blk.get<unsigned>( "min_border_left" );
    min_border_dim_[1] = blk.get<unsigned>( "min_border_top" );
    min_border_dim_[2] = blk.get<unsigned>( "min_border_right" );
    min_border_dim_[3] = blk.get<unsigned>( "min_border_bottom" );
    max_border_dim_[0] = blk.get<unsigned>( "max_border_width" );
    max_border_dim_[1] = blk.get<unsigned>( "max_border_height" );
    max_border_dim_[2] = blk.get<unsigned>( "max_border_width" );
    max_border_dim_[3] = blk.get<unsigned>( "max_border_height" );

    // Capture all other settings
    use_history_ = blk.get<bool>( "use_history" );
    history_length_ = blk.get<unsigned>( "history_length" );
    fix_border_ = blk.get<bool>( "fix_border" );
    initial_hold_count_ = blk.get<unsigned>( "initial_hold_count" );

    algorithm_settings_.side_dilation_ = blk.get<unsigned>( "side_dilation" );
    algorithm_settings_.default_variance_ = blk.get<PixType>( "default_variance" );
    algorithm_settings_.high_res_variance_ = blk.get<PixType>( "high_res_variance" );
    algorithm_settings_.required_percentage_ = blk.get<double>( "required_percentage" );
    algorithm_settings_.invalid_count_ = blk.get<unsigned>( "invalid_count" );
    algorithm_settings_.edge_refinement_ = blk.get<bool>( "edge_refinement" );
    algorithm_settings_.edge_search_radius_ = blk.get<unsigned>( "edge_search_radius" );
    algorithm_settings_.edge_percentage_req_ = blk.get<double>( "edge_percentage_req" );
    algorithm_settings_.edge_threshold_ = blk.get<PixType>( "edge_threshold" );
    algorithm_settings_.common_colors_only_ = blk.get<bool>( "common_colors_only" );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Validate input parameters
  if( use_history_ && history_length_ <= 1 )
  {
    LOG_ERROR( this->name() << ": set params failed: invalid history length" );
    return false;
  }

  // Configure historic buffer if required
  if( use_history_ )
  {
    history_.set_capacity( history_length_ );
    history_.clear();
  }

  // Set internal config block
  config_.update( blk );

  return true;
}


template <typename PixType>
bool
border_detection_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
process::step_status
border_detection_process<PixType>
::step2()
{
  // Check disabled flag
  if( disabled_ )
  {
    return process::SUCCESS;
  }

  // Handle grayscale port set, but not color
  if( gray_image_.size() > 0 && color_image_.size() == 0 )
  {
    color_image_ = gray_image_;
  }

  // Perform basic input validation
  if( !color_image_ )
  {
    LOG_ERROR( "No valid input image provided!" );
    return process::FAILURE;
  }
  if( color_image_.nplanes() != 1 && color_image_.nplanes() != 3 )
  {
    LOG_ERROR( "Input color image must contain 1 or 3 channels." );
    return process::FAILURE;
  }
  if( gray_image_.nplanes() > 1 )
  {
    LOG_ERROR( "Input grayscale image contains more than 1 channel." );
    return process::FAILURE;
  }

  // Check for reset
  if( reset_flag_ && use_history_ )
  {
    history_.clear();
  }

  // Check fixed flag
  if( fix_border_ &&
      detected_border_.area() > 0 &&
      ( !frame_hold_ || initial_hold_count_ == 0 ) )
  {
    color_image_ = NULL;
    reset_flag_ = false;

    return process::SUCCESS;
  }

  // Perform correct action
  if( mode_ == DETECT_SOLID_RECT )
  {
    // Detect border
    image_border detected_border;
    detect_solid_borders( color_image_, gray_image_, algorithm_settings_, detected_border );

    // Utilize history if specified
    if( use_history_ )
    {
      this->historic_smooth( detected_border );
    }

    // Set outputs
    this->set_square_mask( detected_border );
  }
  else if( mode_ == DETECT_BW )
  {
    // Detect non-linear bw borders directly into output mask
    output_mask_image_ = image_border_mask();
    detect_bw_nonrect_borders( color_image_, output_mask_image_ );

    // Set border rect to be in the entire image, because the detected
    // mask might not be rectilinear in this mode
    detected_border_ = image_border( 0, color_image_.ni(), 0, color_image_.nj() );
  }
  else if( mode_ == FIXED_MODE )
  {
    // Get image dimenions
    const unsigned ni = color_image_.ni();
    const unsigned nj = color_image_.nj();

    // Forumulate boundaries based on image size and minimum border size
    image_border img_border( min_border_dim_[0], ni-min_border_dim_[2],
                             min_border_dim_[1], nj-min_border_dim_[3] );

    // Create binary output mask
    this->set_square_mask( img_border );
  }
  else
  {
    LOG_ERROR( this->name() << ": Invalid mode supplied!" );
    return process::FAILURE;
  }

  // Handle frame buffering
  if( initial_hold_count_ > 0 && frame_hold_ )
  {
    hold_count_++;

    if( hold_count_ < initial_hold_count_ )
    {
      return process::NO_OUTPUT;
    }
    else
    {
      frame_hold_ = false;

      for( unsigned i = 0; i < initial_hold_count_ - 1; ++i )
      {
        this->push_output( process::SUCCESS );
      }
    }
  }

  // Reset input variables
  color_image_ = NULL;
  reset_flag_ = false;
  return process::SUCCESS;
}

// Set the mask output image to be around some perfect square
template <typename PixType>
void
border_detection_process<PixType>
::set_square_mask( const image_border& borders )
{
  // Get image dimenions
  const unsigned ni = color_image_.ni();
  const unsigned nj = color_image_.nj();

  // Confirm output mask size
  output_mask_image_.clear();
  output_mask_image_.set_size( ni, nj );

  // Determine start/end positions after all filters (max dim, min dim, dilation)
  image_border adj_border = borders;

  image_border min_border( min_border_dim_[0], ni-min_border_dim_[2],
                           min_border_dim_[1], nj-min_border_dim_[3] );
  image_border max_border( max_border_dim_[0], ni-max_border_dim_[2],
                           max_border_dim_[1], nj-max_border_dim_[3] );

  adj_border = vgl_intersection( adj_border, min_border );

  if( max_border.area() > 0 )
  {
    adj_border.add( max_border );
  }

  // Unlikely special case: The entire image is considered a border.
  if( adj_border.area() == 0 )
  {
    output_mask_image_.fill( true );
  }
  // Fill in adjusted borders
  else
  {
    output_mask_image_.fill( false );

    for( int i = 0; i < adj_border.min_x(); i++ )
    {
      vil_fill_col( output_mask_image_, i, true );
    }
    for( int i = adj_border.max_x(); i < static_cast<int>( color_image_.ni() ); i++ )
    {
      vil_fill_col( output_mask_image_, i, true );
    }
    for( int j = 0; j < adj_border.min_y(); j++ )
    {
      vil_fill_row( output_mask_image_, j, true );
    }
    for( int j = adj_border.max_y(); j < static_cast<int>( color_image_.nj() ); j++ )
    {
      vil_fill_row( output_mask_image_, j, true );
    }
  }

  // Set output border in case its used
  detected_border_ = adj_border;
}

// Using the stored settings, takes the maximum border in the last x frames
template <typename PixType>
void
border_detection_process<PixType>
::historic_smooth( image_border& img_border )
{
  // Insert copy of entry into history
  history_.insert( img_border );

  // Combine all past borders
  for( unsigned i = 1; i < history_.size(); i++ )
  {
    img_border = vgl_intersection( img_border, history_.datum_at( i ) );
  }
}

// Accessors
template <typename PixType>
void
border_detection_process<PixType>
::set_source_color_image( vil_image_view<PixType> const& src )
{
  color_image_ = src;
}

template <typename PixType>
void
border_detection_process<PixType>
::set_source_gray_image( vil_image_view<PixType> const& src )
{
  gray_image_ = src;
}

template <typename PixType>
void
border_detection_process<PixType>
::set_reset_flag( bool flag )
{
  reset_flag_ = flag;
}

template <typename PixType>
image_border_mask
border_detection_process<PixType>
::border_mask() const
{
  return output_mask_image_;
}

template <typename PixType>
image_border
border_detection_process<PixType>
::border() const
{
  return detected_border_;
}

} // end namespace vidtk
