/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pvohog_generator_h_
#define vidtk_pvohog_generator_h_

#include <descriptors/online_descriptor_generator.h>

#include <utilities/config_block.h>

#include <vnl/vnl_int_2.h>

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

namespace vidtk
{

#define PVOHOG_HIST_SIZE 10

#define pvohog_desc_settings( add_param ) \
  add_param( nframes, \
             int, \
             1, \
             "TODO" ); \
  add_param( width, \
             int, \
             0, \
             "TODO" ); \
  add_param( height, \
             int, \
             0, \
             "TODO" ); \
  add_param( winSize, \
             vnl_int_2, \
             vnl_int_2(64, 64), \
             "TODO" ); \
  add_param( winStride, \
             vnl_int_2, \
             vnl_int_2(8, 8), \
             "TODO" ); \
  add_param( blockSize, \
             vnl_int_2, \
             vnl_int_2(16, 16), \
             "TODO" ); \
  add_param( blockStride, \
             vnl_int_2, \
             vnl_int_2(8, 8), \
             "TODO" ); \
  add_param( cellSize, \
             vnl_int_2, \
             vnl_int_2(8, 8), \
             "TODO" ); \
  add_param( padding, \
             vnl_int_2, \
             vnl_int_2(8, 8), \
             "TODO" ); \
  add_param( margin, \
             vnl_int_2, \
             vnl_int_2(4, 4), \
             "TODO" ); \
  add_param( nbins, \
             int, \
             9, \
             "TODO" ); \
  add_param( derivAperture, \
             int, \
             1, \
             "TODO" ); \
  add_param( winSigma, \
             double, \
             -1, \
             "TODO" ); \
  add_param( histogramNormType, \
             int, \
             0, \
             "TODO" ); \
  add_param( L2HysThreshold, \
             double, \
             0.2, \
             "TODO" ); \
  add_param( gammaCorrection, \
             bool, \
             false, \
             "TODO" );

#define add_histogram( name, add_param, add_array ) \
  add_param( name##_min, \
             double, \
             0., \
             "Minimum score mapped by the histogram" ); \
  add_param( name##_max, \
             double, \
             0., \
             "Maximum score mapped by the histogram" ); \
  add_array( name##_p, \
             double, \
             PVOHOG_HIST_SIZE, \
             "0 0 0 0 0 0 0 0 0 0", \
             "The histogram of person instances given a raw score within " \
             "a bucket (normalized internally)." ); \
  add_array( name##_v, \
             double, \
             PVOHOG_HIST_SIZE, \
             "0 0 0 0 0 0 0 0 0 0", \
             "The histogram of vehicle instances given a raw score within " \
             "a bucket (normalized internally)." ); \
  add_array( name##_o, \
             double, \
             PVOHOG_HIST_SIZE, \
             "0 0 0 0 0 0 0 0 0 0", \
             "The histogram of other instances given a raw score within " \
             "a bucket (normalized internally)." )

#define add_histogram_params( add_param, add_array ) \
  add_histogram( histogram_conversion_r0, add_param, add_array ) \
  add_histogram( histogram_conversion_r1, add_param, add_array ) \
  add_histogram( histogram_conversion_r2, add_param, add_array )

#define pvohog_gen_settings( add_param, add_array ) \
  add_param( descriptor_name, \
             std::string, \
             "pvohog", \
             "Raw descriptor ID to attach to outgoing descriptors." ); \
  add_param( config_path, \
             std::string, \
             "", \
             "Path to the configuration file." ); \
  add_param( raw_classifier_output_mode, \
             bool, \
             false, \
             "As opposed to outputting PVO probabilities, should a raw " \
             "descriptor be computed which outputs raw classifier outputs " \
             "and the max and min raw classifier outputs over time?" ); \
  add_param( thread_count, \
             unsigned, \
             1, \
             "Number of threads to use across all tracks." ); \
  add_param( samples, \
             unsigned, \
             0, \
             "Number of samples to required to create a score." ); \
  add_param( bbox_size, \
             unsigned, \
             64, \
             "Size of the bounding box for sthog and spatial searches." ); \
  add_param( bbox_border, \
             unsigned, \
             16, \
             "Size of the border around image chips for context." ); \
  add_param( skip_frames, \
             unsigned, \
             0, \
             "The number of frames to skip at the start of a track." ); \
  add_param( sampling_rate, \
             unsigned, \
             1, \
             "Only use every 1 in this many frames while computing " \
             "descriptors." ); \
  add_param( model_file, \
             std::string, \
             "", \
             "Path to the combined model file." ); \
  add_param( model_file_vehicle, \
             std::string, \
             "", \
             "Path to the vehicle/other model file." ); \
  add_param( model_file_people, \
             std::string, \
             "", \
             "Path to the person/other model file." ); \
  add_param( use_linear_svm, \
             bool, \
             false, \
             "Whether the models are linear SVM or not." ); \
  add_param( gsd_threshold, \
             double, \
             0.19, \
             "The GSD threshold for determining near/far field-of-view." ); \
  add_param( gsd_scale_vehicle, \
             double, \
             0.2, \
             "TODO" ); \
  add_param( gsd_scale_people, \
             double, \
             0.05, \
             "TODO" ); \
  add_param( backup_gsd, \
             double, \
             0.30, \
             "Backup GSD estimate if one is not available." ); \
  add_param( dt_threshold_vehicle, \
             double, \
             0.0, \
             "TODO" ); \
  add_param( dt_threshold_people, \
             double, \
             0.0, \
             "TODO" ); \
  add_param( score_sigma_vehicle, \
             double, \
             0, \
             "TODO" ); \
  add_param( score_sigma_people, \
             double, \
             0, \
             "TODO" ); \
  add_param( frame_start, \
             unsigned, \
             20, \
             "The first frame to use for temporal filtering on results." ); \
  add_param( frame_end, \
             unsigned, \
             20, \
             "The last frame to use for temporal filtering on results " \
             "(disabled if >= frame_start)." ); \
  add_param( frame_step, \
             unsigned, \
             2, \
             "Use every frame_step'th frame when temporally filtering " \
             "results." ); \
  add_param( use_rotation_chips, \
             bool, \
             false, \
             "If true, rotate vehicle chips to normalize direction." ); \
  add_param( use_spatial_search, \
             bool, \
             false, \
             "TODO" ); \
  add_param( dbg_write_outputs, \
             bool, \
             false, \
             "TODO" ); \
  add_param( dbg_output_path, \
             std::string, \
             "", \
             "TODO" ); \
  add_param( dbg_filename_fmt, \
             std::string, \
             "", \
             "TODO" ); \
  add_param( video_basename, \
             std::string, \
             "", \
             "TODO" ); \
  add_param( dbg_write_intermediates, \
             int, \
             0, \
             "DEPRECATED: Logging is now used. Kept here for VIRAT's " \
             "configuration files." ); \
  add_param( use_histogram_conversion, \
             bool, \
             false, \
             "If true, convert the raw scores into PVO scores using " \
             "histograms." ); \
  add_histogram_params( add_param, add_array ) \
  pvohog_desc_settings( add_param )

init_external_settings2( pvohog_settings, pvohog_gen_settings )

#undef add_histogram
#undef add_histogram_params
#undef pvohog_gen_settings

struct pvohog_parameters;
class pvohog_track_data;

/// A generator for computing test hog descriptors in an online manner
class pvohog_generator : public online_descriptor_generator
{

public:

  pvohog_generator();
  ~pvohog_generator() {}

  bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  // Used to set and initialize any track data we might want to store for
  // the tracks duration, this data will be provided via future step function
  // calls.
  virtual track_data_sptr new_track_routine( track_sptr const& new_track );

  // Standard update function called for all active tracks on the current
  // frame.
  virtual bool track_update_routine( track_sptr const& active_track,
                                     track_data_sptr track_data );

  // Called everytime a track is terminated, in case we want to output
  // any partially complete descriptors.
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data );

private:

  void push_results(pvohog_track_data* data);

  boost::shared_ptr<pvohog_parameters> params_;
};


} // end namespace vidtk

#endif // vidtk_pvohog_generator_h_
