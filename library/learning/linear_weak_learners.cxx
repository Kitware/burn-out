/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_cassert.h>
#include "linear_weak_learners.h"

namespace vidtk
{

//=========================================================================================//
weak_learner_sptr
linear_weak_learner::train( training_feature_set const & datas,
                               vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  linear_weak_learner * result = new linear_weak_learner(name_,descriptor_);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
  vnl_matrix< double > X(datas.size(), n+1);
  vnl_matrix< double > W(datas.size(),datas.size());
  W.set_identity();
  vnl_vector< double > y(datas.size());
  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    for( unsigned int j = 0; j < v.size(); ++j)
    {
      X(i,j) = v[j];
    }
    X(i,v.size()) = 1;
    y[i] = datas[i]->label()*weights[i];
    W(i,i) = weights[i];
  }
  vnl_matrix<double> Xt = X.transpose();
  vnl_matrix<double> XtWX = Xt*W*X;
  vnl_matrix_inverse<double> XtWX_i(XtWX);
  result->sign_ = 1;
  result->beta_ = XtWX_i * Xt * y;
  result->beta_ /= result->beta_[n];
  double er = learner_base::error_rate(result,datas,weights);
  if(er > 0.5) {
    result->sign_ = -1;
  }
  return result;
}

int linear_weak_learner::classify(learner_data const & data)
{
  vnl_vector<double> v = data.get_value(descriptor_);
  double dp = 0;
  for(unsigned int i = 0; i < v.size(); ++i)
  {
    dp += v[i]*beta_[i];
  }
  dp += beta_[v.size()];
  if(dp < 0) return -this->sign_;
  return this->sign_;
}

vcl_string linear_weak_learner::gplot_command() const
{
  assert(beta_.size() == 3);
  double sl = -beta_[0]/beta_[1];
  double c = -beta_[2]/beta_[1];
  vcl_stringstream ss;
  #define l(x) (((sl)*(x))+(c))
  ss << "set arrow from -100," << l(-100) << " to 100," << l(100) << " nohead lt -1 lw 1.2\n";
  vcl_string result(ss.str());
  return result;
}

bool linear_weak_learner::read(vcl_istream & in)
{
  bool r = weak_learner::read(in);
  unsigned int s;
  in >> s;
  beta_.set_size(s);
  in >> beta_ >> sign_;
  return r;
}

bool linear_weak_learner::write(vcl_ostream & out) const
{
  bool r = weak_learner::write(out);
  out << beta_.size() << " " << beta_ << " " << sign_ << vcl_endl;
  return r;
}

//=========================================================================================//
weak_learner_sptr
lm_linear_weak_learner_v2::train( training_feature_set const & datas,
                                     vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  vcl_cout << "begin" << vcl_endl;
  lm_linear_weak_learner_v2 * result =
    new lm_linear_weak_learner_v2(name_,descriptor_);
  assert(result);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
//       vcl_cout << n << vcl_endl;
  vnl_matrix< double > X(datas.size(), n+1);
  vnl_matrix< double > W(datas.size(),datas.size());
  W.set_identity();
  vnl_vector< double > y(datas.size());
  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    for( unsigned int j = 0; j < v.size(); ++j)
    {
      X(i,j) = v[j];
    }
    X(i,v.size()) = 1;
    y[i] = datas[i]->label()*weights[i];
    W(i,i) = weights[i];
//         vcl_cout << weights[i] << vcl_endl;
  }
  vnl_matrix<double> Xt = X.transpose();
  vnl_matrix<double> XtWX = Xt*W*X;
  vnl_matrix_inverse<double> XtWX_i(XtWX);
  vnl_vector<double> beta = XtWX_i * Xt * y;
  result->beta_ = vnl_vector<double>(n+1);
//       vcl_cout << beta << vcl_endl;
  function lmf(n+1, datas, weights);
  vnl_levenberg_marquardt lm(lmf);
  lm.minimize_without_gradient(beta);
  result->beta_ = beta;
  double er = learner_base::error_rate(result,datas,weights);
  if(er > 0.5) {
    result->sign_ = -1;
  }
  else
  {
    result->sign_ = 1;
  }
  vcl_cout << "end" << vcl_endl;
  return result;
}

//=========================================================================================//

weak_learner_sptr
lm_linear_weak_learner::train( vcl_vector< learner_training_data_sptr > const & datas,
                                  vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  lm_linear_weak_learner * result = new lm_linear_weak_learner(name_,descriptor_);
  assert(result);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
//       vcl_cout << n << vcl_endl;
  vnl_matrix< double > X(datas.size(), n+1);
  vnl_matrix< double > W(datas.size(),datas.size());
  W.set_identity();
  vnl_vector< double > y(datas.size());
  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    for( unsigned int j = 0; j < v.size(); ++j)
    {
      X(i,j) = v[j];
    }
    X(i,v.size()) = 1;
    y[i] = datas[i]->label()*weights[i];
    W(i,i) = weights[i];
//         vcl_cout << weights[i] << vcl_endl;
  }
  vnl_matrix<double> Xt = X.transpose();
  vnl_matrix<double> XtWX = Xt*W*X;
  vnl_matrix_inverse<double> XtWX_i(XtWX);
  vnl_vector<double> beta = XtWX_i * Xt * y;
  vnl_vector<double> dir(n);
  result->beta_ = vnl_vector<double>(n+1);
  //double sum = 0;
  for(unsigned int i = 0; i < n; ++i)
  {
    dir[i] = beta[i];
//         sum += beta[i]*beta[i];
  }
