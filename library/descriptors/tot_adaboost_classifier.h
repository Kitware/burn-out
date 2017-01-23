/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tot_adaboost_classifier_h_
#define vidtk_tot_adaboost_classifier_h_

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/online_track_type_classifier.h>

#include <learning/adaboost.h>

#include <tracking_data/raw_descriptor.h>

#include <vector>
#include <string>
#include <map>
#include <set>

namespace vidtk
{

/// Settings for the tot_adaboost_classifier class.
#define settings_macro( add_param ) \
  add_param( \
    main_person_classifier, \
    std::string, \
    "", \
    "Main world vs person classifier filename" ); \
  add_param( \
    main_vehicle_classifier, \
    std::string, \
    "", \
    "Main world vs vehicle classifier filename" ); \
  add_param( \
    main_other_classifier, \
    std::string, \
    "", \
    "Main world vs other classifier filename" ); \
  add_param( \
    extract_features, \
    bool, \
    false, \
    "Are we in training mode? This will write track ids and feature vectors " \
    "to file" ); \
  add_param( \
    feature_filename_prefix, \
    std::string, \
    "", \
    "File to place features into, if in training mode" ); \
  add_param( \
    track_id_adj, \
    unsigned, \
    0, \
    "If in training mode, add this value to track ids being written to file" ); \
  add_param( \
    gsd_pivot, \
    double, \
    0.35, \
    "If this value is non-zero, a different classifier will be used at higher " \
    "GSDs " \
    "above this threshold" ); \
  add_param( \
    high_gsd_person_classifier, \
    std::string, \
    "", \
    "Classifier filename for high gsd world vs person classifier" ); \
  add_param( \
    high_gsd_vehicle_classifier, \
    std::string, \
    "", \
    "Classifier filename for high gsd world vs vehicle classifier" ); \
  add_param( \
    high_gsd_other_classifier, \
    std::string, \
    "", \
    "Classifier filename for high gsd world vs other classifier" ); \
  add_param( \
    enable_temporal_averaging, \
    bool, \
    true, \
    "Should temporal averaging of classifier outputs be enabled?" ); \
  add_param( \
    frame_pivot, \
    unsigned, \
    10, \
    "If non-zero and temporal averaging is enabled, classifications after " \
    "we classify a track this many times will count double" ); \
  add_param( \
    gsd_descriptor, \
    std::string, \
    "meds", \
    "Descriptor ID which supplies a GSD, if one exists." ); \
  add_param( \
    post_proc_mode, \
    std::string, \
    "scaled_ratio", \
    "Post processing mode which converts classifier outputs into probabilities. " \
    "Can currently only be either none or scaled_ratios" ); \
  add_param( \
    max_score_to_report, \
    double, \
    0.92, \
    "Do not report a probability higher than this score" ); \
  add_param( \
    positive_scale_percent, \
    double, \
    2.0, \
    "For scaled_ratio mode, weight positive classification" ); \
  add_param( \
    normalization_additive_factor, \
    std::string, \
    "0.10 0.10 0.10", \
    "For scaled_ratio mode, add this amount to PVO values before normalization." ); \
  add_param( \
    normalization_expansion_ratio, \
    double, \
    0.0, \
    "Value in [0,1] for expanding the highest ranked category towards." ); \

init_external_settings( tot_adaboost_settings, settings_macro )

#undef settings_macro


/// A class which consolidates multiple descriptors into a single TOT classification,
/// for every track, via AdaBoosting.
class tot_adaboost_classifier : public online_track_type_classifier
{

public:

  typedef tot_adaboost_settings settings_t;

  /// Constructor
  tot_adaboost_classifier();

  /// Destructor
  virtual ~tot_adaboost_classifier();

  /// Use External Settings
  bool configure( const settings_t& params );

protected:

  /// Given a track object and a list of corresponding descriptors for that
  /// track for the current state, derive an object class for the given track.
  object_class_sptr_t classify( const track_sptr_t& track_object,
                                const raw_descriptor::vector_t& descriptors );

  /// Called for every terminated track to remove internal variables.
  virtual void terminate_track( const track_sptr_t& track_object );

private:

  // Internal settings
  settings_t settings_;
  enum { NONE, SCALED_RATIO } postproc_mode_;
  double norm_factors_[3];

  // Internal helper functions
  void classify_track( raw_descriptor::vector_t& descriptors, track_sptr& trk, double gsd );
  void scale_pvo_values( double& p, double& v, double& o );
  void average_pvo_values( unsigned track_id, double& p, double& v, double& o );

  // Loaded classifiers
  adaboost main_classifiers_[3];
  adaboost high_gsd_classifiers_[3];

  // Variables used for historical averaging, if enabled
  struct cumulative_pvo
  {
    double p_sum, v_sum, o_sum;
    unsigned count;
    bool hit_pivot;

    cumulative_pvo()
    : p_sum(0),
      v_sum(0),
      o_sum(0),
      count(0),
      hit_pivot(false)
    {}
  };

  // A map of prior PVO values
  std::map< unsigned, cumulative_pvo > pvo_priors_;
};

} // end namespace vidtk

#endif
