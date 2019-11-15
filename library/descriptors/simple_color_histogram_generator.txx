/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/simple_color_histogram_generator.h>

#include <utilities/vxl_to_cv_converters.h>

#include "opencv2/imgproc.hpp"

namespace vidtk
{


template <typename PixType>
simple_color_histogram_generator<PixType>
::simple_color_histogram_generator()
{
  this->configure( simple_color_histogram_settings() );
}

template <typename PixType>
bool simple_color_histogram_generator<PixType>
::configure( const external_settings& settings )
{
  simple_color_histogram_settings const* desc_settings =
    dynamic_cast<simple_color_histogram_settings const*>(&settings);

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

  // Set each resolution bin
  total_resolution_ = settings_.resolution_per_chan *
                      settings_.resolution_per_chan *
                      settings_.resolution_per_chan;

  return true;
}


template <typename PixType>
external_settings* simple_color_histogram_generator<PixType>
::create_settings()
{
  return new simple_color_histogram_settings;
}


template <typename PixType>
track_data_sptr simple_color_histogram_generator<PixType>
::new_track_routine( track_sptr const& new_track )
{
  // Create new data for this track, and initialize counters
  hist_track_data* track_data = new hist_track_data;
  track_data->frames_observed = 0;
  track_data->compute_mode = true;
  track_data->wip = create_descriptor( settings_.descriptor_name, new_track->id() );
  track_data->hist.AllocateHistogram( settings_.resolution_per_chan );
  return track_data_sptr(track_data);
}


template <typename PixType>
bool simple_color_histogram_generator<PixType>
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  hist_track_data* data = dynamic_cast<hist_track_data*>(track_data.get());

  // Increase observed frames count
  data->frames_observed++;

  // Validate input frame and track bounding boxes
  const frame_sptr& frame_data = this->current_frame();
  vil_image_view< PixType > image = frame_data->image< PixType >();
  descriptor_history_entry region = generate_history_entry( frame_data, active_track );
  data->wip->add_history_entry( region );

  if( !image || image.nplanes() != 3 )
  {
    this->report_error( "Invalid image provided!\n" );
    return false;
  }

  // If in compute mode, add this region to the descriptor
  if( data->compute_mode )
  {
    vil_image_view<PixType> track_region_ptr = point_view_to_region( image, region.get_image_location() );

    cv::Mat ocv_region_ptr;
    deep_cv_conversion( track_region_ptr, ocv_region_ptr, true );

    if( settings_.smooth_image && ocv_region_ptr.rows > 5 && ocv_region_ptr.cols > 5 )
    {
      cv::Mat smoothed_img( ocv_region_ptr.size(), ocv_region_ptr.type() );
      cv::blur( ocv_region_ptr, smoothed_img, cv::Size( 3, 3 ) );
      ocv_region_ptr = smoothed_img;
    }

    IplImage ipl_region_ptr = ocv_region_ptr;
    data->hist.AddImgToHistogram( &ipl_region_ptr );
  }

  // Check if we should output a descriptor
  if( data->compute_mode && data->frames_observed >= settings_.descriptor_length )
  {
    // Dump and normalize output
    if( settings_.smooth_histogram )
    {
      data->hist.SmoothHistogram();
    }
    data->hist.CopyHistIntoVector( data->wip->get_features() );
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
    data->wip = create_descriptor(settings_.descriptor_name, active_track->id());
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
bool simple_color_histogram_generator<PixType>
::terminated_track_routine( track_sptr const& /*finished_track*/,
                            track_data_sptr track_data )
{
  hist_track_data* data = dynamic_cast<hist_track_data*>(track_data.get());

  if( data->compute_mode && data->frames_observed >= settings_.min_frames_for_incomplete )
  {
    // Dump and normalize output
    if( settings_.smooth_histogram )
    {
      data->hist.SmoothHistogram();
    }
    data->hist.CopyHistIntoVector( data->wip->get_features() );
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


} // end namespace vidtk
