/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "adaboost.h"

#include <vcl_iostream.h>

#include <vnl/vnl_double_2.h>


#define DEBUG

#ifdef DEBUG
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_sstream.h>
#include <vcl_iomanip.h>
#include <vcl_fstream.h>
#endif

#include <math.h>

#include <learning/linear_weak_learners.h>
#include <learning/gaussian_weak_learners.h>
#include <learning/stump_weak_learners.h>
#include <learning/histogram_weak_learners.h>

namespace vidtk
{

inline double alpha_t(double e_t)
{
  // NOTE:  More thought needs to be taken about what should happen
  //        when e_t is zero.
  if(e_t)
    return 0.5 * log((1.-e_t)/e_t);
  return 1.0;
}

void sigmoid_training(vcl_vector<vnl_double_2> const & training, double & A, double & B)
{
  double prior1 = 0;
  double prior0 = 0;
  for(unsigned int i = 0; i < training.size(); ++i)
  {
    if(training[i][0] == -1)
    {
      prior0++;
    }
    else if( training[i][0] == 1)
    {
      prior1++;
    }
    else
    {
      assert(!"This should not happen");
    }
    A = 0;
    B = vcl_log((prior0+1)/(prior1+1));
    double hiTarget = (prior1 + 1)/(prior1+2);
    double loTarget = 1/(prior0+2);
    double lambda = 1e-3;
    double olderr = 1e300;
    unsigned int count = 0;
    vcl_vector<double> pp(training.size(),((prior1+1)/(prior0+prior1+2)));
    for(unsigned int it = 0; it < 100; ++it)
    {
      double a = 0, b = 0, c = 0, d = 0, e = 0;
      for(unsigned int i = 0; i < training.size(); ++i)
      {
        double t = (training[i][0] == 1)?hiTarget:loTarget;
        double d1 = pp[i] - t;
        double d2 = pp[i]*(1-pp[i]);
        a += training[i][1]*training[i][1]*d2;
        b += d2;
        c += training[i][1]*d2;
        d += training[i][1]*d1;
        e += d1;
      }
      if( vcl_abs(d) < 1e-9 && vcl_abs(e) < 1e-9 )
      {
        break;
      }
      double oldA = A;
      double oldB = B;
      double err = 0;
      while(true)
      {
        double det = (a+lambda)*(b+lambda)-c*c;
        if( det == 0 )
        {
          lambda *=10;
          continue;
        }
        A = oldA + ((b+lambda)*d-c*e)/det;
        B = oldB + ((a+lambda)*e-c*d)/det;
        err = 0;
        for(unsigned int i = 0; i < training.size(); ++i)
        {
          double p = 1/(1+vcl_exp(training[i][1]*A+B));
          pp[i] = p;
          double t = (training[i][0] == 1)?hiTarget:loTarget;
          //NOTE: there might be an error with log;
          double log_p = (p!=0)?vcl_log(p):-200.0;
          double log_1mp = (p!=1)?vcl_log(1-p):-200.0;
//           assert(p);
//           assert(1-p);
          err -= t*log_p + (1-t)*log_1mp;
        }
        if(err<olderr*(1+1e-7))
        {
          lambda *= 0.1;
          break;
        }
        lambda *= 10;
        if(lambda >= 1e6 )
        {
          vcl_cout << "SOMETHING IS BROKEN HERE" << vcl_endl;
          break;
        }
        double diff = err - olderr;
        double scale = 0.5*(err+olderr+1);
        if( diff > -1e-3*scale && diff < 1e-7*scale )
        {
          count++;
        }
        else
        {
          count = 0;
        }
        olderr = err;
        if(count == 3)
        {
          break;
        }
      }
    }
  }
}

void adaboost::train(vcl_vector<learner_training_data_sptr> const & d)
{
  assert(this->weak_learner_factories_.size());
  this->learners_.clear();
  training_feature_set training_set(d);
  unsigned int s = training_set.size();
  vnl_vector<double> wgts(s, 1.0/(double)(s));

  // For t = 1, ..., MAX
  for(unsigned int t = 0; t < this->max_number_of_iterations_; ++t)
  {
    assert(this->weak_learner_factories_[0]);
    // Find the best distribution for the current distribution.
    weak_learner_sptr wc = this->weak_learner_factories_[0]->train(training_set,wgts);
    double et = this->error_rate(wc.as_pointer(), training_set, wgts);
    //vcl_cout << "\t" << et << vcl_endl;

    for(unsigned int i = 1; i < this->weak_learner_factories_.size(); ++i)
    {
      weak_learner_sptr temp_wc = this->weak_learner_factories_[i]->train(training_set,wgts);
      double temp_et = this->error_rate(temp_wc.as_pointer(), training_set, wgts);
      //vcl_cout << "\t" << temp_et << vcl_endl;
      if(temp_et<et)
      {
        wc = NULL;
        et = temp_et;
        wc = temp_wc;
      }
      else
      {
        temp_wc = NULL;
      }
    }
    //vcl_cout << "ERROR: " << et << vcl_endl;
    // If the error rate is greater than or equal to 50%, then stop
    if(et >= 0.5)
    {
      wc = NULL;
      break;
    }
    else //Add the new learner to the set
    {
      assert(wc);
      this->learners_.push_back(wc);
      double a_t = alpha_t(et);
      this->weights_.push_back(a_t);
      //vcl_cout << et << " " << a_t << vcl_endl;
      if(et != 0)
      {
        this->update_distribution(wc.as_pointer(), training_set, a_t, wgts);
      }
      else // The distribution is perfectly described so quit
      {
        break;
      }
    }
  }
  assert(this->learners_.size() != 0);
  assert(this->learners_.size() == this->weights_.size());
}

void
adaboost::trainPlatt(vcl_vector<learner_training_data_sptr> const & datas)
{
  /// Calculate the Platt calibration parameters
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  vcl_vector< learner_training_data_sptr > rvect = datas;
  random_shuffle(rvect.begin(), rvect.end());
  vcl_vector< vnl_double_2 > platt_training_values;
  for(unsigned int i = 0; i < 3; ++i)
  {
    vcl_vector< learner_training_data_sptr > training;
    vcl_vector< learner_training_data_sptr > testing;
    for(unsigned int t = 0; t < rvect.size(); ++t)
    {
      if(t%3==i)
      {
        testing.push_back(rvect[t]);
      }
      else
      {
        training.push_back(rvect[t]);
      }
    }
    vcl_vector<weak_learner_sptr> wcfs; //, unsigned int max = 100
    for(unsigned int t = 0; t < weak_learner_factories_.size(); ++t)
    {
      wcfs.push_back(weak_learner_factories_[t]->clone());
    }
    adaboost temp_ada(wcfs, max_number_of_iterations_);
    temp_ada.train(training);
    for( unsigned int t = 0; t < testing.size(); ++t)
    {
      vnl_double_2 r(testing[t]->label(),temp_ada.f_x(*testing[t]));
      platt_training_values.push_back(r);
    }
  }
  sigmoid_training(platt_training_values, platt_A_, platt_B_);
  platt_trained_ = true;
}

int adaboost::classify(learner_data const & data)
{
  return this->classify( data, learners_.size() );
}

int adaboost::classify(learner_data const & data,unsigned int use_upto)
{
  assert(use_upto <= this->learners_.size());
  assert(use_upto !=0);
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double result = 0;
  for( unsigned int i = 0; i < use_upto; ++i )
  {
    assert(this->learners_[i]);
    result += this->weights_[i]*this->learners_[i]->classify(data);
  }
  //If negative return -1 else return 1
  return (result<0)?-1:1;
}

double
learner_base
::error_rate( learner_base * cl,
              training_feature_set const & datas,
              vnl_vector<double> const & wgts )
{
  assert(datas.size() == wgts.size());
  double result = 0;
  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    int c = cl->classify(*(datas[i]));
    if(c != datas[i]->label())
    {
      result += wgts[i];
    }
  }
  return result;
}

