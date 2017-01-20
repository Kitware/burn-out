/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "multi_color_descriptor_generator.h"

#include <tracking_data/pvo_probability.h>
#include <utilities/vxl_to_cv_converters.h>

#include <vil/vil_transform.h>
#include <vil/vil_copy.h>

namespace vidtk
{


template <typename PixType>
multi_color_descriptor_generator<PixType>
::multi_color_descriptor_generator()
{
  this->configure( multi_color_descriptor_settings() );
}

template <typename PixType>
bool multi_color_descriptor_generator<PixType>
::configure( const external_settings& settings )
{
  multi_color_descriptor_settings const* desc_settings =
    dynamic_cast<multi_color_descriptor_settings const*>(&settings);

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

  // Compute total resolution for when the input is 3-channeled
  total_resolution_ = settings_.resolution_per_chan *
                      settings_.resolution_per_chan *
                      settings_.resolution_per_chan;

  // Set color space if valid
  if( settings_.color_space == "RGB" )
  {
    color_space_ = MF_RGB;
  }
  else if( settings_.color_space == "Lab" || settings_.color_space == "LAB" )
  {
    color_space_ = MF_LAB;
  }
  else if( settings_.color_space == "HSV" )
  {
    color_space_ = MF_HSV;
  }
  else
  {
    this->report_error( "Invalid color space specified: " + settings_.color_space );
  }

  if( settings_.use_color_commonality && settings_.use_track_mask )
  {
    weight_mode_ = MF_COMBO;
  }
  else if( settings_.use_color_commonality )
  {
    weight_mode_ = MF_CCI;
  }
  else if( settings_.use_track_mask )
  {
    weight_mode_ = MF_TRACKMASK;
  }
  else
  {
    weight_mode_ = MF_NONE;
  }

  // Set color commonality filtering settings
  cci_settings_.resolution_per_channel = settings_.resolution_per_chan;
  cci_settings_.histogram = &( this->cci_histogram_ );

  return true;
}


template <typename PixType>
external_settings* multi_color_descriptor_generator<PixType>
::create_settings()
{
  return new multi_color_descriptor_settings;
}


template <typename PixType>
bool multi_color_descriptor_generator<PixType>
::frame_update_routine()
{
  // Validate input frame format
  const frame_sptr& frame_data = this->current_frame();
  vil_image_view< PixType > current_image = frame_data->image< PixType >();

  if( !current_image || current_image.nplanes() != 3 )
  {
    this->report_error( "Input image must contain 3 planes!" );
    return false;
  }

  // Don't process any further when there are no tracks
  if( this->current_frame()->active_tracks().size() == 0 )
  {
    return true;
  }

  // Create a copy of the shared input frame and an ocv pointer to the data
  if( filtered_frame_.ni() != current_image.ni() ||
      filtered_frame_.nj() != current_image.nj() ||
      filtered_frame_.nplanes() != current_image.nplanes() )
  {
    filtered_frame_ = vil_image_view< PixType >( current_image.ni(),
                                                 current_image.nj(),
                                                 1,
                                                 current_image.nplanes() );
  }

  vil_copy_reformat( current_image, filtered_frame_ );

  // White balancing
  if( settings_.perform_auto_white_balancing )
  {
    white_balancer_.apply( filtered_frame_ );
  }

  // Create OpenCV image copy
  deep_cv_conversion( filtered_frame_, filtered_frame_ocv_ );

  // Color space conversion
  if( color_space_ == MF_LAB )
  {
    cv::cvtColor( filtered_frame_ocv_, filtered_frame_ocv_, CV_RGB2HSV );
  }
  else if( color_space_ == MF_HSV )
  {
    cv::cvtColor( filtered_frame_ocv_, filtered_frame_ocv_, CV_RGB2Lab );
  }

  // Color commonality computation
  if( settings_.use_color_commonality )
  {
    if( color_commonality_image_.ni() != current_image.ni() ||
        color_commonality_image_.nj() != current_image.nj() ||
        color_commonality_image_.nplanes() != current_image.nplanes() )
    {
      color_commonality_image_ = vil_image_view< float >( current_image.ni(),
                                                          current_image.nj() );
    }

    // Calculate cci
    color_commonality_filter( current_image, color_commonality_image_, cci_settings_ );
    deep_cv_conversion( color_commonality_image_, color_commonality_image_ocv_ );
  }

  return true;
}

template <typename PixType>
track_data_sptr multi_color_descriptor_generator<PixType>
::new_track_routine( track_sptr const& new_track )
{
  // Create new data for this track, and initialize counters
  hist_track_data* track_data = new hist_track_data;
  track_data->frames_observed = 0;
  track_data->compute_mode = true;
  track_data->wip = create_descriptor( settings_.histogram_output_id, new_track->id() );
  track_data->hist.AllocateHistogram( settings_.resolution_per_chan );
  return track_data_sptr(track_data);
}

// Helper functions for the below operation which merges a color commonality
// image and some tracking mask to give certain weights to different pixels
// in a region. Currently, these scaling parameters are locked to fixed values,
// but may be made parameters later.

const float hs_value_adj = 0.001f; // Added to each input value
const float hs_scale_factor_no_mask = 0.2f;
const float hs_scale_factor_mask_pixel = 1.0f;
const float hs_scale_factor_nonmask_pixel = 0.1f;

static inline float hyperbolic_scaling( const float value )
{
  return hs_scale_factor_no_mask / ( value + hs_value_adj );
}

static inline float hyperbolic_scaling_w_mask( const float value, const bool mask )
{
  if( mask )
  {
    return hs_scale_factor_mask_pixel / ( value + hs_value_adj );
  }
  return hs_scale_factor_nonmask_pixel / ( value + hs_value_adj );
}

// Given an input mask and color commonality snippet for some region
// compute an estimated weight for each pixel
void compute_pixel_weights( const vil_image_view< float >& cci_image,
                            const vil_image_view< bool >& mask_image,
                            cv::Mat& pixel_weights )
{
  vil_image_view< float > output;
  output.deep_copy( cci_image );
  vil_transform( output, mask_image, output, hyperbolic_scaling_w_mask );
  deep_cv_conversion( output, pixel_weights );
}

void compute_pixel_weights( const vil_image_view< float >& cci_image,
                            cv::Mat& pixel_weights )
{
  vil_image_view< float > output;
  output.deep_copy( cci_image );
  vil_transform( output, hyperbolic_scaling );
  deep_cv_conversion( output, pixel_weights );
}

// Called for each track per frame in order to update models and output descriptors
template <typename PixType>
bool multi_color_descriptor_generator<PixType>
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  hist_track_data* data = dynamic_cast<hist_track_data*>(track_data.get());

