/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/integral_hog_generator.h>

#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <vil/vil_resample_bilin.h>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "integral_hog_generator_cxx" );

template <typename PixType>
integral_hog_generator<PixType>
::integral_hog_generator()
{
}

template <typename PixType>
bool
integral_hog_generator<PixType>
::configure( const external_settings& settings )
{
  integral_hog_settings const* desc_settings =
    dynamic_cast<integral_hog_settings const*>(&settings);

  if( !desc_settings )
  {
    LOG_ERROR( "Received invalid settings structure" );
    return false;
  }

  // Create internal copy of inputted external settings
  this->settings_ = *desc_settings;

  // Check if a chip image path is set
  is_training_mode_ = !settings_.chip_extraction_prefix.empty();

  // Set internal descriptor running mode options
  generator_settings running_options;
  running_options.thread_count = settings_.num_threads;
  running_options.sampling_rate = settings_.descriptor_skip_count;
  this->set_operating_mode( running_options );

  // Load integral hog model
  if( !model_.load_model( settings_.model_filename ) || !model_.is_valid() )
  {
    LOG_ERROR( "Unable to load integral hog model" );
    return false;
  }

  return true;
}


template <typename PixType>
external_settings*
integral_hog_generator<PixType>
::create_settings()
{
  return new integral_hog_settings;
}


template <typename PixType>
bool
integral_hog_generator<PixType>
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
integral_hog_generator<PixType>
::new_track_routine( track_sptr const& new_track )
{
  // Create new data for this track, and initialize counters
  integral_hog_track_data* track_data = new integral_hog_track_data;
  track_data->frames_observed = 0;
  track_data->compute_mode = true;
  track_data->wip = create_descriptor( settings_.descriptor_name, new_track->id() );
  track_data->wip->resize_features( model_.feature_count(), 0.0 );
  return track_data_sptr(track_data);
}


template <typename PixType>
bool
integral_hog_generator<PixType>
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  integral_hog_track_data* data = dynamic_cast<integral_hog_track_data*>(track_data.get());

  // Decide if we only want to process frames with new bboxes
  if( settings_.new_states_w_bbox_only )
  {
    const timestamp& current_ts = active_track->last_state()->time_;
    vgl_box_2d<unsigned> tmp_bbox;

    if( !active_track->last_state()->bbox( tmp_bbox ) ||
        ( data->last_state_ts.has_time() &&
          data->last_state_ts.time() == current_ts.time() ) )
    {
      return true;
    }

    data->last_state_ts = current_ts;
  }

  // Increase observed frames count
  data->frames_observed++;

  // Add history reference to descriptor
  descriptor_history_entry region = generate_history_entry( current_frame(), active_track );
  data->wip->add_history_entry( region );

  // If in compute mode, add this region to the descriptor
  if( data->compute_mode )
  {
    // Expand BB to desired size and format
    vil_image_view<float> fp_track_region;

    if( !compute_track_snippet( current_frame_, region.get_image_location(), fp_track_region ) )
    {
      return true;
    }

    // Compute descriptor for region, and add to moving average
    std::vector<double>& feature_space = data->features;
    std::vector<double>& output_space = data->wip->get_features();

    if( is_training_mode_ )
    {
      static unsigned chip_id = 1;
      std::string chip_fn = settings_.chip_extraction_prefix +
        boost::lexical_cast< std::string >( chip_id++ ) + ".png";
      vil_image_view<vxl_byte> converted_image;
      vil_convert_cast( fp_track_region, converted_image );
      vil_save( converted_image, chip_fn.c_str() );
    }
    else
    {
      integral_hog_descriptor< float > computer( fp_track_region );

      computer.generate_hog_descriptor(
        vgl_box_2d< unsigned >( 0, fp_track_region.ni(), 0, fp_track_region.nj() ),
        model_, feature_space );

      for( unsigned i = 0; i < output_space.size(); i++ )
      {
        output_space[i] += feature_space[i];
      }
    }
  }

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.descriptor_length )
  {
    // Dump and normalize output
    normalize_descriptor( data->wip );

    // Add outgoing descriptor
    this->add_outgoing_descriptor( data->wip );

    // Make new wip descriptor and reset counters
    data->frames_observed = 0;
    data->compute_mode = false;
    data->wip = create_descriptor(settings_.descriptor_name, active_track->id());
    data->wip->resize_features( model_.feature_count(), 0.0 );
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
integral_hog_generator<PixType>
::terminated_track_routine( track_sptr const& /*finished_track*/,
                            track_data_sptr track_data )
{
  integral_hog_track_data* data = dynamic_cast<integral_hog_track_data*>(track_data.get());

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.min_frames_for_desc )
  {
    // Dump and normalize output
    normalize_descriptor( data->wip );

    // Add outgoing descriptor
    this->add_outgoing_descriptor( data->wip );
  }
  return true;
}

template <typename PixType>
bool
integral_hog_generator<PixType>
::compute_track_snippet( const vil_image_view<PixType>& img,
                         const image_region& region,
                         vil_image_view<float>& output )
{
  // Validate input area
  if( region.volume() == 0 )
  {
    return false;
  }

  // TODO: Need to convert option for warping based on velocity to vidtk version

  // Expand bbox size, union with image bounds
  image_region new_region = region;
  scale_region( new_region, settings_.bb_scale_factor, settings_.bb_pixel_shift );

  // Create new single channel image
  vil_image_view<PixType> rgb_region = point_view_to_region( img, new_region );

  // Validate extracted region size
  if( rgb_region.ni() < 4 || rgb_region.nj() < 4 )
  {
    return false;
  }

  if( rgb_region.nplanes() == 3 )
  {
    vil_convert_planes_to_grey( rgb_region, output, 0.3333, 0.3333, 0.3333 );
  }
  else
  {
    vil_convert_cast( rgb_region, output );
  }
  if( settings_.enable_adaptive_resize )
  {
    // Validate input region
    vil_image_view<float> resized;

    int region_area = output.size();

    if( region_area == 0 )
    {
      return false;
    }

    const double desired_min_area = settings_.desired_min_area;
    const double desired_max_area = settings_.desired_max_area;

    double resize_factor = 1.0;

    if( region_area >= desired_min_area && region_area <= desired_max_area )
    {
      return true;
    }
    else if( region_area < desired_min_area )
    {
      resize_factor = std::sqrt( desired_min_area / region_area );
    }
    else
    {
      resize_factor = std::sqrt( desired_max_area / region_area );
    }

    // Calculate and validate resizing parameters
    unsigned new_width = static_cast<unsigned>(resize_factor * output.ni());
    unsigned new_height = static_cast<unsigned>(resize_factor * output.nj());

    if( new_width < 4 || new_height < 4 )
    {
      return false;
    }

    // Resample
    vil_image_view<float> resize;
    vil_resample_bilin( output, resize, new_width, new_height );
    output = resize;
  }

  return true;
}

} // end namespace vidtk
