/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/mfeat_bow_generator.h>

#include <vil/vil_convert.h>
#include <vil/vil_resample_bilin.h>

namespace vidtk
{


template <typename PixType>
mfeat_bow_generator<PixType>
::mfeat_bow_generator()
{
}

template <typename PixType>
bool mfeat_bow_generator<PixType>
::configure( const external_settings& settings )
{
  mfeat_bow_settings const* desc_settings =
    dynamic_cast<mfeat_bow_settings const*>(&settings);

  if( !desc_settings )
  {
    return false;
  }

  // Create internal copy of inputted external settings
  this->settings_ = *desc_settings;

  // Set internal descriptor running mode options
  generator_settings running_options;
  running_options.thread_count = settings_.num_threads;
  running_options.sampling_rate = settings_.descriptor_skip_count;
  this->set_operating_mode( running_options );

  // Attempt to load BoW model
  is_model_loaded_ = false;

  if( settings_.bow_filename.size() > 0 )
  {
    mfeat_computer_settings computer_settings;

    computer_settings.bow_filename = settings_.bow_filename;

    computer_settings.compute_hessian = settings_.compute_hessian;
    computer_settings.compute_harris_laplace = settings_.compute_harris_laplace;
    computer_settings.compute_dog = settings_.compute_dog;
    computer_settings.compute_hessian_laplace = settings_.compute_hessian_laplace;
    computer_settings.compute_multiscale_hessian = settings_.compute_multiscale_hessian;
    computer_settings.compute_multiscale_harris = settings_.compute_multiscale_harris;
    computer_settings.compute_mser = settings_.compute_mser;

    is_model_loaded_ = computer_.configure( computer_settings );

    if( !is_model_loaded_ )
    {
      this->report_error( "unable to load supplied BoW model!" );
      return false;
    }
  }

  // Set size variable
  if( is_model_loaded_ )
  {
    const mfeat_vocabulary& vocab = computer_.vocabulary();
    vocabulary_size_ = vocab.word_count();
  }

  return true;
}


template <typename PixType>
external_settings* mfeat_bow_generator<PixType>
::create_settings()
{
  return new mfeat_bow_settings;
}


template <typename PixType>
bool mfeat_bow_generator<PixType>
::frame_update_routine()
{
  // Validate that a model was loaded
  if( !is_model_loaded_ )
  {
    this->report_error( "No valid bow model provided!" );
    return false;
  }

  // Validate input frame format
  const frame_sptr& frame_data = this->current_frame();
  current_frame_ = frame_data->image< PixType >();

  if( !current_frame_ || (current_frame_.nplanes() != 3 && current_frame_.nplanes() != 1 ) )
  {
    this->report_error( "Input image must contain 1 or 3 planes!" );
    return false;
  }
  return true;
}

template <typename PixType>
track_data_sptr mfeat_bow_generator<PixType>
::new_track_routine( track_sptr const& new_track )
{
  // Create new data for this track, and initialize counters
  mfeat_track_data* track_data = new mfeat_track_data();
  track_data->frames_observed = 0;
  track_data->compute_mode = true;
  track_data->wip = create_descriptor( settings_.descriptor_name, new_track->id() );
  track_data->wip->resize_features( vocabulary_size_, 0.0 );
  return track_data_sptr( track_data );
}


template <typename PixType>
bool mfeat_bow_generator<PixType>
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  mfeat_track_data* data = dynamic_cast<mfeat_track_data*>(track_data.get());

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
    compute_track_snippet( current_frame_, region.get_image_location(), fp_track_region );

    // Validate region size
    if( fp_track_region.ni() < settings_.min_chip_length ||
        fp_track_region.nj() < settings_.min_chip_length )
    {
      return true;
    }

    // Compute descriptor for region, and add to moving average
    std::vector<float>& feature_space = data->features;
    std::vector<double>& mapping_space = data->bow_mapping;
    descriptor_data_t& output_space = data->wip->get_features();

    computer_.compute_features( fp_track_region, feature_space );
    computer_.compute_bow_descriptor( feature_space, mapping_space );

    for( unsigned i = 0; i < output_space.size(); i++ )
    {
      output_space[i] += mapping_space[i];
    }
  }

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.descriptor_length )
  {
    // Dump and normalize output
    normalize_descriptor( data->wip );

    // Add outgoing descriptor
    this->add_outgoing_descriptor( data->wip );

    // Handle duplicate flag
    if( !settings_.duplicate_name.empty() )
    {
      descriptor_sptr copy = new raw_descriptor( *data->wip );
      copy->set_type( settings_.duplicate_name );
      this->add_outgoing_descriptor( copy );
    }

    // Make new wip descriptor and reset counters
    data->frames_observed = 0;
    data->compute_mode = false;
    data->wip = create_descriptor( settings_.descriptor_name, active_track->id() );
    data->wip->resize_features( vocabulary_size_, 0.0 );
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
bool mfeat_bow_generator<PixType>
::terminated_track_routine( track_sptr const& /*finished_track*/,
                            track_data_sptr track_data )
{
  mfeat_track_data* data = dynamic_cast<mfeat_track_data*>(track_data.get());

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.min_frames_for_desc )
  {
    // Dump and normalize output
    normalize_descriptor( data->wip );

    // Add outgoing descriptor
    this->add_outgoing_descriptor( data->wip );

    // Handle duplicate flag
    if( !settings_.duplicate_name.empty() )
    {
      descriptor_sptr copy = new raw_descriptor( *data->wip );
      copy->set_type( settings_.duplicate_name );
      this->add_outgoing_descriptor( copy );
    }
  }
  return true;
}

template <typename PixType>
void mfeat_bow_generator<PixType>
::compute_track_snippet( const vil_image_view<PixType>& img,
                         const image_region& region,
                         vil_image_view<float>& output )
{
  // Expand bbox size, union with image bounds
  image_region new_region = region;
  scale_region( new_region, settings_.bb_scale_factor, settings_.bb_pixel_shift );

  // Create new single channel image
  vil_image_view<PixType> rgb_region = point_view_to_region( img, new_region );

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
      return;
    }

    const double desired_min_area = settings_.desired_min_area;
    const double desired_max_area = settings_.desired_max_area;

    double resize_factor = 1.0;

    if( region_area >= desired_min_area && region_area <= desired_max_area )
    {
      return;
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
    unsigned new_width = static_cast<unsigned>( resize_factor * output.ni() );
    unsigned new_height = static_cast<unsigned>( resize_factor * output.nj() );

    // Resample
    vil_image_view<float> resize;
    vil_resample_bilin( output, resize, new_width, new_height );
    output = resize;
  }
}

} // end namespace vidtk
