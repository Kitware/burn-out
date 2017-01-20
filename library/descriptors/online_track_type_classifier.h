/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_online_track_type_classifier_h_
#define vidtk_online_track_type_classifier_h_

#include <descriptors/online_descriptor_generator.h>

#include <utilities/track_demultiplexer.h>

#include <tracking_data/raw_descriptor.h>
#include <tracking_data/pvo_probability.h>

#include <vector>
#include <string>
#include <map>
#include <set>

namespace vidtk
{

/// \brief A class which consolidates multiple raw descriptors or object type
/// classifications in raw descriptor form (formerly PVO values) into a single
/// classification for an individual track object.
///
/// Derived versions of this class can either use some classifier to combine
/// multiple raw descriptors into a single object classification, or alternative
/// methods by defining different classify methods.
class online_track_type_classifier
{

public:

  typedef vidtk::pvo_probability object_class_t;
  typedef pvo_probability_sptr object_class_sptr_t;
  typedef std::vector< descriptor_id_t > descriptor_id_list_t;
  typedef track_sptr track_sptr_t;
  typedef descriptor_sptr descriptor_sptr_t;

  online_track_type_classifier();
  virtual ~online_track_type_classifier();

  /// \brief Given a list of active tracks, and a list of active descriptors
  /// generated for the current frame, fill in object classifications for
  /// the supplied tracks when able.
  ///
  /// This is the main entry point for using this class in external interfaces
  /// and should not be re-implemented by derived classes.
  void apply( track::vector_t& tracks, const raw_descriptor::vector_t& descriptors );

  /// \brief If a derived class requires it, this function will only pass descriptors
  /// with certain IDs to the classify function.
  ///
  /// If empty, descriptors of all types will be passed to the function.
  void set_ids_to_filter( const descriptor_id_list_t& ids = descriptor_id_list_t() );

  /// \brief If a derived class requires it, this function will only call the
  /// classify function for an individual track if the number of descriptors
  /// for that track exceeds the provided count (for the current frame).
  void set_min_required_count( unsigned descriptor_count );

  /// \brief If a derived class requires it, this function will only call the
  /// classify function for an individual track if the number of descriptors
  /// for that track is less than this provided count (for the current frame).
  void set_max_required_count( unsigned descriptor_count );

  /// \brief If this flag is set, all descriptors provided to the classify function
  /// will be sorted (alphabetically) based on their descriptor ids.
  ///
  /// This function exists to facilitate classifiers in which the order of descriptors
  /// is important (such as when they're used to formulate a feature vector).
  void set_sort_flag( bool sort_descriptors = true );

  /// \brief Set a known average gsd.
  void set_average_gsd( double average_gsd );

protected:

  /// \brief Given a list of raw descriptors corresponding to the given track,
  /// compute an updated object classification, if one exists.
  ///
  /// If an updated object type classification can not be computed, a NULL smart
  /// pointer should be returned.
  virtual object_class_sptr_t classify( const track_sptr_t& track_object,
                                        const raw_descriptor::vector_t& descriptors ) = 0;

  /// \brief Whenever a track is terminated, this function will be called by
  /// which can optionally be used to "close off" any computations pretaining
  /// to object type scores or alternatively do nothing (the default).
  virtual void terminate_track( const track_sptr_t& track_object );

  /// \brief Returns average GSD.
  double average_gsd() { return average_gsd_; }

private:

  // Descriptor ids to filter out
  descriptor_id_list_t id_filter_;

  // Required descriptor counts
  unsigned min_count_filter_;
  unsigned max_count_filter_;

  // Should descriptors provided to classify functions be sorted based on ids?
  bool sort_descriptors_;

  // Stored track demuxer for repeated operation
  track_demultiplexer track_demuxer_;

  // Known GSD
  double average_gsd_;

  // Last object classifications for all active tracks. These are used if we decide
  // not to update PVO scores on this step, but have a classification from prior frames.
  std::map< track::track_id_t, object_class_sptr_t > prior_scores_;

  // Update the track score for a given track
  void set_track_object_class( track_sptr_t& track, const object_class_sptr_t& scores );

  // Fill in the last known score for this track, if possible
  void use_last_score_for_track( track_sptr_t& track );
};

} // end namespace vidtk

#endif
