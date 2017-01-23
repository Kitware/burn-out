/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <descriptors/icosahedron_hog_generator.h>

#include <logger/logger.h>

#include <vil/vil_plane.h>
#include <vil/vil_resample_bilin.h>
#include <vil/vil_math.h>

namespace vidtk
{

template <typename PixType>
bool icosahedron_hog_generator<PixType>
::configure( const external_settings& settings )
{
  icosahedron_hog_settings const* desc_settings =
    dynamic_cast<icosahedron_hog_settings const*>(&settings);

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

  return true;
}


template <typename PixType>
external_settings* icosahedron_hog_generator<PixType>
::create_settings()
{
  return new icosahedron_hog_settings;
}


template <typename PixType>
track_data_sptr icosahedron_hog_generator<PixType>
::new_track_routine( track_sptr const& /*new_track*/ )
{
  hog_track_data* track_data = new hog_track_data;
  track_data->frames_observed = 0;
  track_data->start_frame_id = this->current_frame()->frame_number();
  track_data->cropped_image_history.set_capacity( settings_.descriptor_length + 2 );
  track_data->region_history.set_capacity( settings_.descriptor_length + 2 );
  return track_data_sptr(track_data);
}

template <typename PixType>
bool icosahedron_hog_generator<PixType>
::terminated_track_routine( track_sptr const& /*finished_track*/,
                            track_data_sptr /*track_data*/ )
{
  return true;
}


// Given a series of grayscale regions, normalize their sizes to a specified size
bool format_data_cube( const boost::circular_buffer< vil_image_view<float> >& buffer,
                       std::vector< vil_image_view<float> >& data,
                       const icosahedron_hog_settings& options )
{
  data.clear();

  // Calculate average dimension
  unsigned avg_width = 0;
  unsigned avg_height = 0;

  if( buffer.size() == 0 )
  {
    return false;
  }

  for( unsigned int i = 0; i < buffer.size(); i++ )
  {
    avg_width += buffer[i].ni();
    avg_height += buffer[i].nj();
  }

  avg_width /= buffer.size();
  avg_height /= buffer.size();

  unsigned area = avg_width * avg_height;

  if( area == 0 )
  {
    return false;
  }

  if( area < options.min_region_area )
  {
    double scale_factor = std::sqrt( static_cast<double>(options.min_region_area) ) /
                          std::sqrt( static_cast<double>(area) );

    avg_width = static_cast<unsigned>(scale_factor * avg_width );
    avg_height = static_cast<unsigned>(scale_factor * avg_height );
  }

  // Resample all images to the average dimensions
  for( unsigned int i = 0; i < buffer.size(); i++ )
  {
    vil_image_view< float > layer;
    vil_resample_bilin( buffer[i], layer, avg_width, avg_height );
    data.push_back( layer );
  }

  return true;
}

// Copies a region from an RGB image to a preallocated grayscale image
template< typename PixType >
void crop_rgb_image_to_gray( const vil_image_view< PixType >& src,
                             const vgl_box_2d< unsigned >& region,
                             vil_image_view< float >& dst )
{
  assert( src.nplanes() == 3 );
  vil_image_view< PixType > cropped = point_view_to_region( src, region );
  vil_convert_planes_to_grey( cropped, dst );
}

template <typename PixType>
bool icosahedron_hog_generator<PixType>
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  hog_track_data* data = dynamic_cast<hog_track_data*>(track_data.get());

  // Validate input frame and track bounding boxes
  const frame_sptr& frame_data = this->current_frame();
  vil_image_view< PixType > frame = frame_data->image< PixType >();
  descriptor_history_entry region = generate_history_entry( frame_data, active_track );

  // Validate frame
  if( !frame || frame.nplanes() != 3 )
  {
    return false;
  }

  // Crop out region in frame and add it to our history
  vil_image_view<float> copied_region;

  crop_rgb_image_to_gray( frame, region.get_image_location(), copied_region );

  data->cropped_image_history.push_back( copied_region );
  data->region_history.push_back( region );

  // Determine if we need to compute a descriptor this iteration
  unsigned int current_id = this->current_frame()->frame_number();
  unsigned int frames_since_first_log = current_id - data->start_frame_id;

  // We need 2 extra frames to reach descriptor length because we lose 2
  // frames when calculating gradients in time.
  if( frames_since_first_log >= settings_.descriptor_length + 2 &&
      ( frames_since_first_log - 2 ) % settings_.descriptor_spacing == 0 )
  {
    // Compute descriptor
    descriptor_sptr icos_descriptor = create_descriptor( settings_.descriptor_name,
                                                         active_track->id() );

    // Format input data cube
    std::vector< vil_image_view<float> > input;
    if( format_data_cube( data->cropped_image_history, input, settings_ ) )
    {
      // Calculate features
      icosahedron_hog_parameters descriptor_options;
      descriptor_options.time_cells = settings_.time_cells;
      descriptor_options.width_cells = settings_.width_cells;
      descriptor_options.height_cells = settings_.height_cells;

      if( calculate_icosahedron_hog_features( input, icos_descriptor->get_features(), descriptor_options ) )
      {
        // Append history from moving buffer into decriptor
        // Need to convert from boost circular buffer to descriptor_history vector
        descriptor_history_t local_history;

        local_history.insert( local_history.end(),
                              data->region_history.begin(),
                              data->region_history.end() );

        icos_descriptor->set_history( local_history );

        // Add outgoing descriptor
        this->add_outgoing_descriptor( icos_descriptor );
      }
    }
  }
  return true;
}

} // end namespace vidtk
