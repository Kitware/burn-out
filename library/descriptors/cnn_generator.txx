/*ckwg +5
 * Copyright 2015-2017 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/cnn_generator.h>

#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <vil/vil_resample_bilin.h>

#include <boost/lexical_cast.hpp>

#include <algorithm>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "cnn_generator_txx" );

template <typename PixType>
cnn_generator<PixType>
::cnn_generator()
{
}

template <typename PixType>
bool
cnn_generator<PixType>
::configure( const external_settings& settings )
{
  cnn_settings const* desc_settings =
    dynamic_cast<cnn_settings const*>(&settings);

  if( !desc_settings )
  {
    return false;
  }

  // Create internal copy of inputted external settings
  this->settings_ = *desc_settings;

  // Check if a chip image path is set
  is_training_mode_ = !settings_.chip_extraction_prefix.empty();

  // Configure internal descriptor module
  cnn_descriptor_settings computer_settings;

  computer_settings.model_definition = settings_.model_definition;
  computer_settings.model_weights = settings_.model_weights;
  computer_settings.use_gpu = settings_.use_gpu;
  computer_settings.device_id = settings_.device_id;
  computer_settings.layers_to_use = settings_.layers_to_use;
  computer_settings.final_layer_indices = settings_.final_layer_indices;

  if( !descriptor_.configure( computer_settings ) )
  {
    LOG_ERROR( "Unable to configure descriptor" );
    return false;
  }

  // Pre-compute descriptor IDs
  for( unsigned i = 0; i < settings_.layers_to_use.size(); ++i )
  {
    descriptor_ids_.push_back( settings_.descriptor_prefix + settings_.layers_to_use[i] );
  }

  return true;
}

template <typename PixType>
external_settings*
cnn_generator<PixType>
::create_settings()
{
  return new cnn_settings;
}

template <typename PixType>
bool
cnn_generator<PixType>
::frame_update_routine()
{
  // Validate input frame format
  const frame_sptr& frame_data = this->current_frame();
  current_frame_ = frame_data->image< PixType >();

  if( !current_frame_ || ( current_frame_.nplanes() != 3 && current_frame_.nplanes() != 1 ) )
  {
    LOG_ERROR( "Input image must contain 1 or 3 planes!" );
    return false;
  }

  return true;
}

template <typename PixType>
track_data_sptr
cnn_generator<PixType>
::new_track_routine( track_sptr const& new_track )
{
  // Create new data for this track, and initialize counters
  cnn_track_data* track_data = new cnn_track_data;

  track_data->frames_observed = 0;
  track_data->compute_mode = true;

  for( unsigned i = 0; i < descriptor_ids_.size(); ++i )
  {
    track_data->descs.push_back(
      create_descriptor(
        descriptor_ids_[i],
        new_track->id() ) );
  }

  return track_data_sptr( track_data );
}


template <typename PixType>
bool
cnn_generator<PixType>
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  cnn_track_data* data = dynamic_cast< cnn_track_data* >( track_data.get() );

  // Decide if we only want to process frames with new bboxes
  if( settings_.new_states_w_bbox_only )
  {
    const timestamp& current_ts = active_track->last_state()->time_;
    vgl_box_2d<unsigned> tmp_bbox;

    if( !active_track->last_state()->bbox( tmp_bbox ) ||
        ( data->last_state_ts.has_time() &&
          data->last_state_ts.time() == current_ts.time() ) ||
        ( data->last_state_ts.has_frame_number() &&
          data->last_state_ts.frame_number() == current_ts.frame_number() ) )
    {
      return true;
    }

    data->last_state_ts = current_ts;
  }

  // Increase observed frames count
  data->frames_observed++;

  // Add history reference to descriptor
  descriptor_history_entry region = generate_history_entry( current_frame(), active_track );

  for( unsigned i = 0; i < descriptor_ids_.size(); ++i )
  {
    data->descs[i]->add_history_entry( region );
  }

  // If in compute mode, add this region to the descriptor
  if( data->compute_mode )
  {
    // Expand BB to desired size and format
    vil_image_view< vxl_byte > track_region;

    if( !compute_track_snippet( current_frame_,
                                region.get_image_location(),
                                track_region ) )
    {
      return true;
    }

    // Compute descriptor for region, and add to moving average
    if( is_training_mode_ )
    {
      static unsigned chip_id = 1;
      std::string chip_fn = settings_.chip_extraction_prefix +
        boost::lexical_cast< std::string >( chip_id++ ) + ".png";

      vil_save( track_region, chip_fn.c_str() );
    }
    else
    {
      cnn_descriptor::output_t features;

      {
        boost::unique_lock< boost::mutex > lock( mutex_ );
        descriptor_.compute( track_region, features );
      }

      for( unsigned i = 0; i < features.size(); ++i )
      {
        if( settings_.temporal_pooling_method == cnn_settings::SUM )
        {
          add_features_into( features[i], data->descs[i] );
        }
        else
        {
          take_max_features( features[i], data->descs[i] );
        }

        if( settings_.output_size_info )
        {
          LOG_INFO( "CNN Descriptor: "
                    << settings_.layers_to_use[i]
                    << " Size: "
                    << features[i].size() );
        }
      }
    }
  }

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.descriptor_length )
  {
    for( unsigned i = 0; i < descriptor_ids_.size(); ++i )
    {
      // Dump and normalize output
      if( settings_.normalize_descriptors )
      {
        normalize_descriptor( data->descs[i] );
      }

      // Set optional flag
      data->descs[i]->set_pvo_flag( settings_.set_pvo_flag );

      // Add outgoing descriptor
      this->add_outgoing_descriptor( data->descs[i] );

      // Make new wip descriptor
      data->descs[i] = create_descriptor( descriptor_ids_[i], active_track->id() );
    }

    // Reset counters
    data->frames_observed = 0;
    data->compute_mode = false;
  }

  // Check if we should switch to compute mode again
  if( !data->compute_mode && data->frames_observed >= settings_.descriptor_spacing )
  {
    data->frames_observed = 0;
    data->compute_mode = true;
  }

  return true;
}


template <typename PixType>
bool
cnn_generator<PixType>
::terminated_track_routine( track_sptr const& /*finished_track*/,
                            track_data_sptr track_data )
{
  cnn_track_data* data = dynamic_cast< cnn_track_data* >( track_data.get() );

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.min_frames_for_desc )
  {
    for( unsigned i = 0; i < descriptor_ids_.size(); ++i )
    {
      // Dump and normalize output
      if( settings_.normalize_descriptors )
      {
        normalize_descriptor( data->descs[i] );
      }

      // Set optional flag
      data->descs[i]->set_pvo_flag( settings_.set_pvo_flag );

      // Add outgoing descriptor
      this->add_outgoing_descriptor( data->descs[i] );
    }
  }

  return true;
}

