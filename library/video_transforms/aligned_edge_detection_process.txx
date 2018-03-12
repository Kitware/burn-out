/*ckwg +5
 * Copyright 2012-2017 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "aligned_edge_detection_process.h"

#include <utilities/config_block.h>

#include <video_transforms/aligned_edge_detection.h>

#include <vil/vil_math.h>
#include <vil/vil_copy.h>

#include <vil/algo/vil_gauss_filter.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "aligned_edge_detection_process" );

// Process Definition
template <typename PixType>
aligned_edge_detection_process<PixType>
::aligned_edge_detection_process( std::string const& _name )
  : process( _name, "aligned_edge_detection_process" ),
    threshold_( 10 ),
    produce_joint_output_( true ),
    smoothing_sigma_( 1.3 ),
    smoothing_half_step_( 2 )
{
  config_.add_parameter( "threshold",
    "10",
    "Minimum edge magnitude required to report as an edge "
    "in any output image." );
  config_.add_parameter( "produce_joint_output",
    "true",
    "Set to false if we do not want to spend time computing "
    "joint edge images comprised of both horizontal and verticle "
    "information." );
  config_.add_parameter( "smoothing_sigma",
    "1.3",
    "Smoothing sigma for the output NMS edge density map." );
  config_.add_parameter( "smoothing_half_step",
    "2",
    "Smoothing half step for the output NMS edge density map." );
  config_.add_parameter( "max_thread_count",
    "1",
    "Whether or not to enable internal threading." );
  config_.add_parameter( "border_ignore_factor",
    "1",
    "Number of pixels to ignore near the border during operation." );
}


template <typename PixType>
aligned_edge_detection_process<PixType>
::~aligned_edge_detection_process()
{
}


template <typename PixType>
config_block
aligned_edge_detection_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
aligned_edge_detection_process<PixType>
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    threshold_ = blk.get<float>( "threshold" );
    produce_joint_output_ = blk.get<bool>( "produce_joint_output" );

    if( produce_joint_output_ )
    {
      smoothing_sigma_ = blk.get<double>( "smoothing_sigma" );
      smoothing_half_step_ = blk.get<unsigned>( "smoothing_half_step" );
    }

    unsigned max_thread_count = blk.get<unsigned>( "max_thread_count" );

    if( max_thread_count < 3 )
    {
      threads_.reset( new thread_sys_t( 1, 1 ) );
    }
    else
    {
      threads_.reset( new thread_sys_t( 1, 3 ) );
    }

    threads_->set_function(
      boost::bind(
        &self_type::process_region,
        this, _1, _2, _3, _4, _5 ) );

    threads_->set_border_mode( true, blk.get<int>( "border_ignore_factor" ) );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Set internal config block
  config_.update( blk );

  return true;
}


template <typename PixType>
bool
aligned_edge_detection_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
bool
aligned_edge_detection_process<PixType>
::step()
{
  // Perform Basic Validation
  if( !source_image_ || source_image_.nplanes() != 1 )
  {
    LOG_ERROR( this->name() << ": Input must be a grayscale image!" );
    return false;
  }

  // Perform algorithm with optional threading
  aligned_edge_output_ = input_image_t( source_image_.ni(),
                                        source_image_.nj(),
                                        2 );

  aligned_edge_output_.fill( 0 );

  if( produce_joint_output_ )
  {
    combined_edge_output_ = input_image_t( source_image_.ni(),
                                           source_image_.nj() );

    combined_edge_output_.fill( 0 );
  }

  if( grad_i_.ni() != source_image_.ni() || grad_i_.nj() != source_image_.nj() )
  {
    grad_i_.set_size( source_image_.ni(), source_image_.nj() );
  }
  if( grad_j_.ni() != source_image_.ni() || grad_j_.nj() != source_image_.nj() )
  {
    grad_j_.set_size( source_image_.ni(), source_image_.nj() );
  }

  threads_->apply_function( border_, source_image_, aligned_edge_output_,
                            combined_edge_output_, grad_i_, grad_j_ );

  // Reset input variables
  source_image_ = input_image_t();

  return true;
}

template <typename PixType>
void
aligned_edge_detection_process<PixType>
::process_region( const input_image_t input_image,
                  input_image_t aligned_edges,
                  input_image_t combined_edges,
                  float_image_t grad_i_buffer,
                  float_image_t grad_j_buffer )
{
  calculate_aligned_edges( input_image,
                           threshold_,
                           aligned_edges,
                           grad_i_buffer,
                           grad_j_buffer );

  // Perform extra op if enabled
  if( produce_joint_output_ )
  {
    // Add verticle and horizontal edge planes together and smooth
    input_image_t joint_nms_edges;

    vil_math_image_sum( vil_plane( aligned_edges, 0 ),
                        vil_plane( aligned_edges, 1 ),
                        joint_nms_edges );

    unsigned half_step = smoothing_half_step_;
    unsigned min_dim = std::min( joint_nms_edges.ni(), joint_nms_edges.nj() );

    if( 2 * half_step + 1 >= min_dim )
    {
      half_step = ( min_dim - 1 ) / 2;
    }

    if( half_step != 0 )
    {
      vil_gauss_filter_2d( joint_nms_edges,
                           combined_edges,
                           smoothing_sigma_,
                           half_step );
    }
    else
    {
      vil_copy_reformat( joint_nms_edges, combined_edges );
    }
  }
}

// Accessors
template <typename PixType>
void
aligned_edge_detection_process<PixType>
::set_source_image( input_image_t const& src )
{
  source_image_ = src;
}

template <typename PixType>
void
aligned_edge_detection_process<PixType>
::set_source_border( image_border const& border )
{
  border_ = border;
}

template <typename PixType>
vil_image_view<PixType>
aligned_edge_detection_process<PixType>
::aligned_edge_image() const
{
  return aligned_edge_output_;
}

template <typename PixType>
vil_image_view<PixType>
aligned_edge_detection_process<PixType>
::combined_edge_image() const
{
  return combined_edge_output_;
}

} // end namespace vidtk
