/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_obj_specific_salient_region_detector_h_
#define vidtk_obj_specific_salient_region_detector_h_

#include <classifier/hashed_image_classifier.h>

#include <utilities/point_view_to_region.h>
#include <utilities/video_modality.h>

#include <video_properties/border_detection.h>

#include <video_transforms/high_pass_filter.h>
#include <video_transforms/color_commonality_filter.h>
#include <video_transforms/convert_color_space.h>
#include <video_transforms/kmeans_segmentation.h>
#include <video_transforms/floating_point_image_hash_functions.h>

#include <object_detectors/pixel_feature_writer.h>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <string>
#include <vector>

namespace vidtk
{

/// Options for the obj_specific_salient_region_detector class.
struct ossrd_settings
{
  /// Should we use threads?
  bool use_threads_;

  /// EO Filename
  std::string initial_eo_model_fn_;

  /// IR Filename
  std::string initial_ir_model_fn_;

  /// Should we enable online learning?
  bool enable_online_learning_;

  /// The minumum number of model updates required before switching to the
  /// internal learned model.
  unsigned min_frames_for_learned_;

  /// Resolution per channel for utilized histograms
  unsigned histogram_resolution_;

  /// Max positive training samples to keep around
  unsigned max_pos_training_samples_;

  /// Max positive training samples to keep around
  unsigned max_neg_training_samples_;

  /// New training sample injection exponential averaging coefficient
  double sample_exp_factor_;

  /// Every time the model is updated, the number of training iterations to run for.
  unsigned iterations_per_update_;

  /// Should we use a dense bow descriptor as one of our features?
  bool enable_dense_bow_descriptor_;

  /// Should we use a hog descriptor as one of our features?
  bool enable_hog_descriptor_;

  /// Exponential averaging coefficient for the dense feature descriptor
  double dense_bow_exp_factor_;

  /// Exponential averaging coefficient for the HoG descriptor
  double hog_exp_factor_;

  /// Color space to work out of for color based features.
  color_space color_space_;

  /// Desired meters per pixel we want to perform filtering at.
  double desired_filter_gsd_;

  /// Maximum area in pixels to perform filtering on, if upsampling.
  unsigned max_filter_area_pixels_;

  /// Crop of top left point that motion image inputs are assumed to have
  unsigned motion_crop_i_;

  /// Crop of top left point that motion image inputs are assumed to have
  unsigned motion_crop_j_;

  // Defaults
  ossrd_settings()
  : use_threads_( true ),
    initial_eo_model_fn_( "" ),
    initial_ir_model_fn_( "" ),
    enable_online_learning_( false ),
    min_frames_for_learned_( 1 ),
    histogram_resolution_( 8 ),
    max_pos_training_samples_( 800 ),
    max_neg_training_samples_( 2000 ),
    sample_exp_factor_( 0.20 ),
    iterations_per_update_( 15 ),
    enable_dense_bow_descriptor_( false ),
    enable_hog_descriptor_( true ),
    dense_bow_exp_factor_( 0.20 ),
    hog_exp_factor_( 0.25 ),
    color_space_( HSV ),
    desired_filter_gsd_( 0.18 ),
    max_filter_area_pixels_( 250000 ),
    motion_crop_i_( 0 ),
    motion_crop_j_( 0 )
  {}
};


/// Decides what part of some type of imagery is foreground by combining simple
/// object recognition and other techniques around some rectangle in the image.
/// Typically, we start with some model trained to detect all salient regions
/// in an image, and slowly specialize that model for a specific object of interest
/// over time.
template< typename PixType = vxl_byte >
class obj_specific_salient_region_detector
{

public:

  // Internal typedefs
  typedef obj_specific_salient_region_detector< PixType > self_t;
  typedef vxl_byte feature_t;
  typedef vil_image_view< PixType > input_image_t;
  typedef vil_image_view< feature_t > feature_image_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef vil_image_view< float > motion_image_t;
  typedef vil_image_view< double > weight_image_t;
  typedef vil_image_view< unsigned > label_image_t;
  typedef vgl_box_2d< unsigned > image_region_t;
  typedef vgl_point_2d< unsigned > image_point_t;
  typedef image_border image_border_t;
  typedef hashed_image_classifier< feature_t > classifier_t;
  typedef double descriptor_value_t;
  typedef std::vector< descriptor_value_t > descriptor_t;
  typedef std::vector< unsigned > histogram_t;
  typedef std::vector< feature_t > normalized_histogram_t;
  typedef boost::function< void() > filter_func_t;
  typedef boost::shared_ptr< boost::thread > thread_sptr_t;

  obj_specific_salient_region_detector();
  obj_specific_salient_region_detector( const ossrd_settings& settings );
  virtual ~obj_specific_salient_region_detector();

  /// Set internal settings and load all required models.
  virtual bool configure( const ossrd_settings& settings );

  /// Reset the learned model and specify the initial modality.
  virtual void reset( const bool is_eo = true );

