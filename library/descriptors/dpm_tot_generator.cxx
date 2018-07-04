/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "dpm_tot_generator.h"

#include <vil/vil_resample_bilin.h>

#include <vgl/vgl_box_2d.h>
#include <vnl/vnl_vector.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <tracking_data/track_state.h>

#include <utilities/vxl_to_cv_converters.h>

#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numeric>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "dpm_tot_generator_cxx" );


// Feature count in the raw descriptor variant
const unsigned raw_feature_count = dpm_tot_generator::TOTAL_FEATURE_COUNT;

// Feature that begins the classifier portion of the descriptor
const unsigned first_classifier_bin = dpm_tot_generator::AVG_PERSON_SCORE;

// Minimum scaled image dimension for successful image scaling
const unsigned min_dim_for_resize = 4;

// Minimum scaled image dimension to run DPM on
const int min_dim_for_dpm = 41;

// Optional variables for writing out features
#ifdef DPM_TOT_FEATURE_WRITER_ENABLED
  boost::mutex writer_mutex;
  std::ofstream ofile_p;
  std::ofstream ofile_v;
#endif


// Any data we want stored for each individual track we encounter
struct dpm_track_data : track_data_storage
{
  // Internal frame counter
  unsigned frames_since_last_compute;

  // Internal frame counter
  unsigned frames_since_last_output;

  // Last timestamp observed for the last track state
  timestamp last_state_ts;

  // The identifier of the last frame we processed
  timestamp last_processed_ts;

  // Pointer to the aggregate version of the raw descriptor
  descriptor_sptr current_desc;

  // Vector of person detection scores
  std::vector< std::pair< double, double > > person_scores;

  // Vector of vehicle detection scores
  std::vector< std::pair< double, double > > vehicle_scores;

  // Records the number of frames with person detections
  unsigned person_counter;

  // Records the number of frames with vehicle detections
  unsigned vehicle_counter;

  // Constructor
  explicit dpm_track_data( const std::string& desc_id,
                           const track_sptr& trk,
                           const double default_value )
   : frames_since_last_compute( std::numeric_limits< unsigned >::max()-1 ),
     frames_since_last_output( std::numeric_limits< unsigned >::max()-1 ),
     person_counter( 0 ),
     vehicle_counter( 0 )
  {
    current_desc = create_descriptor( desc_id, trk->id() );
    current_desc->resize_features( raw_feature_count, 0.0 );

    // Seed classifier bins with most negative double
    for( unsigned i = first_classifier_bin; i < raw_feature_count; i++ )
    {
      current_desc->at( i ) = default_value;
    }
  }
};


dpm_tot_generator
::dpm_tot_generator()
{
}


dpm_tot_generator
::~dpm_tot_generator()
{
#ifdef DPM_TOT_FEATURE_WRITER_ENABLED
  if( ofile_p.is_open() )
  {
    ofile_p.close();
  }
  if( ofile_v.is_open() )
  {
    ofile_v.close();
  }
#endif
}


bool
dpm_tot_generator
::configure( const external_settings& settings )
{
  // Confirm correct settings type and copy options
  dpm_tot_settings const* desc_settings =
    dynamic_cast<dpm_tot_settings const*>( &settings );

  if( !desc_settings )
  {
    return false;
  }

  settings_ = *desc_settings;

  // Set internal generator runtime options
  generator_settings internal_options;

  // Here we divide by two, because if thread_count >= 2 we will use
  // 2 threads for each track (one for each classifier) as opposed to
  // the usual single thread.
  internal_options.thread_count = std::max( settings_.thread_count / 2, 1u );
  internal_options.sampling_rate = 1;
  this->set_operating_mode( internal_options );

  // Load classifier models
  person_detector_.reset( new cv::LatentSvmDetector() );
  vehicle_detector_.reset( new cv::LatentSvmDetector() );

  std::vector< std::string > person_filelist( 1, settings_.person_model_filename );
  std::vector< std::string > vehicle_filelist( 1, settings_.vehicle_model_filename );

  if( !person_detector_->load( person_filelist ) )
  {
    LOG_ERROR( "Failed to open person model: " << settings_.person_model_filename );
    return false;
  }
  if( !settings_.no_vehicles && !vehicle_detector_->load( vehicle_filelist ) )
  {
    LOG_ERROR( "Failed to open vehicle model: " << settings_.vehicle_model_filename );
    return false;
  }

#ifdef DPM_TOT_FEATURE_WRITER_ENABLED
  // Debug score writer
  ofile_p.open( "person_detection_scores", std::ofstream::out );
  ofile_v.open( "vehicle_detection_scores", std::ofstream::out );
#endif

  return true;
}


