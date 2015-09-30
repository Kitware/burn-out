/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "gaussian_weak_learners.h"

#include <vpdl/vpdl_distribution.txx>

VPDL_DISTRIBUTION_INSTANTIATE(double,0);

namespace vidtk
{

//=======================================================================================//
weak_learner_sptr
weak_learner_gausian
::train( training_feature_set const & datas,
         vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  weak_learner_gausian * classifer = new weak_learner_gausian(name_,descriptor_);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
  vnl_vector<double> mean_class(n,0);
  vnl_vector<double> mean_not(n,0);
  double wgt_class = 0, wgt_not = 0;
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == 1)
    {
      mean_class += weights[i]*v;
      wgt_class += weights[i];
    }
    else
    {
      mean_not += weights[i]*v;
      wgt_not += weights[i];
    }
  }
  mean_class *= (1./wgt_class);
  mean_not   *= (1./wgt_not);
  vnl_matrix<double> covar_class(n,n,0);
  vnl_matrix<double> covar_not(n,n,0);
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == 1)
    {
      v = v - mean_class;
      covar_class += weights[i]*outer_product(v,v);
    }
    else
    {
      v = v - mean_not;
      covar_not += weights[i]*outer_product(v,v);
    }
  }
  covar_class *= (1./wgt_class);
  covar_not   *= (1./wgt_not);
  classifer->gaus_class_ = vpdl_gaussian< double, 0 >(mean_class,covar_class);
  classifer->gaus_not_ = vpdl_gaussian< double, 0 >(mean_not,covar_not);
  return (weak_learner*)(classifer);
}

bool
weak_learner_gausian
::read(vcl_istream & in)
{
  bool r = weak_learner::read(in);
  unsigned int n;
  in >> n;
  vnl_vector<double> mean_class(n,0);
  vnl_vector<double> mean_not(n,0);
  vnl_matrix<double> covar_class(n,n,0);
  vnl_matrix<double> covar_not(n,n,0);
  in >> mean_class >> covar_class >> mean_not >> covar_not;
  gaus_class_ = vpdl_gaussian< double, 0 >(mean_class,covar_class);
  gaus_not_ = vpdl_gaussian< double, 0 >(mean_not,covar_not);
  return r;
}

bool
weak_learner_gausian
::write(vcl_ostream & out) const
{
  bool r = weak_learner::write(out);
  out << gaus_class_.mean().size() << " " << gaus_class_.mean() << vcl_endl;
  out << gaus_class_.covariance() << vcl_endl;
  out << gaus_not_.mean() << vcl_endl;
  out << gaus_not_.covariance() << vcl_endl;
  return r;
}

//=========================================================================================//
weak_learner_sptr
weak_learner_single_gausian
::train( training_feature_set const & datas,
         vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  weak_learner_single_gausian * classifer =
    new weak_learner_single_gausian(name_,descriptor_);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
  vnl_vector<double> mean_class(n,0);
  double wgt_class = 0;
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == 1)
    {
      mean_class += weights[i]*v;
      wgt_class += weights[i];
    }
  }
  mean_class *= (1./wgt_class);
  vnl_matrix<double> covar_class(n,n,0);
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == 1)
    {
      v = v - mean_class;
      covar_class += weights[i]*outer_product(v,v);
    }
  }
  covar_class *= (1./wgt_class);
  classifer->gaus_class_ = vpdl_gaussian< double, 0 >(mean_class,covar_class);
  return (weak_learner*)(classifer);
}

bool
weak_learner_single_gausian
::read(vcl_istream & in)
{
  bool r = weak_learner::read(in);
  unsigned int n;
  in >> n;
  vnl_vector<double> mean_class(n,0);
  vnl_matrix<double> covar_class(n,n,0);
  in >> mean_class >> covar_class;
  gaus_class_ = vpdl_gaussian< double, 0 >(mean_class,covar_class);
  return r;
}

bool
weak_learner_single_gausian
::write(vcl_ostream & out) const
{
  bool r = weak_learner::write(out);
  out << gaus_class_.mean().size() << " " << gaus_class_.mean() << vcl_endl;
  out << gaus_class_.covariance() << vcl_endl;
  return r;
}

//=========================================================================================//
weak_learner_sptr
weak_learner_single_hw_gausian
::train( training_feature_set const & datas,
         vnl_vector<double> const & weights )
{
  assert( weights.size() == datas.size() );
  assert( datas.size() );
  double wgt_class = 0;
  double wgt_not = 0;
  sign_ = 1;
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == 1)
    {
      wgt_class += weights[i];
    }
    else
    {
      wgt_not += weights[i];
    }
  }
  if(wgt_class<wgt_not)
  {
    sign_ = -1;
  }
  weak_learner_single_hw_gausian * classifer = new weak_learner_single_hw_gausian(name_,descriptor_);
  const unsigned int n = datas[0]->get_value(descriptor_).size();
  vnl_vector<double> mean(n,0);
  double wgt = 0;
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == sign_)
    {
      mean += weights[i]*v;
      wgt += weights[i];
    }
  }
  mean *= (1./wgt);
  vnl_matrix<double> covar(n,n,0);
  for( unsigned int i = 0; i < datas.size(); ++i )
  {
    vnl_vector<double> v = datas[i]->get_value(descriptor_);
    if(datas[i]->label() == sign_)
    {
      v = v - mean;
      covar += weights[i]*outer_product(v,v);
    }
  }
  covar *= (1./wgt);
  classifer->gaus_ = vpdl_gaussian< double, 0 >(mean,covar);
  classifer->sign_ = sign_;
  return (weak_learner*)(classifer);
}

bool
weak_learner_single_hw_gausian
::read(vcl_istream & in)
{
  bool r = weak_learner::read(in);
  unsigned int n;
  in >> n;
  vnl_vector<double> mean(n,0);
  vnl_matrix<double> covar(n,n,0);
  in >> mean >> covar >> sign_;
  gaus_ = vpdl_gaussian< double, 0 >(mean,covar);
  return r;
}

bool
weak_learner_single_hw_gausian
::write(vcl_ostream & out) const
{
  bool r = weak_learner::write(out);
  out << gaus_.mean().size() << " " << gaus_.mean() << vcl_endl;
  out << gaus_.covariance() << vcl_endl << sign_ << vcl_endl;
  return r;
}

} //namespace vidtk
