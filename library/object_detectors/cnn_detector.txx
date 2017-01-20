/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/cnn_detector.h>

#include <caffe/blob.hpp>
#include <caffe/common.hpp>

#include <vil/vil_copy.h>
#include <vil/vil_convert.h>
#include <vil/vil_plane.h>
#include <vil/vil_resample_bilin.h>
#include <vil/vil_fill.h>
#include <vil/vil_math.h>
#include <vil/vil_transpose.h>

#include <vil/algo/vil_threshold.h>
#include <vil/algo/vil_blob.h>

#include <utilities/point_view_to_region.h>

#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

#include <algorithm>
#include <map>
#include <numeric>

#ifdef USE_OPENCV
#include <utilities/vxl_to_cv_converters.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif

#include <logger/logger.h>

using caffe::Blob;
using caffe::Net;
using caffe::Caffe;

namespace vidtk
{


VIDTK_LOGGER( "cnn_detector" );


template< class PixType >
bool
cnn_detector< PixType >
::configure( const cnn_detector_settings& settings )
{
  // Select GPU / CPU option
  mode_ = Caffe::GPU;
  device_id_ = settings.device_id;
  device_mem_ = 0.0;

  if( settings.use_gpu == cnn_detector_settings::NO )
  {
    mode_ = Caffe::CPU;
  }
  else if( settings.use_gpu == cnn_detector_settings::AUTO )
  {
    // Auto-detect which GPU to use based on highest memory count
    int device_count = 0;
    cudaGetDeviceCount( &device_count );

    if( device_count <= 0 )
    {
      mode_ = Caffe::CPU;

      LOG_WARN( "Unable to find usable GPU, using CPU only." );
    }
    else
    {
      for( int i = 0; i < device_count; ++i )
      {
        cudaDeviceProp prop;
        cudaGetDeviceProperties( &prop, i );

        if( prop.totalGlobalMem > device_mem_ )
        {
          device_id_ = i;
          device_mem_ = prop.totalGlobalMem;
        }
      }
    }
  }

  // Set mode and device ID
  Caffe::set_mode( mode_ );

  if( mode_ == Caffe::GPU && device_id_ >= 0 )
  {
    Caffe::SetDevice( device_id_ );
  }

  // Load detector if required
  static std::map< std::string, detector_sptr_t > loaded_detectors;
  static boost::mutex load_mutex;

  bool load_model = true;
  bool lock_model = !settings_.reload_key.empty();

  if( lock_model )
  {
    boost::unique_lock< boost::mutex > lock( load_mutex );

    if( loaded_detectors[ settings_.reload_key ] )
    {
      cnn_ = loaded_detectors[ settings_.reload_key ];
      load_model = false;
    }
  }

  if( load_model )
  {
    cnn_.reset( new Net< cnn_float_t >( settings.model_definition, caffe::TEST ) );
    cnn_->CopyTrainedLayersFrom( settings.model_weights );

    if( lock_model )
    {
      boost::unique_lock< boost::mutex > lock( load_mutex );

      loaded_detectors[ settings_.reload_key ] = cnn_;
    }
  }

  if( settings.buffer_length > 1 )
  {
    buffer_.clear();
    buffer_.set_capacity( settings.buffer_length );
  }

  scaling_required_ = ( settings.output_scaling_dims[0] != 1 );

  cropping_required_ = ( settings.output_scaling_dims[1] != 0 ) ||
                       ( settings.output_scaling_dims[2] != 0 );

  if( settings.target_channels.empty() )
  {
    LOG_ERROR( "Detector must contain at least one target channel" );
    return false;
  }

  // Check for output folder existance
  if( !settings.detection_image_folder.empty() )
  {
#ifdef USE_OPENCV
    if( boost::filesystem::exists( settings.detection_image_folder ) )
    {
      LOG_INFO( "Writing debug detection images to: " << settings.detection_image_folder );
    }
    else
    {
      LOG_INFO( "Creating folder for detection images: " << settings.detection_image_folder );
      boost::filesystem::create_directory( settings.detection_image_folder );
    }
#else
    LOG_FATAL( "A build with OpenCV is required to output detection images" );
    return false;
#endif
  }

  if( settings.average_count > 1 )
  {
    averager_.reset( new windowed_frame_averager< cnn_float_t >( false, settings.average_count ) );
  }
  else if( settings.average_count < 0 )
  {
    double factor = static_cast< double >( -settings.average_count ) / 100;
    averager_.reset( new exponential_frame_averager< cnn_float_t >( false, factor ) );
  }
  else
  {
    averager_.reset();
  }

  frame_counter_ = 0;
  settings_ = settings;
  return true;
}


// Compute detection maps for a single image.
template< class PixType >
void
cnn_detector< PixType >
::detect( const input_image_t& image, detection_map_t& map )
{
  std::vector< detection_map_t > batch_output;
  batch_detect( std::vector< input_image_t >( 1, image ), batch_output );

  if( batch_output.size() == 1 )
  {
    map = batch_output[0];
  }
  else
  {
    map.clear();
  }
}


// Compute detection mask for a single image.
template< class PixType >
void
cnn_detector< PixType >
::detect( const input_image_t& image, detection_mask_t& mask )
{
  std::vector< detection_mask_t > batch_output;
  batch_detect( std::vector< input_image_t >( 1, image ), batch_output );

  if( batch_output.size() == 1 )
  {
    mask = batch_output[0];
  }
  else
  {
    mask.clear();
  }
}


// Compute detections from a single image.
template< class PixType >
void
cnn_detector< PixType >
::detect( const input_image_t& image, detection_vec_t& detections )
{
  std::vector< detection_vec_t > batch_output;
  batch_detect( std::vector< input_image_t >( 1, image ), batch_output );

  if( batch_output.size() == 1 )
  {
    detections = batch_output[0];
  }
  else
  {
    detections.clear();
  }
}


// Compute detections from a single image.
template< class PixType >
void
cnn_detector< PixType >
::detect( const input_image_t& image,
          detection_map_t& map,
          detection_mask_t& mask,
          detection_vec_t& detections )
{
  std::vector< detection_map_t > batch_output1;
  std::vector< detection_mask_t > batch_output2;
  std::vector< detection_vec_t > batch_output3;

  batch_detect( std::vector< input_image_t >( 1, image ),
                batch_output1, batch_output2, batch_output3 );

  if( batch_output1.size() == 1 )
  {
    map = batch_output1[0];
    mask = batch_output2[0];
    detections = batch_output3[0];
  }
  else
  {
    map.clear();
    mask.clear();
    detections.clear();
  }
}


// Compute detection maps for multiple images.
template< class PixType >
void
cnn_detector< PixType >
::batch_detect( const std::vector< input_image_t >& images,
                std::vector< detection_map_t >& maps )
{
  frame_counter_++;
  maps.clear();

  if( images.empty() )
  {
    return;
  }

  // Handle optional buffering, buffers until the buffer is full, then
  // proceeds to process the input frames.
  const bool buffering_enabled = ( settings_.buffer_length > 1 );
  unsigned buffered_only_entries = 0;

  if( buffering_enabled && buffer_.capacity() != buffer_.size() )
  {
    buffered_only_entries = std::min( static_cast< unsigned >( images.size() ),
                                      buffer_.capacity() - buffer_.size() - 1 );

    bool ready_to_process = true;

    for( unsigned i = 0; i < buffered_only_entries; ++i )
    {
      ready_to_process = ( buffer_.size() + 1 >= buffer_.capacity() );
      buffer_.insert( images[i] );

      if( !ready_to_process )
      {
        maps.push_back( detection_map_t() );
      }
    }

    // We lack enough frames to fill the buffer on this call, return
    if( !ready_to_process )
    {
      return;
    }
  }

  // Re-configure caffe backend, ensure GPU present
  Caffe::set_mode( mode_ );

  if( mode_ == Caffe::GPU && device_id_ >= 0 )
  {
    Caffe::SetDevice( device_id_ );
  }

  // Format input
  const unsigned entry_count = images.size();
  const unsigned ni = ( settings_.transpose_input ? images[0].nj() : images[0].ni() );
  const unsigned nj = ( settings_.transpose_input ? images[0].ni() : images[0].nj() );
  const unsigned nplanes = ( settings_.squash_channels ? 1 : images[0].nplanes() );
  const unsigned planes_per_entry = nplanes * ( buffering_enabled ? buffer_.capacity() : 1 );

  LOG_DEBUG( "Converting vxl image(s) to caffe blob" );

  Blob< cnn_float_t >* input_blob = cnn_->input_blobs()[0];

  input_blob->Reshape( entry_count, planes_per_entry, nj, ni );

  for( unsigned i = buffered_only_entries; i < images.size(); ++i )
  {
    cnn_float_t* entry_start = input_blob->mutable_cpu_data() + input_blob->offset( i );

    if( buffering_enabled )
    {
      buffer_.insert( images[i] );

      if( settings_.detection_image_offset < 0 )
      {
        debug_output( buffer_.datum_at( buffer_.size() / 2 - 1 ), "input", i );
      }
      else
      {
        debug_output( buffer_.datum_at( settings_.detection_image_offset ), "input", i );
      }

      for( unsigned j = 0; j < buffer_.size(); ++j )
      {
        const unsigned offset = ni * nj * nplanes * j;
        const unsigned index = ( settings_.recent_frame_first ? j : buffer_.size() - j - 1 );

        input_image_t image = buffer_.datum_at( index );

        vil_image_view< cnn_float_t > tmp( entry_start + offset, ni, nj, nplanes, 1, ni, ni*nj );
        copy_image_to_blob( image, tmp );
      }
    }
    else
    {
      debug_output( images[i], "input", i );

      vil_image_view< cnn_float_t > tmp( entry_start, ni, nj, nplanes, 1, ni, ni*nj );
      copy_image_to_blob( images[i], tmp );
    }
  }

  // Run CNN on all inputs at once
  LOG_DEBUG( "Running CNN on caffe blob" );

  cnn_->ForwardPrefilled();

  // Retrieve descriptors from internal CNN layers
  LOG_DEBUG( "Converting CNN output to vidtk format" );

  Blob< cnn_float_t >* cnn_output = cnn_->output_blobs()[0];

  for( int i = 0; i < cnn_output->num(); ++i )
  {
    // Create vil image view around output image
    const cnn_float_t* start = cnn_output->cpu_data() + cnn_output->offset( i );

    vil_image_view< cnn_float_t > wrapped_image( start,
      cnn_output->width(), cnn_output->height(), cnn_output->channels(),
      1, cnn_output->width(), cnn_output->width() * cnn_output->height() );

    vil_image_view< cnn_float_t > desired_response;

    if( settings_.target_channels.size() == 1 )
    {
      desired_response = vil_plane( wrapped_image, settings_.target_channels[0] );

      if( settings_.output_option == cnn_detector_settings::PROB_DIFF &&
          wrapped_image.nplanes() == 2 )
      {
        vil_image_view< cnn_float_t > copied_response;
        vil_copy_deep( desired_response, copied_response );

        if( settings_.target_channels[0] == 1 )
        {
          vil_math_add_image_fraction( copied_response, 1.0, vil_plane( wrapped_image, 0 ), -1.0 );
        }
        else
        {
          vil_math_add_image_fraction( copied_response, 1.0, vil_plane( wrapped_image, 1 ), -1.0 );
        }

        desired_response = copied_response;
      }
    }
    else
    {
      vil_math_image_max( vil_plane( wrapped_image, settings_.target_channels[0] ),
                          vil_plane( wrapped_image, settings_.target_channels[1] ),
                          desired_response );

      for( unsigned j = 2; j < settings_.target_channels.size(); ++j )
      {
        vil_math_image_max( desired_response,
                            vil_plane( wrapped_image, settings_.target_channels[j] ),
                            desired_response );
      }
    }

    for( unsigned j = 0; j < wrapped_image.nplanes(); ++j )
    {
      debug_output( vil_plane( wrapped_image, j ), "category", j * ( i + 1 ) );
    }

    // Scale output image so that it matches the input size, if necessary
    vil_image_view< cnn_float_t > scaled_image, final_image( ni, nj, ( averager_ ? 2 : 1 ) );
    vil_fill( final_image, -std::numeric_limits< cnn_float_t >::max() );

    if( scaling_required_ )
    {
      vil_resample_bilin( desired_response,
                          scaled_image,
                          desired_response.ni() * settings_.output_scaling_dims[0],
                          desired_response.nj() * settings_.output_scaling_dims[0] );
    }
    else
    {
      scaled_image = desired_response;
    }

    if( settings_.transpose_input )
    {
      scaled_image = vil_transpose( scaled_image );
    }

    if( cropping_required_ )
    {
      vil_image_view< cnn_float_t > output_region = point_view_to_region(
        vil_plane( final_image, ( averager_ ? 1 : 0 ) ),
        vgl_box_2d< unsigned >(
          settings_.output_scaling_dims[1], settings_.output_scaling_dims[1] + scaled_image.ni(),
          settings_.output_scaling_dims[2], settings_.output_scaling_dims[2] + scaled_image.nj() ) );

      vil_copy_reformat( scaled_image, output_region );
    }
    else
    {
      vil_plane( final_image, ( averager_ ? 1 : 0 ) ) = scaled_image;
    }

    if( averager_ )
    {
      vil_image_view< cnn_float_t > average_plane = vil_plane( final_image, 0 );
      averager_->process_frame( vil_plane( final_image, 1 ), average_plane );
    }

    debug_output( final_image, "net_map", i );


    maps.push_back( final_image );
  }
}


// Compute detection maps for multiple images.
template< class PixType >
void
cnn_detector< PixType >
::batch_detect( const std::vector< input_image_t >& images,
                std::vector< detection_mask_t >& masks )
{
  masks.clear();

  std::vector< detection_map_t > detection_maps;
  batch_detect( images, detection_maps );

  for( unsigned i = 0; i < detection_maps.size(); ++i )
  {
    detection_mask_t detection_mask;

    if( !detection_maps[i] )
    {
      masks.push_back( detection_mask );
      continue;
    }

    if( !averager_ )
    {
      vil_threshold_above( detection_maps[i],
                           detection_mask,
                           settings_.threshold );
    }
    else
    {
      vil_threshold_above( vil_plane( detection_maps[i],
                             ( settings_.mask_option == cnn_detector_settings::UNAVERAGED ? 1 : 0 ) ),
                           detection_mask,
                           settings_.threshold );
    }

    masks.push_back( detection_mask );
  }
}


// Compute detections for multiple images.
template< class PixType >
void
cnn_detector< PixType >
::batch_detect( const std::vector< input_image_t >& images,
                std::vector< detection_vec_t >& detections )
{
  std::vector< detection_map_t > maps;
  std::vector< detection_mask_t > masks;

  batch_detect( images, maps, masks, detections );
}


// Compute detections for multiple images.
template< class PixType >
void
cnn_detector< PixType >
::batch_detect( const std::vector< input_image_t >& images,
                std::vector< detection_map_t >& maps,
                std::vector< detection_mask_t >& masks,
                std::vector< detection_vec_t >& detections )
{
  masks.clear();
  detections.clear();

  masks.resize( images.size() );
  detections.resize( images.size() );

  batch_detect( images, maps );

  for( unsigned i = 0; i < maps.size(); ++i )
  {
    const detection_map_t& detection_map = maps[i];
    detection_mask_t& detection_mask = masks[i];

    if( !detection_map )
    {
      continue;
    }

    if( !averager_ )
    {
      vil_threshold_above( detection_map,
                           detection_mask,
                           settings_.threshold );
    }
    else
    {
      vil_threshold_above( vil_plane( detection_map,
                             ( settings_.mask_option == cnn_detector_settings::UNAVERAGED ? 1 : 0 ) ),
                           detection_mask,
                           settings_.threshold );
    }

    if( settings_.detection_mode == cnn_detector_settings::DISABLED )
    {
      continue;
    }
    else if( settings_.detection_mode == cnn_detector_settings::FIXED_SIZE )
    {
      gen_detections_box( detection_map, detection_mask, detections[i] );
    }
    else if( settings_.detection_mode == cnn_detector_settings::USE_BLOB )
    {
      gen_detections_blob( detection_map, detection_mask, detections[i] );
    }

    if( buffer_.size() > 1 )
    {
      if( settings_.detection_image_offset < 0 )
      {
        debug_output( buffer_.datum_at( buffer_.size() / 2 - i - 1 ),
                      "detections", 0u, detections[i] );
      }
      else
      {
        debug_output( buffer_.datum_at( settings_.detection_image_offset ),
                      "detections", 0u, detections[i] );
      }
    }
    else
    {
      debug_output( images[i], "detections", 0u, detections[i] );
    }
  }
}


template< class PixType >
void
cnn_detector< PixType >
::debug_output( const detection_map_t& map,
                const std::string& key,
                const unsigned id )
{
  if( settings_.detection_image_folder.empty() )
  {
    return;
  }

  std::string frame_id = boost::lexical_cast< std::string >( frame_counter_ );
  std::string key_id = boost::lexical_cast< std::string >( id );

  while( frame_id.size() < 5 )
  {
    frame_id = "0" + frame_id;
  }

  std::string fn = "frame_" + frame_id + "_" + key + "_" + key_id;
  fn = settings_.detection_image_folder + "/" + fn + "." + settings_.detection_image_ext;

#ifdef USE_OPENCV
  cv::Mat ocv_image, ocv_image2;
  cv::Rect ocv_region;

  detection_map_t map_to_output;

  if( map.nplanes() == 2 )
  {
    detection_map_t extended_map( map.ni(), map.nj(), 3 );
    vil_fill( extended_map, static_cast< cnn_float_t >( 0 ) );

    detection_map_t output_plane = vil_plane( extended_map, 2 );
    vil_copy_reformat( vil_plane( map, 0 ), output_plane );
    output_plane = vil_plane( extended_map, 0 );
    vil_copy_reformat( vil_plane( map, 1 ), output_plane );

    map_to_output = extended_map;
  }
  else
  {
    map_to_output = map;
  }

  deep_cv_conversion( map_to_output, ocv_image );

  double min, max;
  cv::minMaxLoc( ocv_image, &min, &max );

  ocv_image.convertTo( ocv_image2, CV_8U, 255.0 / max );
  cv::imwrite( fn, ocv_image2 );

  LOG_INFO( "Debug image: " << fn << " max: " << max << " min: " << min );
#endif
}


template< class PixType >
void
cnn_detector< PixType >
::debug_output( const input_image_t& image,
                const std::string& key,
                const unsigned id,
                const detection_vec_t detections )
{
  if( settings_.detection_image_folder.empty() )
  {
    return;
  }

  std::string frame_id = boost::lexical_cast< std::string >( frame_counter_ );
  std::string key_id = boost::lexical_cast< std::string >( id );

  while( frame_id.size() < 5 )
  {
    frame_id = "0" + frame_id;
  }

  std::string fn = "frame_" + frame_id + "_" + key + "_" + key_id;
  fn = settings_.detection_image_folder + "/" + fn + "." + settings_.detection_image_ext;

#ifdef USE_OPENCV
  cv::Mat ocv_image;
  cv::Rect ocv_region;

  input_image_t vxl_image;
  vxl_image.deep_copy( image );

  if( key == "input" || key == "detections" )
  {
    vil_math_scale_and_offset_values( vxl_image,
      settings_.detection_scaling_dims[0],
      settings_.detection_scaling_dims[1] );
  }

  deep_cv_conversion( vxl_image, ocv_image );

  for( unsigned i = 0; i < detections.size(); ++i )
  {
    convert_to_rect( detections[i].first, ocv_region );
    cv::rectangle( ocv_image, ocv_region, cv::Scalar( 0, 0, 255 ), 2 );
  }

  cv::imwrite( fn, ocv_image );
#endif
}


template< class PixType >
void
cnn_detector< PixType >
::copy_image_to_blob( const vil_image_view< PixType >& source,
                      vil_image_view< cnn_float_t >& dest )
{
  vil_image_view< PixType > formatted_source;
  vil_image_view< PixType > cnn_input;

  double scale = settings_.input_scaling_dims[0];
  double shift = settings_.input_scaling_dims[1];

  // Prefilter images as necessary
  if( settings_.squash_channels )
  {
    vil_math_mean_over_planes( source, formatted_source );
  }
  else
  {
    formatted_source = source;
  }

  if( settings_.transpose_input )
  {
    formatted_source = vil_transpose( formatted_source );
  }

  // Perform normalization
  if( settings_.norm_option == cnn_detector_settings::HIST_EQ )
  {
    vil_copy_deep( formatted_source, cnn_input );

    unsigned order = std::numeric_limits< PixType >::max();

    std::vector< unsigned > hist( order, 0 );
    std::vector< unsigned > cum_hist( order, 0 );
    std::vector< double > norm_hist( order, 0 );

    for( unsigned p = 0; p < cnn_input.nplanes(); ++p )
    {
      for( unsigned i = 0; i < cnn_input.ni(); ++i )
      {
        for( unsigned j = 0; j < cnn_input.nj(); ++j )
        {
          hist[ cnn_input( i, j, p ) ]++;
        }
      }
    }

    cum_hist[0] = hist[0];

    unsigned min_c = std::numeric_limits< unsigned >::max();
    unsigned max_c = 0;

    for( unsigned i = 1; i < hist.size(); ++i )
    {
      cum_hist[i] = hist[i] + cum_hist[i-1];

      if( cum_hist[i] != 0 )
      {
        min_c = std::min( min_c, cum_hist[i] );
        max_c = std::max( max_c, cum_hist[i] );
      }
    }

    for( unsigned i = 0; i < hist.size(); ++i )
    {
      hist[i] = ( static_cast< double >( cum_hist[i] ) - min_c ) * order / ( max_c - min_c );
    }

    for( unsigned p = 0; p < cnn_input.nplanes(); ++p )
    {
      for( unsigned i = 0; i < cnn_input.ni(); ++i )
      {
        for( unsigned j = 0; j < cnn_input.nj(); ++j )
        {
          cnn_input( i, j, p ) = hist[ cnn_input( i, j, p ) ];
        }
      }
    }
  }
  else if( settings_.norm_option == cnn_detector_settings::PER_FRAME_MEAN_STD )
  {
    cnn_input = formatted_source;

    unsigned order = std::numeric_limits< PixType >::max();

    std::vector< unsigned > hist( order, 0 );

    for( unsigned p = 0; p < formatted_source.nplanes(); ++p )
    {
      for( unsigned i = 0; i < formatted_source.ni(); ++i )
      {
        for( unsigned j = 0; j < formatted_source.nj(); ++j )
        {
          hist[ formatted_source( i, j, p ) ]++;
        }
      }
    }

    unsigned total = std::accumulate( hist.begin(), hist.end(), 0 );

    unsigned avg_sum = 0;
    unsigned avg_cnt = 0;

    for( unsigned i = 0, count = 0; i < hist.size(); ++i )
    {
      count = count + hist[i];

      if( static_cast< double >( count ) / total > 0.10 &&
          static_cast< double >( count ) / total < 0.90 )
      {
        avg_sum = avg_sum + i * hist[i];
        avg_cnt = avg_cnt + hist[i];
      }
      else
      {
        hist[i] = 0;
      }
    }

    double avg = static_cast< double >( avg_sum ) / avg_cnt;
    double dev = 0.0;

    for( unsigned i = 0; i < hist.size(); ++i )
    {
      dev = dev + hist[i] * ( ( i - avg ) * ( i - avg ) );
    }

    dev = std::sqrt( dev / avg_cnt );

    if( dev != 0.0 )
    {
      scale = scale / dev;
      shift = -1.0 * scale * avg + shift;
    }
    else
    {
      scale = 0.0;
      shift = 0.0;
    }
  }
  else if( settings_.norm_option != cnn_detector_settings::NO_NORM )
  {
    LOG_FATAL( "Invalid normalization option set" );
  }
  else
  {
    cnn_input = formatted_source;
  }

  // Copy and cast values
  if( settings_.swap_rb && dest.nplanes() == 3 )
  {
    detection_map_t output_p0 = vil_plane( dest, 0 );
    detection_map_t output_p1 = vil_plane( dest, 1 );
    detection_map_t output_p2 = vil_plane( dest, 2 );

    vil_convert_cast( vil_plane( cnn_input, 0 ), output_p2 );
    vil_convert_cast( vil_plane( cnn_input, 1 ), output_p1 );
    vil_convert_cast( vil_plane( cnn_input, 2 ), output_p0 );
  }
  else
  {
    vil_convert_cast( cnn_input, dest );
  }

  // Scale values if enabled
  if( scale != 1.0 || shift != 0.0 )
  {
    vil_math_scale_and_offset_values( dest, scale, shift );
  }
}


bool
compare_weights( const std::pair< vgl_box_2d< unsigned >, double >& p1,
                 const std::pair< vgl_box_2d< unsigned >, double >& p2 )
{
  return p1.second > p2.second;
}


template< class PixType >
void
cnn_detector< PixType >
::gen_detections_box( const detection_map_t& map,
                      const detection_mask_t& mask,
                      detection_vec_t& detections )
{
  LOG_DEBUG( "Generating detections via fixed method" );

  detection_vec_t unfiltered_detections;

  const bool pixel_nms = ( settings_.nms_option == cnn_detector_settings::PIXEL_LEVEL ||
                           settings_.nms_option == cnn_detector_settings::ALL );
  const bool box_nms = ( settings_.nms_option == cnn_detector_settings::BOX_LEVEL ||
                         settings_.nms_option == cnn_detector_settings::ALL );

  const unsigned bbox_width = settings_.bbox_side_dims[0];
  const unsigned bbox_height = settings_.bbox_side_dims[1];

  unsigned ni = mask.ni(), nj = mask.nj(), np = mask.nplanes();
  std::ptrdiff_t sistep = mask.istep(), sjstep = mask.jstep(), spstep = mask.planestep();

  const bool *splane = mask.top_left_ptr();
  for( unsigned p = 0; p < np; ++p, splane += spstep )
  {
    const bool *srow = splane;
    for( unsigned j = 1; j < nj-1; ++j, srow += sjstep )
    {
      const bool *spixel = srow;
      for( unsigned i = 1; i < ni-1; ++i, spixel+=sistep )
      {
        if( *spixel )
        {
          const cnn_float_t mag = map( i, j, p );

          if( pixel_nms && ( map( i-1, j, p ) > mag || map( i+1, j, p ) > mag ||
                             map( i, j-1, p ) > mag || map( i, j+1, p ) > mag ) )
          {
            continue;
          }

          unfiltered_detections.push_back(
            std::make_pair(
              vgl_box_2d< unsigned >(
                ( i > bbox_width ? i - bbox_width : 0 ), i + bbox_width,
                ( j > bbox_height ? j - bbox_height : 0 ), j + bbox_height ),
              mag ) );
        }
      }
    }
  }

  if( box_nms )
  {
    const double percent_threshold = settings_.bbox_nms_crit;

    LOG_DEBUG( "Sorting detections for NMS suppression" );

    sort( unfiltered_detections.begin(), unfiltered_detections.end(), compare_weights );

    // TODO: Replace with kd-tree for efficiency
    LOG_DEBUG( "Performing NMS" );

    for( unsigned i = 0; i < unfiltered_detections.size(); ++i )
    {
      const vgl_box_2d< unsigned >& test_rect = unfiltered_detections[i].first;

      bool found_overlap = false;

      for( unsigned j = 0; j < detections.size(); ++j )
      {
        const vgl_box_2d< unsigned >& test2_rect = detections[j].first;
        const vgl_box_2d< unsigned > box_intersect = vgl_intersection( test_rect, test2_rect );

        double test_area = static_cast< double >( test_rect.area() );
        double test2_area = static_cast< double >( test2_rect.area() );
        double intersect_area = static_cast< double >( box_intersect.area() );

        if( ( test_area != 0.0 && intersect_area / test_area > percent_threshold ) ||
            ( test2_area != 0.0 && intersect_area / test2_area > percent_threshold ) )
        {
          found_overlap = true;
          break;
        }
      }

      if( !found_overlap )
      {
        detections.push_back( unfiltered_detections[i] );
      }
    }
  }
  else
  {
    detections = unfiltered_detections;
  }
}


template< class PixType >
void
cnn_detector< PixType >
::gen_detections_blob( const detection_map_t& map,
                       const detection_mask_t& mask,
                       detection_vec_t& detections )
{
  vil_image_view< unsigned > labels_whole;

  std::vector< vil_blob_region > blobs;
  std::vector< vil_blob_region >::iterator it_blobs;

  vil_blob_labels( mask, vil_blob_8_conn, labels_whole );
  vil_blob_labels_to_regions( labels_whole, blobs );

  // Fill in information about each blob
  for( it_blobs = blobs.begin(); it_blobs < blobs.end(); ++it_blobs )
  {
    vgl_box_2d< unsigned > bbox;
    vil_blob_region::iterator it_chords;

    for( it_chords = it_blobs->begin(); it_chords < it_blobs->end(); ++it_chords )
    {
      bbox.add( image_object::image_point_type( it_chords->ilo, it_chords->j ) );
      bbox.add( image_object::image_point_type( it_chords->ihi, it_chords->j ) );
    }

    bbox.set_max_x( bbox.max_x() + 1 );
    bbox.set_max_y( bbox.max_y() + 1 );

    detections.push_back(
      std::make_pair(
        bbox,
        map( bbox.centroid_x(), bbox.centroid_y() ) ) );
  }
}


} // end namespace vidtk
