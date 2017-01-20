/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ocv_hog_detector_h_
#define vidtk_ocv_hog_detector_h_

#include <object_detectors/threaded_detector.h>

#include <utilities/config_block.h>
#include <utilities/external_settings.h>

#include <vector>
#include <string>

#include <tracking_data/image_object.h>

#include <vil/vil_image_view.h>

#include <vgl/vgl_box_2d.h>

#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>

#include <opencv2/objdetect/objdetect.hpp>

// Forward declaration of OpenCV DPM detector
namespace cv
{
  class LatentSvmDetector;
}


namespace vidtk
{


#define ocv_hog_settings_macro( add_param, add_array ) \
  add_param( \
    model_filename, \
    std::string, \
    "", \
    "The name of the model file to use. If empty, a default model " \
    "will be used, if possible." ); \
  add_param( \
    detector_threshold, \
    double, \
    -1.0, \
    "No detections with scores below this threshold will be reported." ); \
  add_param( \
    overlap_threshold, \
    float, \
    0.5, \
    "The overlap threshold passed to the detect function used for NMS." ); \
  add_param( \
    num_threads, \
    int, \
    1, \
    "Number of threads used when running HOG detectors." ); \
  add_param( \
    location_type, \
    std::string, \
    "centroid", \
    "Location of the target for tracking: bottom or centroid." ); \
  add_param( \
    use_dpm_detector, \
    bool, \
    true, \
    "Should a HOG DPM detector be used instead of a standard HOG kernel?" ); \
  add_param( \
    border_pixel_count, \
    unsigned, \
    2, \
    "Number of pixels near the image boundary to never record overlapping " \
    "detections in." ); \
  add_param( \
    output_detection_folder, \
    std::string, \
    "", \
    "If the string is set, a folder to output detection images into." ); \
  add_param( \
    levels, \
    int, \
    64, \
    "Number of levels for HOG computation if not using DPM." ); \
  add_param( \
    level_scale, \
    double, \
    1.05, \
    "Scale factor between HOG levels." ); \
  add_array( \
    ignore_regions, \
    unsigned, 0, \
    "", \
    "List of bounding boxed regions (minx, maxx, miny, maxy) to not " \
    "report any detections in." ); \
  add_param( \
    use_threads, \
    bool, \
    false, \
    "Whether or not to use internal threading." ); \
  add_param( \
    low_res_model, \
    bool, \
    false, \
    "Whether or not the default standard HoG model should be the lower " \
    "resolution (48x96) version." ); \

init_external_settings2( ocv_hog_detector_settings, ocv_hog_settings_macro );

#undef ocv_hog_settings_macro


/// A class which runs different variants of OpenCV HOG object detectors.
class ocv_hog_detector
{

public:

  typedef vil_image_view< vxl_byte > image_t;
  typedef vgl_box_2d< unsigned > region_t;

  ocv_hog_detector();
  virtual ~ocv_hog_detector();

  /// Configure any external settings.
  bool configure( ocv_hog_detector_settings const& settings );

  /// Using any internal models, return detections as vidtk image objects.
  ///
  /// Accepts an optional scale factor to be applied to the input image
  /// dimensions (detections will be rescaled accordingly).
  std::vector< image_object_sptr > detect( const image_t& image,
                                           const double scale = 1.0 );

protected:

  /// Load additional models from the specified file.
  bool load_model( std::string const& fname );

  /// Load additional models from the specified files.
  bool load_models( std::vector< std::string > const& fnames );

  /// Clear all loaded models.
  void reset_models();

private:

  typedef image_object::float_type float_type;

  // Settings which can be set externally by the caller.
  ocv_hog_detector_settings settings_;

  // OpenCV detector objects.
  boost::scoped_ptr< cv::LatentSvmDetector > dpm_detector_;
  boost::scoped_ptr< cv::HOGDescriptor > dt_detector_;
  boost::scoped_ptr< threaded_detector< image_t > > threads_;

  // Helpers for computing detections
  void compute_detections( const image_border& region,
                           const image_t& image );

  // Other misc settings and counters.
  enum location_type { centroid, bottom };
  location_type loc_type_;
  std::vector< region_t > ignore_regions_;
  std::vector< region_t > detection_bboxes_;
  std::vector< double > confidence_values_;
  double scale_factor_;
  boost::mutex attach_mutex_;
};

} // end namespace vidtk

#endif // vidtk_ocv_hog_detector_h_
