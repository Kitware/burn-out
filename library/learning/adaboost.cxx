/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "adaboost.h"

#include <iostream>

#include <vnl/vnl_double_2.h>


#define DEBUG

#ifdef DEBUG
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>
#endif

#include <math.h>

#include <learning/linear_weak_learners.h>
#include <learning/gaussian_weak_learners.h>
#include <learning/stump_weak_learners.h>
#include <learning/histogram_weak_learners.h>
#include <learning/tree_weak_learners.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_adaboost_cxx__
VIDTK_LOGGER("adaboost_cxx");


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

void sigmoid_training(std::vector<vnl_double_2> const & training, double & A, double & B)
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
    B = std::log((prior0+1)/(prior1+1));
    double hiTarget = (prior1 + 1)/(prior1+2);
    double loTarget = 1/(prior0+2);
    double lambda = 1e-3;
    double olderr = 1e300;
    unsigned int count = 0;
    std::vector<double> pp(training.size(),((prior1+1)/(prior0+prior1+2)));
    for(unsigned int it = 0; it < 100; ++it)
    {
      double a = 0, b = 0, c = 0, d = 0, e = 0;
      for(unsigned int j = 0; j < training.size(); ++j)
      {
        double t = (training[j][0] == 1)?hiTarget:loTarget;
        double d1 = pp[j] - t;
        double d2 = pp[j]*(1-pp[j]);
        a += training[j][1]*training[j][1]*d2;
        b += d2;
        c += training[j][1]*d2;
        d += training[j][1]*d1;
        e += d1;
      }
      if( std::abs(d) < 1e-9 && std::abs(e) < 1e-9 )
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
        for(unsigned int j = 0; j < training.size(); ++j)
        {
          double p = 1/(1+std::exp(training[j][1]*A+B));
          pp[j] = p;
          double t = (training[j][0] == 1)?hiTarget:loTarget;
          //NOTE: there might be an error with log;
          double log_p = (p!=0)?std::log(p):-200.0;
          double log_1mp = (p!=1)?std::log(1-p):-200.0;
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
          LOG_INFO( "SOMETHING IS BROKEN HERE" );
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

void adaboost::train(std::vector<learner_training_data_sptr> const & d)
{
  assert(this->weak_learner_factories_.size());
  this->learners_.clear();
  training_feature_set training_set(d);
  unsigned int s = training_set.size();
  vnl_vector<double> wgts(s, 1.0/s);

  // For t = 1, ..., MAX
  for(unsigned int t = 0; t < this->max_number_of_iterations_; ++t)
  {
    assert(this->weak_learner_factories_[0]);
    // Find the best distribution for the current distribution.
    weak_learner_sptr wc = this->weak_learner_factories_[0]->train(training_set,wgts);
    double et = this->error_rate(wc.as_pointer(), training_set, wgts);
    //LOG_INFO( "\t" << et );

    for(unsigned int i = 1; i < this->weak_learner_factories_.size(); ++i)
    {
      weak_learner_sptr temp_wc = this->weak_learner_factories_[i]->train(training_set,wgts);
      double temp_et = this->error_rate(temp_wc.as_pointer(), training_set, wgts);
      //LOG_INFO( "\t" << temp_et );
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
    //LOG_ERROR( "ERROR: " << et );
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
      //LOG_INFO( et << " " << a_t );
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
adaboost::train_platt(std::vector<learner_training_data_sptr> const & datas)
{
  /// Calculate the Platt calibration parameters
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  std::vector< learner_training_data_sptr > rvect = datas;
  random_shuffle(rvect.begin(), rvect.end());
  std::vector< vnl_double_2 > platt_training_values;
  for(unsigned int i = 0; i < 3; ++i)
  {
    std::vector< learner_training_data_sptr > training;
    std::vector< learner_training_data_sptr > testing;
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
    std::vector<weak_learner_sptr> wcfs; //, unsigned int max = 100
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

double adaboost::classify_raw(learner_data const & data)
{

  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double result = 0;
  for( unsigned int i = 0; i < this->learners_.size(); ++i )
  {
    assert(this->learners_[i]);
    result += this->weights_[i]*this->learners_[i]->classify(data);
  }
  return result;
}

int adaboost::classify(learner_data const & data,unsigned int use_up_to)
{
  assert(use_up_to <= this->learners_.size());
  assert(use_up_to !=0);
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double result = 0;
  for( unsigned int i = 0; i < use_up_to; ++i )
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
::adaboost( std::vector<weak_learner_sptr> const & weak_learner_factories,
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
adaboost::read_from_file(const std::string& fn)
{
  std::ifstream input( fn.c_str() );

  if( input )
  {
    bool status = this->read( input );
    input.close();
    return status;
  }

  LOG_ERROR( "Unable to open: " << fn );
  return false;
}

bool
adaboost::read(std::istream & in)
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
      case weak_learners::tree_weak_learner:
        this->learners_[i] = new tree_weak_learner();
        break;
      default:
        LOG_INFO( "UNCONGIZED");
        return false;
    }
    if( !this->learners_[i]->read(in) )
    {
      LOG_ERROR( "Error parsing learner" );
      return false;
    }
  }
  if(!in.eof())
  {
    in >> platt_trained_ >> platt_A_ >> platt_B_;
  }
  return true;
}

bool
adaboost::write(std::ostream & out) const
{
  unsigned int wsize = this->weights_.size();
  out << wsize << std::endl;
  for(unsigned int i = 0; i < wsize; ++i)
  {
    out << this->weights_[i];
    if( i+1 < wsize)
    {
      out << " ";
    }
  }
  out << std::endl;
  out << this->learners_.size() << std::endl;
  for(unsigned int i = 0; i < this->learners_.size(); ++i)
  {
    out << static_cast<unsigned int>(this->learners_[i]->get_id()) << std::endl;
    this->learners_[i]->write(out);
    out << std::endl;
  }
  out << platt_trained_ << " " << platt_A_ << " " << platt_B_ << std::endl;
  return true;
}

double
adaboost::platt_probability(learner_data const & data)
{
  if(!this->platt_trained_)
  {
    return -1;
  }
  assert(this->platt_trained_);
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double fx = this->f_x(data);
  return 1./(1. + std::exp(platt_A_*fx+platt_B_));
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
    //LOG_INFO( "," << this->weights_[i] << ",");
    fx += this->weights_[i]*this->learners_[i]->classify01(data);
    //LOG_INFO( "");
  }
  fx = fx/weight_sum;

  return fx;
}

void
adaboost::calculate_stats( std::map< std::string, unsigned int > & weak_learner_counts,
                          std::map< std::string, vnl_double_2 > & average_value,
                          std::map< std::string, vnl_double_2 > & average_importance,
                          std::map< std::string, vnl_double_2 > & average_number_of_times )
{
  assert(this->learners_.size());
  assert(this->learners_.size() == this->weights_.size());
  double weight_sum = 0;
  std::map<std::string, int> been_visited;
  for( unsigned int i = 0; i < this->learners_.size(); ++i )
  {
    weight_sum += this->weights_[i];
  }
  for(unsigned int i = 0; i < this->learners_.size(); ++i)
  {
    std::string u = this->learners_[i]->unique_id();
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

bool adaboost::is_valid() const
{
  return !weights_.empty();
}

} //namespace vidtk