void
adaboost
::update_distribution( weak_learner * cl,
                       training_feature_set const & datas,
                       double alpha_t,
                       vnl_vector<double> & wgts ) const
{
  assert(datas.size()==wgts.size());
  double sum = 0;
  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    int c = cl->classify(*(datas[i]));
    wgts[i] = wgts[i] * exp(-alpha_t * c * datas[i]->label());
    sum += wgts[i];
  }
  assert(sum != 0.0);
  for(unsigned int i = 0; i < wgts.size(); ++i)
  {
    wgts[i] = wgts[i]/sum;
  }
}

adaboost
::adaboost( vcl_vector<weak_learner_sptr> const & weak_learner_factories,
            unsigned int max )
  : weak_learner_factories_(weak_learner_factories),
    max_number_of_iterations_(max), platt_trained_(false)
{
}

adaboost::~adaboost()
{
  this->weak_learner_factories_.clear();
  this->learners_.clear();
}

bool
adaboost::read(vcl_istream & in)
{
  unsigned int ws;
  in >> ws;
  this->weights_.resize(ws);
  for(unsigned int i = 0; i < ws; ++i)
  {
    in >> this->weights_[i];
  }
  unsigned int cs;
  in >> cs;
  this->learners_.resize(cs, NULL);
  for(unsigned int i = 0; i < this->learners_.size(); ++i)
  {
    unsigned int id;
    in >> id;
    switch(id)
    {
      case weak_learners::stump_weak_learner:
        this->learners_[i] = new stump_weak_learner();
        break;
      case weak_learners::histogram_weak_learner:
        this->learners_[i] = new histogram_weak_learner();
        break;
      case weak_learners::weak_learner_gausian:
        this->learners_[i] = new weak_learner_gausian();
        break;
      case weak_learners::weak_learner_single_gausian:
        this->learners_[i] = new weak_learner_single_gausian();
        break;
      case weak_learners::weak_learner_single_hw_gausian:
        this->learners_[i] = new weak_learner_single_hw_gausian();
        break;
      case weak_learners::linear_weak_learner:
        this->learners_[i] = new linear_weak_learner();
        break;
      default:
        vcl_cerr << "UNCONGIZED";
        return false;
    }
    this->learners_[i]->read(in);
  }
  if(!in.eof())
  {
    in >> platt_trained_ >> platt_A_ >> platt_B_;
  }
  return true;
}

