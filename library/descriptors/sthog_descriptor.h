/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sthog_descriptor_h_
#define vidtk_sthog_descriptor_h_

#include <learning/kwsvm.h>

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/core/internal.hpp>

#include <boost/shared_ptr.hpp>

#include <vector>
#include <map>
#include <fstream>

namespace vidtk
{

/** \class sthog_descriptor
  * \brief Implements STHOG descriptors
  *
  * STHOG extends 2-D spatial HOG feature (by Dalal and Triggs CVPR05) to
  * a spatiotemporal volume, i.e., a stack of image frames. 2-D HOG features
  * are extracted on each image and concatenated as a feature vector. It is a
  * useful descriptor to detect objects and events in video.
  *
  * The implementation is derived from HOGDescriptor in OpenCV, and provides
  * the following functionalities:
  * 1) Computation of STHOG descriptor at a particular location (rectangular box)
  * 2) Computation of STHOG descriptors at uniform spatial sampling locations of
  *    an image frame
  * 3) Computation of STHOG descriptor when the 2D ROI on each frame is
  *    independent (e.g., parallelepiped)
  * 4) Computation of STHOG descriptors when additional image frames are introduced
  *    (temporal progressive mode). For example, when the STHOG of frames
  *    (1,2,3,4,5) having been computed, the intermediate results on frames
  *    (2,3,4,5) can be leveraged for the STHOG on (2,3,4,5,6)
  * 5) STHOG detection at a location by applying the learned SVM model
  * 6) STHOG detection of a spatiotemporal volume by spatial scanning
  * 7) STHOG detection by scale scanning
*/

/** Standard typedefs */
typedef std::vector<float> HOGTYPE;
typedef std::vector<HOGTYPE> STHOGVECTYPE;
typedef std::vector<float> STHOGTYPE;


class sthog_profile
{
public:
  sthog_profile()
  : nframes_(5),
    width_(0),
    height_(0),
    winSize_(64,64),
    winStride_(8,8),
    blockSize_(16,16),
    blockStride_(8,8),
    cellSize_(8,8),
    padding_(8,8),
    margin_(4,4),
    nbins_(9),
    derivAperture_(1),
    winSigma_(-1),
    histogramNormType_(0),
    L2HysThreshold_(0.2),
    gammaCorrection_(false)
  {}

  int nframes_;    // number of frames in sthog
  int width_;      // frame width in pixels
  int height_;     // frame height in pixels

  // the following parameters are the same as those in HOGDescriptor
  cv::Size winSize_;
  cv::Size winStride_;
  cv::Size blockSize_;
  cv::Size blockStride_;
  cv::Size cellSize_;
  cv::Size padding_;
  cv::Size margin_;

  int nbins_;
  int derivAperture_;
  double winSigma_;
  int histogramNormType_;
  double L2HysThreshold_;
  bool gammaCorrection_;
};

/** Comparison function used by sthog_map
  * Euclidean distance to the origin plus normalized angle
*/
class ltPointCompare
{
public:
  bool operator()(const cv::Point p1, const cv::Point p2) const
  {
    return (( p1.x*p1.x + p1.y*p1.y ) - (p2.x*p2.x+p2.y*p2.y) + (atan(p1.y/((1e-12)+p1.x)) - atan(p2.y/((1e-12)+p2.x)))/3.14159*2 ) < 0.0;
  }
};

/** STHOG descriptor class */
class sthog_descriptor : public cv::HOGDescriptor
{
public:

  typedef KwSVM svm_t;
  typedef boost::shared_ptr<svm_t> svm_sptr;

  /** Default constructor */
  sthog_descriptor();

  /** Constructor */
  sthog_descriptor(int _nframes,
                   cv::Size _winSize,
                   cv::Size _blockSize,
                   cv::Size _blockStride,
                   cv::Size _cellSize,
                   int _nbins,
                   int _derivAperture=1,
                   double _winSigma=-1,
                   int _histogramNormType=L2Hys,
                   double _L2HysThreshold=0.2,
                   bool _gammaCorrection=false);

  sthog_descriptor( const sthog_profile& sp );

  /** Destructor */
  virtual ~sthog_descriptor() {}

  /** clear hit_scores. call this function before detect() */
  void initialize_detector();

  /** write out detection feature */
  void set_sthog_feature(bool b) { sthog_feature_ = b; }

  /** Get STHOG descriptor size */
  size_t get_sthog_descriptor_size() const;

  /** Check if the descriptor size matches the number of images and 2D HOG features */
  bool check_detector_size() const;

  /** Import linear SVM model */
  virtual void set_svm_detector(const std::vector<float>& _svm_detector);

