/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/* This is mainly derived from OpenCV */

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
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_kwsvm_cxx__
VIDTK_LOGGER("kwsvm_cxx");

using namespace cv;

namespace vidtk
{

#define KW_IS_MAT(mat)   \
  ( mat != NULL &&       \
    (mat->type & CV_MAGIC_MASK) == CV_MAT_MAGIC_VAL &&  \
    mat->rows > 0 && \
    mat->cols > 0 && \
    mat->data.ptr != NULL )

#define KW_IS_SPARSE_MAT(mat) \
  ( (mat) != NULL &&          \
    (mat->type & CV_MAGIC_MASK) == CV_SPARSE_MAT_MAGIC_VAL)

#define KW_NODE_IDX(mat,node) \
  (reinterpret_cast<int*>(reinterpret_cast<uchar*>(node) + (mat)->idxoffset))

#define KW_NODE_VAL(mat,node) \
  (reinterpret_cast<void*>( reinterpret_cast<uchar*>(node) + (mat)->valoffset))


typedef struct CvSparseVecElem32f
{
  int idx;
  float val;
}
CvSparseVecElem32f;

void
cvPreparePredictData( const CvArr* sample, int dims_all, const CvMat* comp_idx,
                      int class_count, const CvMat* prob, float** row_sample,
                      int as_sparse CV_DEFAULT(0) );

float KwSVM::predict( const float* row_sample, int row_len, std::vector<float>& row_score, bool returnDFVal) const
{
  LOG_ASSERT( kernel == NULL, "kernal size" );
  LOG_ASSERT( row_sample == NULL, "sample size" );

  int var_count = get_var_count();
  LOG_ASSERT( row_len == var_count, "row length" );
  (void)row_len;

  int class_count = class_labels ? class_labels->cols :
    params.svm_type == ONE_CLASS ? 1 : 0;

  float result = 0;
  cv::AutoBuffer<float> _buffer(sv_total + (class_count+1)*2);
  float* buffer = _buffer;

  if( params.svm_type == EPS_SVR ||
      params.svm_type == NU_SVR ||
      params.svm_type == ONE_CLASS )
  {
    CvSVMDecisionFunc* df = static_cast<CvSVMDecisionFunc*>(decision_func);
    int i, sv_count = df->sv_count;
    double sum = -df->rho;

    kernel->calc( sv_count, var_count, const_cast<const float**>(sv), row_sample, buffer );
    for( i = 0; i < sv_count; i++ )
      sum += buffer[i]*df->alpha[i];

    result = params.svm_type == ONE_CLASS ? static_cast<float>(sum > 0) : static_cast<float>(sum);
  }
  else if( params.svm_type == C_SVC ||
           params.svm_type == NU_SVC )
  {
    CvSVMDecisionFunc* df = static_cast<CvSVMDecisionFunc*>(decision_func);
    float* vote = static_cast<float *>(buffer + sv_total);
    int i, j, k;

    memset( vote, 0, class_count*sizeof(vote[0]));
    kernel->calc( sv_total, var_count, const_cast<const float**>(sv), row_sample, buffer );
    double sum = 0.;

    for( i = 0; i < class_count; i++ )
    {
      for( j = i+1; j < class_count; j++, df++ )
      {
        sum = -df->rho;
        int sv_count = df->sv_count;
        for( k = 0; k < sv_count; k++ )
          sum += df->alpha[k]*buffer[df->sv_index[k]];

        vote[sum > 0 ? i : j]++;

        row_score.push_back( sum );
      }
    }

    for( i = 1, k = 0; i < class_count; i++ )
    {
      if( vote[i] > vote[k] )
        k = i;
    }

    result = returnDFVal && class_count == 2 ? static_cast<float>(sum) : static_cast<float>(class_labels->data.i[k]);
  }
  else
    LOG_ERROR( "INTERNAL ERROR: Unknown SVM type, the SVM structure is probably corrupted" );

  return result;

}

float KwSVM::predict( const Mat& kwsample, std::vector<float>& scores, bool returnDFVal) const
{
  float result = 0;
  float* row_sample = 0;

  CvMat cvsample = kwsample;
  CvMat *sample = &cvsample;

  CV_FUNCNAME( "KwSVM::predict" );

  __BEGIN__;

  int class_count;

  if( !kernel )
    LOG_ERROR( "The SVM should be trained first" );

  class_count = class_labels ? class_labels->cols :
    params.svm_type == ONE_CLASS ? 1 : 0;

  CV_CALL( cvPreparePredictData( sample, var_all, var_idx,
                                 class_count, 0, &row_sample ));

  result = predict( row_sample, get_var_count(), scores, returnDFVal );

  __END__;

  if( sample && (!KW_IS_MAT(sample) || sample->data.fl != row_sample) )
    cvFree( &row_sample );

  return result;

}

int icvCmpSparseVecElems( const void* a, const void* b )
{
  return ( (reinterpret_cast<CvSparseVecElem32f*>( const_cast< void* >(a) ) )->idx - ( reinterpret_cast<CvSparseVecElem32f*>( const_cast< void* > (b) ) )->idx );
}

void
cvPreparePredictData( const CvArr* _sample, int dims_all,
                      const CvMat* comp_idx, int class_count,
                      const CvMat* prob, float** _row_sample,
                      int as_sparse )
{
  float* row_sample = 0;
  int* inverse_comp_idx = 0;

  CV_FUNCNAME( "cvPreparePredictData" );

  __BEGIN__;

  const CvMat* sample = static_cast<const CvMat*>(_sample);
  float* sample_data;
  int sample_step;
  int is_sparse = static_cast<int>( KW_IS_SPARSE_MAT(sample) );
  int d, sizes[CV_MAX_DIM];
  int i, dims_selected;
  int vec_size;

  if( !is_sparse && !KW_IS_MAT(sample) )
    LOG_ERROR( "The sample is not a valid vector" );

  if( cvGetElemType( sample ) != CV_32FC1 )
    LOG_ERROR( "Input sample must have 32fC1 type" );

  CV_CALL( d = cvGetDims( sample, sizes ));

  if( !((is_sparse && d == 1) || (!is_sparse && d == 2 && (sample->rows == 1 || sample->cols == 1))) )
    LOG_ERROR( "Input sample must be 1-dimensional vector" );

  if( d == 1 )
    sizes[1] = 1;

  if( sizes[0] + sizes[1] - 1 != dims_all )
    LOG_ERROR( "The sample size is different from what has been used for training" );

  if( !_row_sample )
    LOG_ERROR( "INTERNAL ERROR: The row_sample pointer is NULL" );

  if( comp_idx && (!KW_IS_MAT(comp_idx) || comp_idx->rows != 1 ||
                   CV_MAT_TYPE(comp_idx->type) != CV_32SC1) )
    LOG_ERROR( "INTERNAL ERROR: invalid comp_idx" );

  dims_selected = comp_idx ? comp_idx->cols : dims_all;

  if( prob )
  {
    if( !KW_IS_MAT(prob) )
      LOG_ERROR( "The output matrix of probabilities is invalid" );

    if( (prob->rows != 1 && prob->cols != 1) ||
        (CV_MAT_TYPE(prob->type) != CV_32FC1 &&
         CV_MAT_TYPE(prob->type) != CV_64FC1) )
      LOG_ERROR( "The matrix of probabilities must be 1-dimensional vector of 32fC1 type" );

    if( prob->rows + prob->cols - 1 != class_count )
      LOG_ERROR( "The vector of probabilities must contain as many elements as the number of classes in the training set" );
  }

  vec_size = !as_sparse ? dims_selected*sizeof(row_sample[0]) :
    (dims_selected + 1)*sizeof(CvSparseVecElem32f);

  if( KW_IS_MAT(sample) )
  {
    sample_data = sample->data.fl;
    sample_step = CV_IS_MAT_CONT(sample->type) ? 1 : sample->step/sizeof(row_sample[0]);

    if( !comp_idx && CV_IS_MAT_CONT(sample->type) && !as_sparse )
      *_row_sample = sample_data;
    else
    {
      CV_CALL( row_sample = static_cast<float*>(cvAlloc( vec_size )));

      if( !comp_idx )
        for( i = 0; i < dims_selected; i++ )
          row_sample[i] = sample_data[sample_step*i];
      else
      {
        int* comp = comp_idx->data.i;
        for( i = 0; i < dims_selected; i++ )
          row_sample[i] = sample_data[sample_step*comp[i]];
      }

      *_row_sample = row_sample;
    }

    if( as_sparse )
    {
      const float* src = static_cast<const float*>(row_sample);
      CvSparseVecElem32f* dst = reinterpret_cast<CvSparseVecElem32f*>(row_sample);

      dst[dims_selected].idx = -1;
      for( i = dims_selected - 1; i >= 0; i-- )
      {
        dst[i].idx = i;
        dst[i].val = src[i];
      }
    }
  }
  else
  {
    CvSparseNode* node;
    CvSparseMatIterator mat_iterator;
    const CvSparseMat* sparse = reinterpret_cast<const CvSparseMat*>(sample);
    LOG_ASSERT( is_sparse, "sparse" );

    node = cvInitSparseMatIterator( sparse, &mat_iterator );
    CV_CALL( row_sample = static_cast<float*>(cvAlloc( vec_size )));

    if( comp_idx )
    {
      CV_CALL( inverse_comp_idx = static_cast<int*>(cvAlloc( dims_all*sizeof(int) )));
      memset( inverse_comp_idx, -1, dims_all*sizeof(int) );
      for( i = 0; i < dims_selected; i++ )
        inverse_comp_idx[comp_idx->data.i[i]] = i;
    }

    if( !as_sparse )
    {
      memset( row_sample, 0, vec_size );

      for( ; node != 0; node = cvGetNextSparseNode(&mat_iterator) )
      {
        int idx = *KW_NODE_IDX( sparse, node );
        if( inverse_comp_idx )
        {
          idx = inverse_comp_idx[idx];
          if( idx < 0 )
            continue;
        }
        row_sample[idx] = *(reinterpret_cast<float*>(KW_NODE_VAL( sparse, node )));
      }
    }
    else
    {
      CvSparseVecElem32f* ptr = reinterpret_cast<CvSparseVecElem32f*>(row_sample);

      for( ; node != 0; node = cvGetNextSparseNode(&mat_iterator) )
      {
        int idx = *(KW_NODE_IDX( sparse, node ));
        if( inverse_comp_idx )
        {
          idx = inverse_comp_idx[idx];
          if( idx < 0 )
            continue;
        }
        ptr->idx = idx;
        ptr->val = *(reinterpret_cast<float*>(KW_NODE_VAL( sparse, node )));
        ptr++;
      }

      qsort( row_sample, ptr - reinterpret_cast<CvSparseVecElem32f*>(row_sample),
             sizeof(ptr[0]), icvCmpSparseVecElems );
      ptr->idx = -1;
    }

    *_row_sample = row_sample;
  }

  __END__;

  if( inverse_comp_idx )
    cvFree( &inverse_comp_idx );

  if( cvGetErrStatus() < 0 && _row_sample )
  {
    cvFree( &row_sample );
    *_row_sample = 0;
  }
}

}