external_settings*
dpm_tot_generator::
create_settings()
{
  return new dpm_tot_settings;
}


track_data_sptr
dpm_tot_generator
::new_track_routine( track_sptr const& new_track )
{
  // Create new track data for this track
  return track_data_sptr( new dpm_track_data( settings_.descriptor_id,
                                              new_track,
                                              settings_.lowest_detector_score ) );
}


void
adjust_tot_values( double p_feature, double v_feature, descriptor_data_t& output, double weight )
{
  if( p_feature > v_feature )
  {
    output[ dpm_tot_generator::PERSON_PROBABILITY_BIN ] += weight;
  }
  else if( v_feature > p_feature )
  {
    output[ dpm_tot_generator::VEHICLE_PROBABILITY_BIN ] += weight;
  }
  else
  {
    output[ dpm_tot_generator::OTHER_PROBABILITY_BIN ] += weight;
  }
}


double
classifier_ratio( double score, double min_score = -1.0, double theta = 0.70 )
{
  if( score < min_score )
  {
    return 0.0;
  }

  const double score_cdf = 1.0 - std::exp( -1.0 * theta * ( score - min_score ) );

  return std::max( std::min( score_cdf, 1.0 ), 0.0 );
}


double
detection_ratio( double detection_count, double total_count )
{
  if( total_count >= 1.0 )
  {
    return std::max( std::min( 1.0, detection_count / total_count ), 0.0 );
  }

  return 0.0;
}


void
make_category_highest( descriptor_data_t& vec, unsigned category )
{
  descriptor_data_t sorted_vec = vec;
  std::sort( sorted_vec.begin(), sorted_vec.end() );

  if( vec[ category ] < sorted_vec.back() )
  {
    vec[ category ] = sorted_vec.back() + 0.01;
  }
  else if( vec[ category ] == sorted_vec[ sorted_vec.size()-2 ] )
  {
    vec[ category ] = sorted_vec[ category ] + 0.01;
  }
}


void
normalize_descriptor( descriptor_data_t& tot_values )
{
  double sum = std::accumulate( tot_values.begin(), tot_values.end(), 0.0 );

  if( sum != 0 )
  {
    tot_values[ 0 ] = tot_values[ 0 ] / sum;
    tot_values[ 1 ] = tot_values[ 1 ] / sum;
    tot_values[ 2 ] = tot_values[ 2 ] / sum;
  }
}


bool
compare_scores( const std::pair< double, double >& p1,
                const std::pair< double, double >& p2 )
{
  return p1.first < p2.first;
}


void
compute_averages( const std::vector< std::pair< double, double > >& scores,
                  const double threshold,
                  double& avg1, double& avg2, const double backup = -1.0 )
{
  unsigned counter = 0;

  avg1 = 0;
  avg2 = 0;

  for( unsigned i = 0; i < scores.size(); ++i )
  {
    if( scores[i].first >= threshold )
    {
      counter++;

      avg1 += scores[i].first;
      avg2 += scores[i].second;
    }
  }

  if( counter != 0 )
  {
    avg1 = avg1 / counter;
    avg2 = avg2 / counter;
  }
  else
  {
    avg1 = backup;
    avg2 = backup;
  }
}