bool
adaboost::write(vcl_ostream & out) const
{
  unsigned int wsize = this->weights_.size();
  out << wsize << vcl_endl;
  for(unsigned int i = 0; i < wsize; ++i)
  {
    out << this->weights_[i];
    if( i+1 < wsize)
    {
      out << " ";
    }
  }
  out << vcl_endl;
  out << this->learners_.size() << vcl_endl;
  for(unsigned int i = 0; i < this->learners_.size(); ++i)
  {
    out << (unsigned int)(this->learners_[i]->get_id()) << vcl_endl;
    this->learners_[i]->write(out);
    out << vcl_endl;
  }
  out << platt_trained_ << " " << platt_A_ << " " << platt_B_ << vcl_endl;
  return true;
}

double
adaboost::probability(learner_data const & data)
{
  if(!this->platt_trained_)
  {
    return -1;
  }
  assert(this->platt_trained_);
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double fx = this->f_x(data);
  return 1./(1. + vcl_exp(platt_A_*fx+platt_B_));
}


double adaboost::f_x(learner_data const & data)
{
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double fx = 0;
  double weight_sum = 0;
  for( unsigned int i = 0; i < this->learners_.size(); ++i )
  {
    assert(this->learners_[i]);
    weight_sum += this->weights_[i];
    //vcl_cout << "," << this->weights_[i] << ",";
    fx += this->weights_[i]*this->learners_[i]->classify01(data);
    //vcl_cout << "\n";
  }
  fx = fx/weight_sum;

  return fx;
}

void
adaboost::calculate_stats( vcl_map< vcl_string, unsigned int > & weak_learner_counts,
                          vcl_map< vcl_string, vnl_double_2 > & average_value,
                          vcl_map< vcl_string, vnl_double_2 > & average_importance,
                          vcl_map< vcl_string, vnl_double_2 > & average_number_of_times )
{
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double weight_sum = 0;
  vcl_map<vcl_string, int> been_visited;
  for( unsigned int i = 0; i < this->learners_.size(); ++i )
  {
    weight_sum += this->weights_[i];
  }
  for(unsigned int i = 0; i < this->learners_.size(); ++i)
  {
    vcl_string u = this->learners_[i]->unique_id();
    if(weak_learner_counts.find(u) == weak_learner_counts.end())
    {
      weak_learner_counts[u] = 0;
      average_value[u] = vnl_double_2(0,0);
      average_importance[u] = vnl_double_2(0,0);
      average_number_of_times[u] = vnl_double_2(0,0);
    }
    if(been_visited.find(u) == been_visited.end())
    {
      been_visited[u]=1;
      average_number_of_times[u][1]++;
    }
    weak_learner_counts[u]++;
    average_importance[u][0] += this->weights_[i]/weight_sum;
    average_importance[u][1]++;
    average_value[u][0] += this->learners_[i]->get_debug_value();
    average_value[u][1]++;
    average_number_of_times[u][0]++;
  }
}

} //namespace vidtk