  /// \brief Process a new frame, returning an output mask highlighting all pixel
  /// locations deemed salient according to some internal classifier. This function
  /// is not currently thread safe.
  ///
  /// \param image Source RGB image
  /// \param region The region in the image plane we want to process
  /// \param motion_mask The region motion based fg image
  /// \param motion_image The region motion based fg image
  /// \param gsd The average gsd of the image
  /// \param mask The output salient pixels mask
  virtual void apply_model( const input_image_t& image,
                            const image_region_t& region,
                            const mask_image_t& motion_mask,
                            const motion_image_t& motion_image,
                            const double gsd,
                            weight_image_t& output );

  /// \brief Process a new frame, returning an output mask highlighting all pixel
  /// locations deemed salient according to some internal classifier. This function
  /// is not currently thread safe.
  ///
  /// \param image Source RGB image
  /// \param region The region in the image plane we want to process
  /// \param motion_mask The region motion based fg image
  /// \param motion_image The region motion based fg image
  /// \param gsd The average gsd of the image
  /// \param center An estimated center for our object
  /// \param mask The output salient pixels mask
  virtual void apply_model( const input_image_t& image,
                            const image_region_t& region,
                            const mask_image_t& motion_mask,
                            const motion_image_t& motion_image,
                            const double gsd,
                            const image_point_t& center,
                            weight_image_t& output );

  /// \brief Update the internal model for this detector given the latest
  /// detection window for some object, and a mask detailing which pixels
  /// likely belong to this mask.
  ///
  /// \param image Source RGB image
  /// \param region The region in the image plane we want to process
  /// \param region_mask A mask with the same size as the region
  /// \param motion_mask The region motion based fg image
  /// \param motion_image The region motion based fg image
  /// \param gsd The average gsd of the image
  virtual void update_model( const input_image_t& image,
                             const image_region_t& region,
                             const mask_image_t& region_mask,
                             const mask_image_t& motion_mask,
                             const motion_image_t& motion_image,
                             const double gsd );

  /// \brief Given some groundtruth mask, write out the features for
  /// every pixel in the input region.
  ///
  /// \param image Source RGB image
  /// \param region The region in the image plane we want to process
  /// \param motion_mask The region motion based fg image
  /// \param motion_image The region motion based fg image
  /// \param gsd The average gsd of the image
  /// \param mask The ground-truthed mask
  virtual void extract_features( const input_image_t& image,
                                 const image_region_t& region,
                                 const mask_image_t& motion_mask,
                                 const motion_image_t& motion_image,
                                 const double gsd,
                                 const image_point_t& center,
                                 const label_image_t& labels,
                                 const std::string& output_fn,
                                 const std::string& feature_folder = "" );


private:

  static const unsigned max_thread_count = 10;
  static const unsigned utilized_threads = 8;
  static const unsigned max_feature_count = 14;
  static const unsigned utilized_features = 12;

  // Externally set options
  ossrd_settings settings_;

  // Stored internal classifiers and related variables
  feature_image_t feature_image_[ max_feature_count ];
  classifier_t initial_eo_classifier_;
  classifier_t initial_ir_classifier_;

  // Variables used across threads
  enum thread_status { UNTASKED, TASKED, COMPLETE, KILLED };
  std::vector< thread_sptr_t > threads_;
  thread_status thread_status_[ max_thread_count ];
  boost::condition_variable thread_cond_var_[ max_thread_count ];
  boost::mutex thread_muti_[ max_thread_count ];

  // Inputs to all thread tasks
  input_image_t input_color_image_;
  image_region_t input_region_;
  motion_image_t input_motion_image_;
  mask_image_t input_motion_mask_;
  image_point_t input_location_;
  label_image_t input_labels_;
  double input_gsd_;
  feature_image_t normalized_region_color_;
  feature_image_t normalized_region_gray_;
  image_point_t normalized_location_;
  feature_image_t normalized_labels_;
  double normalized_gsd_;
  bool is_eo_flag_;

  // Model data forward declaration, is allocated if learning enabled.
  bool use_learned_model_;
  class model_data;
  boost::scoped_ptr< model_data > model_data_;

  // Filter settings
  color_commonality_filter_settings cc_settings_;

  // Helper functions for different types of filtering
  void high_pass_filter1();
  void high_pass_filter2();
  void color_classifier_filter();
  void segmentation_filter();
  void motion_filter();
  void color_conversion_filter();
  void distance_filter();
  void cut_filter();
  void hog_model_filter();
  void dense_bow_model_filter();

  // Create a normalized cropped image region image
  bool format_region_images( bool format_gt_image = false );

  // Apply the correct classifier to our first n features.
  void apply_classifier( const unsigned feature_count, weight_image_t& output );

  // Other helper functions
  void launch_filter_thread( unsigned id, filter_func_t func );
  void filter_thread_job( unsigned id, filter_func_t func );
  void wait_for_threads_to_finish( unsigned max_id );
  void task_thread( unsigned id );
  void kill_all_threads();

};

} // end namespace vidtk

#endif // vidtk_obj_specific_salient_region_detector_h_