  /** Load OpenCV SVM model from a model file */
  virtual void load_svm_ocv_model(const std::string& fname);

  /** Load linear SVM model from a model file */
  virtual void load_svm_linear_model(const std::string& fname, bool verbose=false);

  /** Set multiple OCV svm models */
  virtual void svm_ocv_push_back();

  /** Set multiple linear svm models */
  virtual void svm_linear_push_back();

  /** Compute HOG descriptor on an image */
  virtual void compute(const cv::Mat& img,
                       std::vector<float>& descriptors,
                       cv::Size winStride=cv::Size(),
                       cv::Size padding=cv::Size(),
                       const std::vector<cv::Point>& locations=std::vector<cv::Point>());

  using HOGDescriptor::compute;

  /** Compute HOG descriptor at independent points across frames */
  virtual void compute(std::vector<cv::Mat>& imgs, const std::vector<cv::Point>& pt_in_frames);

  /** Compuate STHOG descriptor by introducing additional images (imgs).
   * It is assumed the rest are in the buffer, especially useful
   * for the progressive mode */
  virtual void compute(std::vector<cv::Mat>& imgs,
                       std::vector<float>& descriptors,
                       cv::Size winStride=cv::Size(),
                       cv::Size padding=cv::Size(),
                       const std::vector<cv::Point>& locations=std::vector<cv::Point>());

  /** Detect object/event based on the STHOG descriptors at a fixed scale level */
  virtual void detect(std::vector<cv::Mat>& imgs,
                      std::vector<cv::Point>& foundLocations,
                      double hitThreshold=0,
                      cv::Size winStride=cv::Size(),
                      cv::Size padding=cv::Size(),
                      const std::vector<cv::Point>& searchLocations=std::vector<cv::Point>());

  using HOGDescriptor::detect;

  /** Detect object/event based on the STHOG descriptors at a fixed scale level using multiple SVM models */
  virtual void multi_model_detection(std::vector<cv::Mat>& imgs,
                                     std::vector<cv::Point>& hits,
                                     std::vector<double> hitThresholds=std::vector<double>(),
                                     cv::Size winStride=cv::Size(),
                                     cv::Size padding=cv::Size(),
                                     const std::vector<cv::Point>& locations=std::vector<cv::Point>());

  /** Detect object/event based on the STHOG descriptors at multiple scale levels */
  virtual void detectMultiScale(std::vector<cv::Mat>& imgs, std::vector<cv::Rect>& foundLocations,
                                double hitThreshold=0,
                                cv::Size winStride=cv::Size(),
                                cv::Size padding=cv::Size(),
                                double scale=1.05,
                                int maxLevels=64,
                                int groupThreshold=2) ;

  using HOGDescriptor::detectMultiScale;

  /** Detect object/event based on the STHOG descriptors at multiple scale levels using multiple SVM models */
  virtual void multi_model_scale_detection(std::vector<cv::Mat>& imgs,
                                           std::vector<cv::Rect>& foundLocations,
                                           std::vector<double> hitThresholds=std::vector<double>(),
                                           cv::Size winStride=cv::Size(),
                                           cv::Size padding=cv::Size(),
                                           double scale0=1.05,
                                           int maxLevels=64, int groupThreshold=2);

  /** Get the detection score associated with the detected ROI's */
  std::vector<double>& get_hit_scores() { return hit_scores_; }

  /** Get the model index used for detection */
  std::vector<int>& get_hit_models() { return hit_models_; }

  /** Get number of frames in the spatiotemporal volume */
  int get_nframes() { return nframes_; }

  /** Set number of frames in the spatiotemporal volume */
  void set_nframes( int n ) { nframes_ = n; }

  /** Get the computed STHOG descriptor at location pt */
  void get_sthog_descriptor(std::vector<float>& des, cv::Point pt);

  /** Clear the cached STHOG descriptor */
  void clear_sthog_map();

protected:

  /** Pop the front element, i.e. pop out the HOG descriptor of the leading image */
  void pop_front_sthog_map(int n);

  /** Detection score */
  std::vector<double> hit_scores_;

  /** Index of used model */
  std::vector<int> hit_models_;

  /** Number of frames in the spatialtemporal volume */
  int nframes_;

  /** Computed STHOG at location (Point) */
  std::map<cv::Point, STHOGVECTYPE, ltPointCompare> sthog_map_;

  /** OpenCV svm model */
  svm_sptr svm_ocv_;
  std::vector<svm_sptr> multi_svm_ocv_;

  /** multiple linear svm models; a vector form of svmDetector */
  std::vector< std::vector<float> > multi_svm_linear_;

  /** Detection feature file*/
  bool sthog_feature_;

};

} // end namespace vidtk

#endif