bool
dpm_tot_generator
::track_update_routine( track_sptr const& active_track,
                        track_data_sptr track_data )
{
  // Scale track region
  dpm_track_data* data = dynamic_cast<dpm_track_data*>( track_data.get() );

  // Increase observed frames counts and handle output sample rate if enabled
  bool output_desc = true;
  bool compute_desc = true;

  if( settings_.output_sampling_rate != 0 &&
      data->frames_since_last_output % settings_.output_sampling_rate != 0 )
  {
    output_desc = false;
  }

  data->frames_since_last_compute++;
  data->frames_since_last_output++;

  // Decide if we only want to process frames with new bboxes
  const timestamp& current_ts = active_track->last_state()->time_;

  if( settings_.new_states_w_bbox_only )
  {
    vgl_box_2d<unsigned> tmp_bbox;

    if( !active_track->last_state()->bbox( tmp_bbox ) ||
        ( data->last_state_ts.has_time() &&
          data->last_state_ts.time() == current_ts.time() ) )
    {
      compute_desc = false;
    }
    else
    {
      data->last_state_ts = current_ts;
    }
  }

  // Add history reference to descriptor
  descriptor_history_entry region = generate_history_entry( current_frame(), active_track );
  data->current_desc->add_history_entry( region );

  // Compute time since last processed frame
  double time_since_last = 0.0;

  if( data->last_processed_ts.has_time() && current_ts.has_time() )
  {
    time_since_last = current_ts.time_in_secs() - data->last_processed_ts.time_in_secs();
  }

  // Account for downsampling parameters
  if( time_since_last <= settings_.time_difference_trigger &&
      data->frames_since_last_compute < settings_.compute_sampling_rate )
  {
    compute_desc = false;
  }

  // Declare variables shared amongst all remaining operations
  descriptor_data_t& raw_features = data->current_desc->get_features();
  ocv_image_t track_region;
  detection_list_t vehicle_detections;
  detection_list_t person_detections;
  double target_area_pixels;

  // Compute raw descriptor if required and possible
  if( compute_desc )
  {
    // Mark this frame as succesfully processed
    data->frames_since_last_compute = 0;
    data->last_processed_ts = current_ts;

    // Expand BB to desired size and run detection algorithm
    if( compute_track_snippet( current_frame(),
                               region.get_image_location(),
                               track_region,
                               target_area_pixels ) )
    {
      // Handle threading option
      if( settings_.thread_count > 1 && !settings_.no_vehicles )
      {
        ocv_image_t copied_track_region;
        track_region.copyTo( copied_track_region );

        boost::thread aux_thread(
          boost::bind(
            &dpm_tot_generator::run_detector,
            this,
            copied_track_region,
            boost::ref( *person_detector_ ),
            boost::ref( person_detections ) ) );

        run_detector( track_region,
                      *vehicle_detector_,
                      vehicle_detections );

        aux_thread.join();
      }
      else
      {
        run_detector( track_region,
                      *person_detector_,
                      person_detections );

        if( !settings_.no_vehicles )
        {
          run_detector( track_region,
                        *vehicle_detector_,
                        vehicle_detections );
        }
      }
    }

    // Update raw descriptor values
    for( unsigned i = 0; i < person_detections.size(); ++i )
    {
      data->person_scores.push_back(
        std::pair< double, double >(
          person_detections[ i ].score,
          static_cast< double >( person_detections[ i ].rect.area() ) / target_area_pixels ) );

      if( person_detections[ i ].score > raw_features[ MAX_PERSON_SCORE ] )
      {
        raw_features[ MAX_PERSON_SCORE ] = person_detections[ i ].score;
      }
    }
    for( unsigned i = 0; i < vehicle_detections.size(); ++i )
    {
      data->vehicle_scores.push_back(
        std::pair< double, double >(
          vehicle_detections[ i ].score,
          static_cast< double >( vehicle_detections[ i ].rect.area() ) / target_area_pixels ) );

      if( vehicle_detections[ i ].score > raw_features[ MAX_VEHICLE_SCORE ] )
      {
        raw_features[ MAX_VEHICLE_SCORE ] = vehicle_detections[ i ].score;
      }
    }

    raw_features[ TOTAL_RUN_COUNT ] = raw_features[ TOTAL_RUN_COUNT ] + 1;
    raw_features[ POS_PERSON_DET ] = data->person_scores.size();
    raw_features[ POS_VEHICLE_DET ] = data->vehicle_scores.size();

    if( !person_detections.empty() )
    {
      data->person_counter++;

      double score_avg, bbox_avg;

      compute_averages( data->person_scores,
                        settings_.lowest_detector_score,
                        score_avg,
                        bbox_avg,
                        settings_.lowest_detector_score );

      raw_features[ AVG_PERSON_SCORE ] = score_avg;
      raw_features[ PERSON_BBOX_SCORE ] = bbox_avg;

      compute_averages( data->person_scores,
                        settings_.raw_threshold_1,
                        score_avg,
                        bbox_avg,
                        settings_.lowest_detector_score );

      raw_features[ PERSON_THRESH1_SCORE ] = score_avg;
      raw_features[ PERSON_THRESH1_BBOX ] = bbox_avg;

      compute_averages( data->person_scores,
                        settings_.raw_threshold_2,
                        score_avg,
                        bbox_avg,
                        settings_.lowest_detector_score );

      raw_features[ PERSON_THRESH2_SCORE ] = score_avg;
      raw_features[ PERSON_THRESH2_BBOX ] = bbox_avg;

      std::sort( data->person_scores.begin(), data->person_scores.end(), compare_scores );

      unsigned index_75th = static_cast< unsigned >( 0.75 * data->person_scores.size() );
      raw_features[ PERSON_75TH_SCORE ] = data->person_scores[ index_75th ].first;
    }

    if( !vehicle_detections.empty() )
    {
      data->vehicle_counter++;

      double score_avg, bbox_avg;

      compute_averages( data->vehicle_scores,
                        settings_.lowest_detector_score,
                        score_avg,
                        bbox_avg,
                        settings_.lowest_detector_score );

      raw_features[ AVG_VEHICLE_SCORE ] = score_avg;
      raw_features[ VEHICLE_BBOX_SCORE ] = bbox_avg;

      compute_averages( data->vehicle_scores,
                        settings_.raw_threshold_1,
                        score_avg,
                        bbox_avg,
                        settings_.lowest_detector_score );

      raw_features[ VEHICLE_THRESH1_SCORE ] = score_avg;
      raw_features[ VEHICLE_THRESH1_BBOX ] = bbox_avg;

      compute_averages( data->vehicle_scores,
                        settings_.raw_threshold_2,
                        score_avg,
                        bbox_avg,
                        settings_.lowest_detector_score );

      raw_features[ VEHICLE_THRESH2_SCORE ] = score_avg;
      raw_features[ VEHICLE_THRESH2_BBOX ] = bbox_avg;

      std::sort( data->vehicle_scores.begin(), data->vehicle_scores.end(), compare_scores );

      unsigned index_75th = static_cast< unsigned >( 0.75 * data->vehicle_scores.size() );
      raw_features[ VEHICLE_75TH_SCORE ] = data->vehicle_scores[ index_75th ].first;
    }

    raw_features[ POS_PERSON_PER ] =
      static_cast< double >( data->person_counter ) / raw_features[ TOTAL_RUN_COUNT ];
    raw_features[ POS_VEHICLE_PER ] =
      static_cast< double >( data->vehicle_counter ) / raw_features[ TOTAL_RUN_COUNT ];

    raw_features[ PERSON_RATIO_SCORE ] =
      ( raw_features[ POS_PERSON_PER ] > raw_features[ POS_VEHICLE_PER ] ?
        raw_features[ POS_PERSON_PER ] - raw_features[ POS_VEHICLE_PER ] : 0.0 );
    raw_features[ VEHICLE_RATIO_SCORE ] =
      ( raw_features[ POS_VEHICLE_PER ] > raw_features[ POS_PERSON_PER ] ?
        raw_features[ POS_VEHICLE_PER ] - raw_features[ POS_PERSON_PER ] : 0.0 );


  }

  // Optionally output raw descriptor
  if( settings_.output_mode == dpm_tot_settings::AGGREGATE_DESCRIPTOR && output_desc )
  {
    this->add_outgoing_descriptor( new raw_descriptor( *data->current_desc ) );
  }

  // Optionally output estimated TOT classification
  if( settings_.output_mode == dpm_tot_settings::TOT_VALUES && output_desc )
  {
    // Generate TOT values
    descriptor_sptr to_output( new raw_descriptor( *data->current_desc ) );

    descriptor_data_t& tot_values = to_output->get_features();
    tot_values.resize( TOTAL_TOT_BINS );
    std::fill( tot_values.begin(), tot_values.end(), 0.0 );

    std::vector< double > strong_person_scores;
    std::vector< double > strong_vehicle_scores;

    // First compute a few properties pretaining to all detections above our
    // detector threshold config options
    double strong_person_average = 0.0;
    double strong_vehicle_average = 0.0;

    for( unsigned i = 0; i < data->person_scores.size(); ++i )
    {
      if( data->person_scores[i].first >= settings_.person_classifier_threshold )
      {
        strong_person_scores.push_back( data->person_scores[i].first );
        strong_person_average += data->person_scores[i].first;
      }
    }
    for( unsigned i = 0; i < data->vehicle_scores.size(); ++i )
    {
      if( data->vehicle_scores[i].first >= settings_.vehicle_classifier_threshold )
      {
        strong_vehicle_scores.push_back( data->vehicle_scores[i].first );
        strong_vehicle_average += data->vehicle_scores[i].first;
      }
    }

    if( strong_person_scores.empty() )
    {
      strong_person_average = -std::numeric_limits< double >::max();
    }
    else
    {
      strong_person_average = strong_person_average / strong_person_scores.size();
    }

    if( strong_vehicle_scores.empty() )
    {
      strong_vehicle_average = -std::numeric_limits< double >::max();
    }
    else
    {
      strong_vehicle_average = strong_vehicle_average / strong_vehicle_scores.size();
    }

    // Decide which category we want to be the highest in our TOT output. Note: there
    // is currently a few magic numbers in the below code and the TOT probabilities
    // being produced by this descriptor are relatively arbitrary. In the future more
    // effort may be put into generating better probabilities (i.e., by either training
    // classifiers on top of raw descriptors, or binning scores into histograms...) as
    // has been done with prior PVO/TOT descriptors. Right now they are only produced
    // via a few heuristics.
    unsigned top_category = OTHER_PROBABILITY_BIN;

    if( !strong_vehicle_scores.empty() || !strong_person_scores.empty() )
    {
      adjust_tot_values( strong_person_scores.size(),
                         strong_vehicle_scores.size(),
                         tot_values, 0.10 );

      adjust_tot_values( strong_person_average,
                         strong_vehicle_average,
                         tot_values, 0.10 );

      adjust_tot_values( raw_features[ MAX_PERSON_SCORE ],
                         raw_features[ MAX_VEHICLE_SCORE ],
                         tot_values, 0.05 );

      if( tot_values[ PERSON_PROBABILITY_BIN ] > tot_values[ VEHICLE_PROBABILITY_BIN ] )
      {
        top_category = PERSON_PROBABILITY_BIN;
      }
      else
      {
        top_category = VEHICLE_PROBABILITY_BIN;
      }
    }

    const double total_frames = raw_features[ TOTAL_RUN_COUNT ];
    const double min_det_score = settings_.lowest_detector_score;
    const double normalization_bias = 0.05;
    const double no_vehicle_cancellation_factor = 0.01;

    // Assign probabilities
    double person_probability = classifier_ratio( raw_features[ AVG_PERSON_SCORE ], min_det_score ) *
                                classifier_ratio( raw_features[ MAX_PERSON_SCORE ], min_det_score ) *
                                detection_ratio( raw_features[ POS_PERSON_DET ], total_frames );

    double vehicle_probability = classifier_ratio( raw_features[ AVG_VEHICLE_SCORE ], min_det_score ) *
                                 classifier_ratio( raw_features[ MAX_VEHICLE_SCORE ], min_det_score ) *
                                 detection_ratio( raw_features[ POS_VEHICLE_DET ], total_frames );

    double other_weight = 1.0 - detection_ratio( raw_features[ POS_PERSON_DET ] +
                                                 raw_features[ POS_VEHICLE_DET ],
                                                 2 * total_frames );

    double max_probability = std::max( person_probability, vehicle_probability );

    tot_values[ PERSON_PROBABILITY_BIN ] = person_probability;
    tot_values[ VEHICLE_PROBABILITY_BIN ] = vehicle_probability;

    if( top_category == OTHER_PROBABILITY_BIN && max_probability > 0.0 )
    {
      tot_values[ PERSON_PROBABILITY_BIN ] *= ( 1.0 - other_weight );
      tot_values[ VEHICLE_PROBABILITY_BIN ] *= ( 1.0 - other_weight );
      tot_values[ OTHER_PROBABILITY_BIN ] = max_probability * ( 1.0 + other_weight );
    }
    else if( top_category == OTHER_PROBABILITY_BIN && max_probability <= 0.0 )
    {
      other_weight = ( detection_ratio( total_frames, 4 ) );

      tot_values[ PERSON_PROBABILITY_BIN ] = 0.33 * ( 1.0 - other_weight );
      tot_values[ VEHICLE_PROBABILITY_BIN ] = 0.33 * ( 1.0 - other_weight );
      tot_values[ OTHER_PROBABILITY_BIN ] = 0.33 + 0.67 * other_weight;
    }
    else if( max_probability > 0.0 )
    {
      tot_values[ OTHER_PROBABILITY_BIN ] = ( max_probability * other_weight ) / 2.0;
    }
    else
    {
      tot_values[ OTHER_PROBABILITY_BIN ] = 1.0;
    }

    // Normalize descriptor and confirm desired category is set
    normalize_descriptor( tot_values );

    tot_values[ PERSON_PROBABILITY_BIN ] += normalization_bias;
    tot_values[ VEHICLE_PROBABILITY_BIN ] += normalization_bias;
    tot_values[ OTHER_PROBABILITY_BIN ] += normalization_bias;

    make_category_highest( tot_values, top_category );

    // Nuke vehicle score if option set
    if( settings_.no_vehicles )
    {
      tot_values[ 1 ] *= no_vehicle_cancellation_factor;
    }

    normalize_descriptor( tot_values );

    // Validate output
    if( tot_values[ 0 ] != tot_values[ 0 ] ||
        tot_values[ 1 ] != tot_values[ 1 ] ||
        tot_values[ 2 ] != tot_values[ 2 ] )
    {
      tot_values[ 0 ] = 0.33;
      tot_values[ 1 ] = 0.33;
      tot_values[ 2 ] = 0.34;
    }

    to_output->set_pvo_flag( true );

    // Output descriptor
    this->add_outgoing_descriptor( to_output );
  }

  // Optionally output any detection images to folder
  if( !settings_.output_detection_folder.empty() && compute_desc )
  {
    static unsigned dpm_tot_detection_counter = 1;

    std::string fn = settings_.output_detection_folder + "/detection" +
      boost::lexical_cast< std::string >( dpm_tot_detection_counter ) + ".png";

    dpm_tot_detection_counter++;

    ocv_image_t copied_image;

    if( track_region.channels() == 3 )
    {
      track_region.copyTo( copied_image );
    }
    else
    {
      cv::cvtColor( track_region, copied_image, CV_GRAY2BGR );
    }

    for( unsigned i = 0; i < person_detections.size(); ++i )
    {
      cv::rectangle( copied_image, person_detections[ i ].rect, cv::Scalar( 255, 0, 255 ), 2 );
    }
    for( unsigned i = 0; i < vehicle_detections.size(); ++i )
    {
      cv::rectangle( copied_image, vehicle_detections[ i ].rect, cv::Scalar( 0, 0, 255 ), 2 );
    }

    cv::imwrite( fn, copied_image );
  }

  return true;
}


