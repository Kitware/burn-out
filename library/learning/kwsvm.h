/*ckwg +5
 * Copyright 2010-2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _KWSVM_
#define _KWSVM_

#include <opencv2/ml/ml.hpp>

namespace vidtk
{

class KwSVM : public CvSVM
{
public:

  KwSVM();
  virtual ~KwSVM();

  KwSVM( const cv::Mat& trainData, const cv::Mat& responses,
         const cv::Mat& varIdx=cv::Mat(), const cv::Mat& sampleIdx=cv::Mat(),
         CvSVMParams params=CvSVMParams() );

  /// Classify (kwsample), a matrix with rows of samples and cols of sample elements.
  /// The classification scores are stored in (scores), one for each sample or each row of (kwsample)
  /// Argument (returnDFVal) indicates if the scores are categorial labels or raw scores to the decision boundary
  virtual float predict( const cv::Mat& kwsample, std::vector<float>& scores, bool returnDFVal=false) const;
  using CvSVM::predict;

  virtual bool predict_linear_svm( const cv::Mat& kwsample, std::vector<float>& scores) const;
  virtual float predict_linear_svm( const std::vector<float>& kwsample) const;

  int get_class_count() const  {return class_labels->cols; }
  void set_prob_scale(float s) { prob_scale_=s;}

  virtual std::vector<float>& get_linear_svm_coefficients() {return linear_svm_; }
  virtual void convert_to_linear_svm_coefficients();
  virtual bool write_linear_model( const std::string& fname ) const;
  virtual bool read_linear_model ( const std::string& fname, int counts=1e6 );

protected:
  virtual float predict( const float* row_sample, int row_len, std::vector<float>& row_score, bool returnDFVal=false ) const;
  virtual float predict_linear_svm( const float* row_sample, int row_len) const;

  float prob_scale_;

  std::vector<float> linear_svm_;
};

} // namespace vidtk

#endif