  // Increase observed frames count
  data->frames_observed++;

  // Get reccommended track history entry
  descriptor_history_entry region = generate_history_entry( current_frame(), active_track );
  data->wip->add_history_entry( region );

  // If in compute mode, add this region to the descriptor
  if( data->compute_mode )
  {
    // Format input color region
    cv::Mat ocv_region_ptr, ocv_mask_ptr;
    vil_image_view<PixType> track_region_ptr = point_view_to_region( filtered_frame_, region.get_image_location() );

    if( settings_.smooth_image )
    {
      deep_cv_conversion( track_region_ptr, ocv_region_ptr, false );
      cv::blur( ocv_region_ptr, ocv_region_ptr, cv::Size(3,3) );
    }
    else
    {
      deep_cv_conversion( track_region_ptr, ocv_region_ptr );
    }

    bool update_model_this_frame = true;

    // Compute input weights for each pixel within this BBOX
    switch( weight_mode_ )
    {
      case MF_COMBO:
      {
        vil_image_view<float> cci_region;
        point_view_to_region( color_commonality_image_, region.get_image_location(), cci_region );
        vil_image_view<bool> tracking_mask = get_tracking_mask( active_track );
        if( !tracking_mask || tracking_mask.ni() != cci_region.ni() || tracking_mask.nj() != cci_region.nj() )
        {
          compute_pixel_weights( cci_region, ocv_mask_ptr );
        }
        else
        {
          compute_pixel_weights( cci_region, tracking_mask, ocv_mask_ptr );
        }
        break;
      }
      case MF_CCI:
      {
        vil_image_view<float> cci_region;
        point_view_to_region( color_commonality_image_, region.get_image_location(), cci_region );
        compute_pixel_weights( cci_region, ocv_mask_ptr );
        break;
      }
      case MF_TRACKMASK:
      {
        vil_image_view<bool> tracking_mask = get_tracking_mask( active_track );
        if( !tracking_mask || tracking_mask.ni() != track_region_ptr.ni()
                           || tracking_mask.nj() != track_region_ptr.nj() )
        {
          update_model_this_frame = false;
        }
        else
        {
          deep_cv_conversion( tracking_mask, ocv_mask_ptr, false );
        }
        break;
      }
      case MF_NONE:
        break;
    }

    // Finally, update the internal model given some filtered image and weight mask
    IplImage ipl_region_ptr = ocv_region_ptr;
    if( update_model_this_frame )
    {
      switch( weight_mode_ )
      {
        case MF_COMBO:
        case MF_CCI:
        case MF_TRACKMASK:
        {
          IplImage ipl_mask_ptr = ocv_mask_ptr;
          data->hist.UpdateModelFromImage( &ipl_region_ptr, &ipl_mask_ptr );
          break;
        }
        default:
        {
          data->hist.UpdateModelFromImage( &ipl_region_ptr );
          break;
        }
      }
    }
  }

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.descriptor_length )
  {
    output_descriptors_for_track( active_track, *data );
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
bool multi_color_descriptor_generator<PixType>
::terminated_track_routine( track_sptr const& finished_track,
                            track_data_sptr track_data )
{
  hist_track_data* data = dynamic_cast<hist_track_data*>(track_data.get());

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.min_frames_for_incomplete )
  {
    output_descriptors_for_track( finished_track, *data );
  }
  return true;
}