template <typename PixType>
bool
cnn_generator<PixType>
::compute_track_snippet( vil_image_view< PixType > const& image,
                         vgl_box_2d< unsigned > const& region,
                         vil_image_view< vxl_byte>& output )
{
  // Validate input area
  if( region.volume() == 0 )
  {
    return false;
  }

  // Cast image to byte if it is not already
  vil_image_view< vxl_byte > byte_image;
  vil_convert_cast( image, byte_image );

  // Expand bbox size, make sure it is square
  vgl_box_2d< int > scaled_region;
  convert_cast_bbox( region, scaled_region );
  scale_region( scaled_region, settings_.bb_scale_factor, settings_.bb_pixel_shift );

  if( !settings_.stretch_chip )
  {
    if( scaled_region.width() < scaled_region.height() )
    {
      scaled_region.set_width( scaled_region.height() );
    }
    else
    {
      scaled_region.set_height( scaled_region.width() );
    }
  }

  // Create new single channel image
  vil_image_view< vxl_byte > input_region = point_view_to_region( byte_image, scaled_region );

  // Validate extracted region size for min size allowed for below operations
  if( input_region.ni() < 4 || input_region.nj() < 4 )
  {
    return false;
  }

  // Create output chip
  const int chip_length = settings_.chip_length;

  output = vil_image_view< vxl_byte >( chip_length, chip_length, image.nplanes() );

  if( settings_.stretch_chip )
  {
    vil_resample_bilin( input_region, output, chip_length, chip_length );
  }
  else
  {
    int xmin_offset = std::max( 0, -scaled_region.min_x() );
    int xmax_offset = std::max( 0, scaled_region.max_x() - static_cast< int >( image.ni() ) );
    int ymin_offset = std::max( 0, -scaled_region.min_y() );
    int ymax_offset = std::max( 0, scaled_region.max_y() - static_cast< int >( image.nj() ) );

    if( xmin_offset == 0 && xmax_offset == 0 && ymin_offset == 0 && ymax_offset == 0 )
    {
      vil_resample_bilin( input_region, output, chip_length, chip_length );
    }
    else
    {
      // Zero bad border around invalid regions
      output.fill( 0 );

      vgl_box_2d< int > subregion(
        chip_length * static_cast< double >( xmin_offset ) / scaled_region.width(),
        chip_length * ( 1.0 - static_cast< double >( xmax_offset ) / scaled_region.width() ),
        chip_length * static_cast< double >( ymin_offset ) / scaled_region.height(),
        chip_length * ( 1.0 - static_cast< double >( ymax_offset ) / scaled_region.height() ) );

      vil_image_view< vxl_byte > output_subregion = point_view_to_region( output, subregion );
      vil_resample_bilin( input_region, output_subregion, chip_length, chip_length );
    }
  }

  return true;
}

} // end namespace vidtk
