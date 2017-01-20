/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_reinit_detector_h_
#define vidtk_track_reinit_detector_h_

#include <vil/vil_image_view.h>

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <object_detectors/obj_specific_salient_region_classifier.h>

#include <tracking_data/image_object.h>
#include <tracking_data/track.h>
#include <tracking_data/track_sptr.h>
#include <tracking_data/track_state.h>

#include <utilities/homography.h>
#include <utilities/ring_buffer.h>

#include <vector>
#include <limits>
#include <string>

namespace vidtk
{

/// Settings for the track_reinit_detector class.
struct track_reinit_settings
{
  /// Optional color descriptor
  bool use_color_descriptor;

  /// Optional area descriptor
  bool use_area_descriptor;

  /// Optional hog descriptor
  bool use_hog_descriptor;

  /// Optional dense bow descriptor
  bool use_dense_bow_descriptor;

  /// Optional ip based bow descriptor
  bool use_ip_bow_descriptor;

  /// Bag-of-words vocabulary for the dense feature point model.
  std::string dense_bow_vocab;

  /// Bag-of-words vocabulary for the ip based feature point model.
  std::string ip_bow_vocab;

  /// Type of learned model to use.
  enum { ADABOOST, ADABOOST_OCV } model_type;

  /// Maximum number of internal training iterations per model update.
  unsigned max_iterations;

  /// Maximum depth of each weak learner
  unsigned max_tree_depth;

  /// Type of boosting to use
  enum { REAL, GENTLE } boost_type;

  /// When creating an image chip for the object, the minimum desired area.
  double min_chip_area;

  /// When creating an image chip for the object, the minimum desired area.
  double max_chip_area;

  /// When creating an image chip, bounding box scale factor.
  double chip_scale_factor;

  /// When creating an image chip, pixel shift expansion count.
  unsigned chip_shift_factor;

  /// Should we rotate the image chip?
  bool use_obj_angle;

  // Defaults
  track_reinit_settings()
  : use_color_descriptor( true ),
    use_area_descriptor( true ),
    use_hog_descriptor( true ),
    use_dense_bow_descriptor( true ),
    use_ip_bow_descriptor( false ),
    dense_bow_vocab( "" ),
    ip_bow_vocab( "" ),
    model_type( ADABOOST_OCV ),
    max_iterations( 120 ),
    max_tree_depth( 3 ),
    boost_type( GENTLE ),
    min_chip_area( 5500.0 ),
    max_chip_area( 7000.0 ),
    chip_scale_factor( 1.37 ),
    chip_shift_factor( 0 ),
    use_obj_angle( false )
  {}
};

/// \brief A detector class to assist with re-initializing prior tracks.
///
/// This detector contains an internal model (a binary classifier) which
/// should be trained to distinguish the object under some track from other
/// objects or background. An external class is required to maintain training
/// data to train the internal model. This class contains a single internal
/// thread for training the model, where update_model is a non-blocking function
/// call which will begin training the new classifier if the thread is available,
/// otherwise the function call will exit.
class track_reinit_detector
{
public:

  typedef track_reinit_detector self_t;
  typedef boost::shared_ptr< self_t > sptr_t;
  typedef vil_image_view< vxl_byte > image_t;
  typedef vil_image_view< bool > mask_t;
  typedef double feature_t;
  typedef moving_training_data_container< feature_t > training_data_t;
  typedef std::vector< feature_t > descriptor_t;
  typedef vgl_box_2d< unsigned > image_region_t;
  typedef image_to_utm_homography homography_t;
  typedef track_sptr track_t;

  /// Constructor, require settings to be specified
  explicit track_reinit_detector( const track_reinit_settings& options );

  /// Destructor
  virtual ~track_reinit_detector();

  /// Extract features into the desired output location.
  ///
  /// output_loc must point to a location of size features_per_descriptor
  virtual bool extract_features( const image_t& image,
                                 const image_region_t& region,
                                 const double region_scale,
                                 feature_t* output_loc,
                                 const double rotation = 0.0 ) const;

  /// Given a vector of positive and negative training samples, launch
  /// a training thread, if it is available, to train a new model.
  virtual void update_model( const training_data_t& positive_samples,
                             const training_data_t& negative_samples );

  /// Given an image, an image region, and an approximate region scale,
  /// determine if the supplied region likely overlaps our object of interest
  /// via an appearance model only.
  virtual bool classify_obj( const image_t& image,
                             const image_region_t& region,
                             const double region_scale,
                             const double rotation = 0.0 );

  /// Given an image buffer of the last few frames, a track, an approximate
  /// scale of the image around the track, the current time, and current
  /// image to world homography, generate a linking descriptor for the track.
  virtual bool create_linking_descriptor( const ring_buffer< image_t >& image_buffer,
                                          const ring_buffer< timestamp >& ts_buffer,
                                          const track_sptr& track,
                                          const double region_scale,
                                          const double current_time,
                                          const homography_t& homog,
                                          descriptor_t& linking_descriptor );

  /// Given a pointer to a track, extractor information pretraining to it's
  /// last state and store it internally. This function is required to be set
  /// in order to formulate linking descriptors.
  virtual bool set_last_state_info( const homography_t& homog,
                                    const track_t& trk,
                                    const descriptor_t& descriptor,
                                    const double region_scale );

  /// Given the configured settings, how many features do each internal
  /// descriptor have?
  virtual unsigned features_per_descriptor() const;

  /// Reset any trained internal models. Useful if we want to now use
  /// this class on a new track or object.
  virtual void reset();

  /// Has an internal model been successfully trained since the last reset?
  virtual bool is_trained() const;

private:

  // A class for internal threading tasks, for updating the below model
  class model_update_threaded_task;

  // A class for storing the internal models and descriptor classes
  class object_model;
  boost::scoped_ptr< object_model > internal_model_;

};

} // end namespace vidtk


#endif // vidtk_track_reinit_detector_h_
