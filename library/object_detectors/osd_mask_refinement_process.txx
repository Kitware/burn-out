/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "osd_mask_refinement_process.h"

#include <video_transforms/floating_point_image_hash_functions.h>
#include <video_transforms/automatic_white_balancing.h>

#include <utilities/config_block.h>

#include <vil/vil_plane.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER( "osd_mask_refinement_process" );


// Process Definition
template <typename PixType>
osd_mask_refinement_process<PixType>
::osd_mask_refinement_process( std::string const& _name )
  : process( _name, "osd_mask_refinement_process" )
{
  config_.add_parameter( "disabled",
    "false",
    "Should this process be disabled?" );

  config_.merge( osd_mask_refiner_settings().config() );
}


template <typename PixType>
osd_mask_refinement_process<PixType>
::~osd_mask_refinement_process()
{
}


template <typename PixType>
config_block
osd_mask_refinement_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
osd_mask_refinement_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );

    osd_mask_refiner_settings settings( blk );

    if( !refiner_.configure( settings ) )
    {
      throw config_block_parse_error( "Cannot configure osd mask refiner!" );
    }
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
osd_mask_refinement_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
process::step_status
osd_mask_refinement_process<PixType>
::step2()
{
  const bool is_last_frame = ( !input_mask_ || input_mask_.size() == 0 );
  process::step_status status = ( is_last_frame ? process::FAILURE : process::SUCCESS );
  output_mask_ = mask_image_type();

  if( disabled_ )
  {
    output_mask_ = input_mask_;
  }
  else
  {
    // Add new feature if set
    if( new_feature_.size() > 0 && input_osd_info_ )
    {
      for( unsigned p = 0; p < new_feature_.nplanes(); p++ )
      {
        input_osd_info_->features.push_back( vil_plane( new_feature_, p ) );
      }
    }

    // Run algorithm
    std::vector< mask_image_type > masks_to_output;
    std::vector< osd_info_sptr > osd_info_to_output;

    if( is_last_frame )
    {
      refiner_.flush_masks( masks_to_output,
                            osd_info_to_output );
    }
    else
    {
      refiner_.refine_mask( input_mask_,
                            input_osd_info_,
                            masks_to_output,
                            osd_info_to_output );
    }

    // Handle outputting past frames in case we have buffering opts turned on
    for( unsigned i = 0; i < masks_to_output.size(); ++i )
    {
      output_mask_ = masks_to_output[i];
      output_osd_info_ = osd_info_to_output[i];

      if( i != masks_to_output.size() - 1 )
      {
        push_output( process::SUCCESS );
      }
    }

    if( !is_last_frame && masks_to_output.empty() )
    {
      status = process::NO_OUTPUT;
    }
  }

  // Reset inputs
  input_mask_ = mask_image_type();
  new_feature_ = feature_image_type();

  return status;
}


template <typename PixType>
void
osd_mask_refinement_process<PixType>
::set_mask_image( mask_image_type const& mask )
{
  input_mask_ = mask;
}


template <typename PixType>
void
osd_mask_refinement_process<PixType>
::set_new_feature( feature_image_type const& feat )
{
  new_feature_ = feat;
}


template <typename PixType>
void
osd_mask_refinement_process<PixType>
::set_osd_info( osd_info_sptr input )
{
  input_osd_info_ = input;
}


template <typename PixType>
typename osd_mask_refinement_process<PixType>::mask_image_type
osd_mask_refinement_process<PixType>
::mask_image() const
{
  return output_mask_;
}


template <typename PixType>
typename osd_mask_refinement_process<PixType>::osd_info_sptr
osd_mask_refinement_process<PixType>
::osd_info() const
{
  return output_osd_info_;
}


} // end namespace vidtk
