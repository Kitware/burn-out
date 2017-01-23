/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tot_descriptor_generator.h"

#include <string>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_tot_descriptor_generator_cxx__
VIDTK_LOGGER( "tot_descriptor_generator_cxx" );

namespace vidtk
{

const std::string tot_sp_identifier = "tot_sp";


tot_descriptor_settings
::tot_descriptor_settings()
 : output_intermediates( false )
{
  tot_super_process< vxl_byte > tmp( tot_sp_identifier );
  stored_config = tmp.params();

  stored_config.add_parameter( "output_intermediates",
    "false",
    "Should raw descriptors be output in addition to tot "
    "outputs in descriptor form?" );
}


config_block
tot_descriptor_settings
::config() const
{
  return stored_config;
}


void
tot_descriptor_settings
::read_config( const config_block& blk )
{
  stored_config.update( blk );

  output_intermediates = blk.get< bool >( "output_intermediates" );
}


tot_descriptor_generator
::tot_descriptor_generator()
 : computer_( tot_sp_identifier )
{
}


tot_descriptor_generator
::~tot_descriptor_generator()
{
}


bool
tot_descriptor_generator
::configure( const external_settings& settings )
{
  tot_descriptor_settings const* desc_settings =
    dynamic_cast< tot_descriptor_settings const* >( &settings );

  if( !desc_settings )
  {
    return false;
  }

  settings_ = *desc_settings;

  config_block forced_parameters;
  forced_parameters.add_parameter( "run_async", "false", "Forced parameter" );
  settings_.stored_config.update( forced_parameters );

  config_block dependency_parameters;
  dependency_parameters.add_parameter( "output_intermediates", "false", "Dependency parameter" );
  settings_.stored_config.remove( dependency_parameters );

  return this->reset_generator();
}


external_settings* tot_descriptor_generator
::create_settings()
{
  return new tot_descriptor_settings;
}


bool
tot_descriptor_generator
::frame_update_routine()
{
  frame_data_sptr frame = this->current_frame();

  if( !frame )
  {
    LOG_ERROR( "Invalid input frame" );
    return false;
  }

  // Process the current frame
  if( frame->was_image_set() )
  {
    computer_.set_source_image( frame->image_8u() );
  }
  if( frame->was_tracks_set() )
  {
    computer_.set_source_tracks( frame->active_tracks() );
  }
  if( frame->was_metadata_set() )
  {
    computer_.set_source_metadata( frame->metadata() );
  }
  if( frame->was_timestamp_set() )
  {
    computer_.set_source_timestamp( frame->frame_timestamp() );
  }
  if( frame->was_world_homography_set() )
  {
    image_to_utm_homography world_homography;
    world_homography.set_transform( frame->world_homography() );

    computer_.set_source_image_to_world_homography( world_homography );
  }
  if( frame->was_modality_set() )
  {
    computer_.set_source_modality( frame->modality() );
  }
  if( frame->was_break_flags_set() )
  {
    computer_.set_source_shot_break_flags( frame->shot_breaks() );
  }
  if( frame->was_gsd_set() )
  {
    computer_.set_source_gsd( frame->average_gsd() );
  }

  // Step computer
  if( computer_.step2() != process::SUCCESS )
  {
    LOG_ERROR( "Failed to step tot super process" );
    return false;
  }

  // Generate descriptors from the PVO values stored in tracks
  std::vector< track_sptr > tracks_w_pvo = computer_.output_tracks();

  for( unsigned i = 0; i < tracks_w_pvo.size(); ++i )
  {
    const track_sptr& trk = tracks_w_pvo[i];
    pvo_probability pvo;

    if( trk && trk->get_pvo( pvo ) )
    {
      descriptor_sptr output = create_descriptor( "tot_descriptor", trk->id() );
      descriptor_history_entry region = generate_history_entry( this->current_frame(), trk );

      output->add_history_entry( region );
      output->resize_features( 3.0 );
      output->at( 0 ) = pvo.get_probability_person();
      output->at( 1 ) = pvo.get_probability_vehicle();
      output->at( 2 ) = pvo.get_probability_other();
      output->set_pvo_flag( true );

      this->add_outgoing_descriptor( output );
    }
  }

  if( settings_.output_intermediates )
  {
    raw_descriptor::vector_t descs = computer_.descriptors();

    for( unsigned i = 0; i < descs.size(); ++i )
    {
      this->add_outgoing_descriptor( descs[i] );
    }
  }

  return true;
}


bool
tot_descriptor_generator
::reset_generator()
{
  if( !computer_.set_params( settings_.stored_config ) )
  {
    return false;
  }

  return computer_.initialize();
}


} //namespace vidtk