bool
dpm_tot_generator
::terminated_track_routine( track_sptr const& finished_track,
                            track_data_sptr track_data )
{
  bool output = track_update_routine( finished_track, track_data );

#ifdef DPM_TOT_FEATURE_WRITER_ENABLED
  dpm_track_data* data = dynamic_cast<dpm_track_data*>( track_data.get() );

  boost::unique_lock< boost::mutex > lock( writer_mutex );

  ofile_p << finished_track->id() << " " << data->current_desc->at( 0 ) << " ";
  for( unsigned i = 0; i < data->person_scores.size(); i++ )
  {
    ofile_p << data->person_scores[i] << " ";
  }
  ofile_p << std::endl;

  ofile_v << finished_track->id() << " " << data->current_desc->at( 0 ) << " ";
  for( unsigned i = 0; i < data->vehicle_scores.size(); i++ )
  {
    ofile_v << data->vehicle_scores[i] << " ";
  }
  ofile_v << std::endl;
#endif

  return output;
}


bool
dpm_tot_generator
::compute_track_snippet( const frame_data_sptr input_frame,
                         const bbox_t& region,
                         ocv_image_t& output,
                         double& new_target_area )
{
  // Validate input area
  if( region.volume() <= settings_.required_min_area )
  {
    return false;
  }

  // Expand bbox size, union with image bounds
  image_region new_region = region;
  scale_region( new_region, settings_.bb_scale_factor, settings_.bb_pixel_shift );

  // Create new single channel image
  vxl_image_t img_region = point_view_to_region(
    ( settings_.process_grayscale ? input_frame->image_8u_gray() : input_frame->image_8u() ),
    new_region );

  // Validate extracted region size
  if( img_region.ni() < min_dim_for_resize || img_region.nj() < min_dim_for_resize )
  {
    return false;
  }

  if( settings_.enable_adaptive_resize )
  {
    // Validate input region
    vxl_image_t resized;

    double region_area = static_cast<double>( img_region.ni() * img_region.nj() );

    if( region_area == 0.0 )
    {
      return false;
    }

    const double desired_min_area = settings_.desired_min_area;
    const double desired_max_area = settings_.desired_max_area;

    double resize_factor = 1.0;

    if( region_area < desired_min_area )
    {
      resize_factor = std::sqrt( desired_min_area / region_area );
    }
    else if( region_area > desired_max_area )
    {
      resize_factor = std::sqrt( desired_max_area / region_area );
    }

    if( resize_factor > settings_.max_upscale_factor )
    {
      resize_factor = settings_.max_upscale_factor;
    }

    // Calculate and validate resizing parameters
    unsigned new_width = static_cast<unsigned>( resize_factor * img_region.ni() );
    unsigned new_height = static_cast<unsigned>( resize_factor * img_region.nj() );

    if( new_width < min_dim_for_resize || new_height < min_dim_for_resize )
    {
      return false;
    }

    // Resample image
    new_target_area = static_cast<double>( resize_factor * resize_factor * region.volume() );
    vil_resample_bilin( img_region, resized, new_width, new_height );
    deep_cv_conversion( resized, output );
  }
  else
  {
    deep_cv_conversion( img_region, output );
  }

  // Verify desired channel count
  if( !settings_.process_grayscale && output.channels() == 1 )
  {
    ocv_image_t upsampled;
    cv::cvtColor( output, upsampled, CV_GRAY2BGR );
    output = upsampled;
  }

  return true;
}


bool
dpm_tot_generator
::run_detector( const ocv_image_t& image,
                dpm_detector_t& detector,
                detection_list_t& output )
{
  if( image.rows < min_dim_for_dpm || image.cols < min_dim_for_dpm )
  {
    return true;
  }

  try
  {
    detector.detect( image, output, settings_.overlap_threshold );
  }
  catch( const cv::Exception& e )
  {
    LOG_ERROR( "Caught exception while running OpenCV DPM detector: " << e.msg );
    return false;
  }

  return true;
}


} // end namespace vidtk
