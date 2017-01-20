/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <math.h>
#include <fstream>
#include <iostream>
#include <string>

#include <learning/kwsvm.h>
#include <opencv2/core/core_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/core/internal.hpp>

#include <utilities/kwocv_utility.h>

#include <logger/logger.h>
VIDTK_LOGGER("kwsvm_cxx");

namespace vidtk
{

KwSVM::KwSVM()
    : CvSVM(),
      prob_scale_(0.3)
{
}

KwSVM::~KwSVM()
{
  CvSVM::clear();
}

KwSVM::KwSVM( const cv::Mat& trainData, const cv::Mat& responses,  const cv::Mat& varIdx, const cv::Mat& sampleIdx, CvSVMParams _params )
    : CvSVM( trainData, responses, varIdx, sampleIdx, _params ),
      prob_scale_(1.0)
{
}

void KwSVM::convert_to_linear_svm_coefficients()
{
  // linear svm only

  int var_count = get_var_count();

  float* s = new float [var_count];
  for(int j=0; j<var_count; j++)
    s[j] = 0.0;

  CvSVMDecisionFunc* df = static_cast<CvSVMDecisionFunc*>(decision_func);

  for(int k=0; k<sv_total; k++)
  {
    const float* onesv = sv[ df->sv_index[k] ];
    for(int j=0; j < var_count; j++ )
      s[j] += onesv[j] * df->alpha[k];
  }

  linear_svm_.push_back( df->rho );

  delete[] s;
}

float KwSVM::predict_linear_svm( const float* row_sample, int row_len) const
{
  LOG_ASSERT( row_sample==NULL, "row size" );
  LOG_ASSERT( linear_svm_.size() >= static_cast<unsigned>(row_len), "linear svm size" );

  float sum=0.0;

  // bias term
  if( linear_svm_.size() > static_cast<unsigned>(row_len) )
    sum = linear_svm_[row_len];

  // linear svm
  for(int i=0; i<row_len; i++)
    sum += linear_svm_[i] * row_sample[i];

  return sum;
}

float KwSVM::predict_linear_svm( const std::vector<float>& kwsample) const
{
  float sum=0.0;
  if( linear_svm_.size() > kwsample.size() )
    sum = linear_svm_[ kwsample.size() ];

  for(unsigned i=0; i<kwsample.size(); i++ )
    sum += linear_svm_[i] * kwsample[i];

  return sum;
}

bool KwSVM::predict_linear_svm( const cv::Mat& kwsample, std::vector<float>& scores) const
{

  for(int i=0; i<kwsample.rows; i++)
  {
    float* row_sample = const_cast<float *>( kwsample.ptr<float>( i ) );
    float result = predict_linear_svm( row_sample, kwsample.cols );
    scores.push_back( result );
  }

  return true;
}

bool KwSVM::write_linear_model( const std::string& fname ) const
{
  std::ofstream ofs;
  ofs.open( fname.c_str() );
  if( !ofs.is_open() )
    return false;

  for(unsigned i=0; i<linear_svm_.size(); i++)
    ofs << linear_svm_[i] << std::endl;

  ofs.close();
  return true;
}

bool KwSVM::read_linear_model( const std::string& fname, int counts )
{
  std::ifstream ifs;
  ifs.open( fname.c_str() );
  if( !ifs.is_open() )
    return false;

  float t;
  linear_svm_.clear();
  std::string line;
  int i=0;

  while( i<counts && std::getline(ifs, line) )
  {
    std::vector<std::string> col;
    if(line=="")
      continue;

    kwocv::tokenize(line, col);

    if( col.size() == 1 )
    {
      t=atof(col[0].c_str());
      linear_svm_.push_back(t);
      ++i;
    }
  }

  ifs.close();

  return true;
}

}