//       vcl_cout << beta << vcl_endl;
  double mag = dir.magnitude();
  dir = dir/mag;
  vnl_vector<double> c(1,beta[n]/mag);
  //dir /= sum;
//       vcl_cout << dir << "  " << c << " " << dot_product(dir,dir) << vcl_endl;
  function lmf(dir, datas, weights);
  vnl_levenberg_marquardt lm(lmf);
  lm.minimize_without_gradient(c);
//       vcl_cout << dir << "  " << c << vcl_endl;
  for(unsigned int i = 0; i < n; ++i)
  {
    result->beta_[i] = dir[i];
  }
  result->beta_[n] = c[0];
//       vcl_cout << "HERE" << vcl_endl;
  double er = learner_base::error_rate(result,datas,weights);
  if(er > 0.5) {
    result->sign_ = -1;
  }
  else
  {
    result->sign_ = 1;
  }
  return result;
}

//=========================================================================================//
weak_learner_sptr
lm_bias_positive_linear_weak_learner
::train( training_feature_set const & datas,
         vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  lm_bias_positive_linear_weak_learner * result = new lm_bias_positive_linear_weak_learner(name_,descriptor_);
  assert(result);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
//       vcl_cout << n << vcl_endl;
  vnl_matrix< double > X(datas.size(), n+1);
  vnl_matrix< double > W(datas.size(),datas.size());
  W.set_identity();
  vnl_vector< double > y(datas.size());
  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    for( unsigned int j = 0; j < v.size(); ++j)
    {
      X(i,j) = v[j];
    }
    X(i,v.size()) = 1;
    y[i] = datas[i]->label()*weights[i];
    W(i,i) = weights[i];
  }
  vnl_matrix<double> Xt = X.transpose();
  vnl_matrix<double> XtWX = Xt*W*X;
  vnl_matrix_inverse<double> XtWX_i(XtWX);
  vnl_vector<double> beta = XtWX_i * Xt * y;
  vnl_vector<double> dir(n);
  result->beta_ = vnl_vector<double>(n+1);
  //double sum = 0;
  for(unsigned int i = 0; i < n; ++i)
  {
    dir[i] = beta[i];
//         sum += beta[i]*beta[i];
  }
//       vcl_cout << beta << vcl_endl;
  double mag = dir.magnitude();
  dir = dir/mag;
  vnl_vector<double> c(1,beta[n]/mag);
  //dir /= sum;
//       vcl_cout << dir << "  " << c << " " << dot_product(dir,dir) << vcl_endl;
  function lmf(dir, datas, weights);
  vnl_levenberg_marquardt lm(lmf);
  lm.minimize_without_gradient(c);
//       vcl_cout << dir << "  " << c << vcl_endl;
  for(unsigned int i = 0; i < n; ++i)
  {
    result->beta_[i] = dir[i];
  }
  result->beta_[n] = c[0];
  double er = learner_base::error_rate(result,datas,weights);
  if(er > 0.5) {
    result->sign_ = -1;
  }
  else
  {
    result->sign_ = 1;
  }
//       vcl_cout << "HERE" << vcl_endl;
  return result;
}

//=========================================================================================//
weak_learner_sptr
lm_bias_negative_linear_weak_learner
::train( training_feature_set const & datas,
         vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  lm_bias_negative_linear_weak_learner * result = new lm_bias_negative_linear_weak_learner(name_,descriptor_);
  assert(result);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
//       vcl_cout << n << vcl_endl;
  vnl_matrix< double > X(datas.size(), n+1);
  vnl_matrix< double > W(datas.size(),datas.size());
  W.set_identity();
  vnl_vector< double > y(datas.size());
  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    for( unsigned int j = 0; j < v.size(); ++j)
    {
      X(i,j) = v[j];
    }
    X(i,v.size()) = 1;
    y[i] = datas[i]->label()*weights[i];
    W(i,i) = weights[i];
  }
  vnl_matrix<double> Xt = X.transpose();
  vnl_matrix<double> XtWX = Xt*W*X;
  vnl_matrix_inverse<double> XtWX_i(XtWX);
  vnl_vector<double> beta = XtWX_i * Xt * y;
  vnl_vector<double> dir(n);
  result->beta_ = vnl_vector<double>(n+1);
  //double sum = 0;
  for(unsigned int i = 0; i < n; ++i)
  {
    dir[i] = beta[i];
//         sum += beta[i]*beta[i];
  }
//       vcl_cout << beta << vcl_endl;
  double mag = dir.magnitude();
  dir = dir/mag;
  vnl_vector<double> c(1,beta[n]/mag);
  //dir /= sum;
//       vcl_cout << dir << "  " << c << " " << dot_product(dir,dir) << vcl_endl;
  function lmf(dir, datas, weights);
  vnl_levenberg_marquardt lm(lmf);
  lm.minimize_without_gradient(c);
//       vcl_cout << dir << "  " << c << vcl_endl;
  for(unsigned int i = 0; i < n; ++i)
  {
    result->beta_[i] = dir[i];
  }
  result->beta_[n] = c[0];
  double er = learner_base::error_rate(result,datas,weights);
  if(er > 0.5) {
    result->sign_ = -1;
  }
  else
  {
    result->sign_ = 1;
  }
//       vcl_cout << "HERE" << vcl_endl;
  return result;
}

}//namespace vidtk