template <typename PixType>
void multi_color_descriptor_generator<PixType>
::output_descriptors_for_track( track_sptr const& active_track, hist_track_data& data )
{
  // Dump and normalize descriptor outputs if possible
  if( data.hist.StoredWeight() > 0.0 )
  {
    descriptor_sptr motion_hist = data.wip;
    descriptor_sptr single_value = create_descriptor( settings_.single_value_output_id );
    descriptor_sptr object_value = create_descriptor( settings_.person_output_id );

    copy_aux_descriptor_info( motion_hist, single_value );
    copy_aux_descriptor_info( motion_hist, object_value );

    if( settings_.smooth_histogram )
    {
      data.hist.SmoothHistogram();
    }

    if( settings_.filter_dark_pixels )
    {
      data.hist.BlackFilter();
    }

    motion_hist->resize_features( total_resolution_ );
    single_value->resize_features( 6 );
    object_value->resize_features( 8 );

    data.hist.DumpOutputs( motion_hist->get_features(),
                           single_value->get_features(),
                           object_value->get_features() );

    // Add outgoing descriptor
    this->add_outgoing_descriptor( motion_hist );
    this->add_outgoing_descriptor( single_value );

    // Handle duplicate flag
    if( !settings_.duplicate_histogram_output_id.empty() )
    {
      descriptor_sptr copy = new raw_descriptor( *motion_hist );
      copy->set_type( settings_.duplicate_histogram_output_id );
      this->add_outgoing_descriptor( copy );
    }
    if( !settings_.duplicate_single_value_output_id.empty() )
    {
      descriptor_sptr copy = new raw_descriptor( *single_value );
      copy->set_type( settings_.duplicate_single_value_output_id );
      this->add_outgoing_descriptor( copy );
    }

    // Only output a person descriptor based on PVO values
    vidtk::pvo_probability prob;

    if( active_track->get_pvo(prob) )
    {
      double pprob = prob.get_probability_person();
      double vprob = prob.get_probability_vehicle();

      if( pprob > vprob )
      {
        this->add_outgoing_descriptor( object_value );

        if( !settings_.duplicate_person_output_id.empty() )
        {
          descriptor_sptr copy = new raw_descriptor( *object_value );
          copy->set_type( settings_.duplicate_person_output_id );
          this->add_outgoing_descriptor( copy );
        }
      }
    }
  }

  // Make new wip descriptor and reset counters
  data.frames_observed = 0;
  data.compute_mode = false;
  data.wip = create_descriptor( settings_.histogram_output_id, active_track->id() );
}


} // end namespace vidtk
