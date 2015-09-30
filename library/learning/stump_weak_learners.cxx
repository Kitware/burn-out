/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "stump_weak_learners.h"

#include <vcl_cassert.h>
#include <vcl_iostream.h>
#include <vcl_algorithm.h>

#include <vnl/vnl_math.h>

namespace vidtk
{

stump_weak_learner
::stump_weak_learner( int s, unsigned int d, double v,
                      vcl_string const & name, int desc )
      : weak_learner(name, desc),
        sign_(s), dim_(d), value_(v)
{
}

weak_learner_sptr
stump_weak_learner
::train( training_feature_set const & datas,
         vnl_vector<double> const & weights )
{
  assert( datas.size() > 0 );
  unsigned int n_dim = datas[0]->get_value(descriptor_).size();
  assert(n_dim >0);
  int best_sign;
  unsigned int best_dim = 0;
  double best_value;
  double best_error = this->calculate_singed(datas,weights,best_dim,best_sign,best_value);
  for(unsigned int i = 1; i < n_dim; ++i)
  {
    int tmp_sign;
    double tmp_value;
    double tmp_error = this->calculate_singed(datas,weights,i,tmp_sign,tmp_value);
    if(tmp_error < best_error)
    {
      best_error = tmp_error;
      best_sign = tmp_sign;
      best_value = tmp_value;
      best_dim = i;
    }
  }
  return (weak_learner *)(new stump_weak_learner( best_sign, best_dim,
                                                  best_value, name_,descriptor_) );
}

int
stump_weak_learner
::classify(learner_data const & data)
{
  double v = data.get_value_at(descriptor_,dim_);
  //vcl_cerr << descriptor_ << " " << dim_ << " " << v << " " << value_ << vcl_endl;
  if(v > value_)
  {
    return sign_;
  }
  else if(v==value_)
  {
    return -sign_;
  }
  else
  {
    return -sign_;
  }
}

struct value_weight
{
  double v;
  double pos_w;
  double neg_w;
  double pos_wv()
  { return pos_w*v; }
  double neg_wv()
  { return neg_w*v; }
  value_weight(){}
  value_weight(double inv, double pw, double nw)
    : v(inv), pos_w(pw), neg_w(nw)
  {}
};

double
stump_weak_learner
::calculate_singed( training_feature_set const & datas,
                    vnl_vector<double> const & wgts,
                    unsigned int d, int & s, double & v)
{
  assert(datas.size());
  vcl_vector<value_weight> vals;
  double pos_weight_sum = 0.0;
  double neg_weight_sum = 0.0;
  double pos_weight_sum_lte_split = 0.0;
  double neg_weight_sum_lte_split = 0.0;
  bool all_equal = true;

  for(unsigned int i = 0; i < datas.size(); ++i)
  {
    double v = datas(descriptor_,d,i);
    unsigned int loc = datas.get_location(descriptor_,d,i);

    double pos_w = 0;
    double neg_w = 0;
    if(datas[loc]->label() == 1)
    {
      pos_w = wgts[loc];
      pos_weight_sum += pos_w;
    }
    else
    {
      neg_w = wgts[loc];
      neg_weight_sum += neg_w;
    }
    if(vals.size() && vals[vals.size()-1].v == v)
    {
      vals[vals.size()-1].pos_w += pos_w;
      vals[vals.size()-1].neg_w += neg_w;
    }
    else
    {
      value_weight val(v,pos_w,neg_w);
      vals.push_back(val);
    }
  }
  all_equal = !(vals.size()-1);

  double best_split_v = 0.0;
  int best_sign = 2;
  double best_score = 0.0;

  for(unsigned int i = 0; i < vals.size()-1; ++i)
  {
    if(!(vals[i].v < vals[i+1].v))
    {
      vcl_cout << i << " " << name_ << " " << i+1 << " " << vals[i].v << " " << vals[i+1].v << "\t" << vals[i+1].v-vals[i].v << vcl_endl;
    }
    assert(vals[i].v < vals[i+1].v);
    double temp_split = static_cast<double>(vals[i].v + vals[i+1].v)*static_cast<double>(0.5);
    if(temp_split==vals[i+1].v)
    {
      temp_split = vals[i].v;
    }

    assert(vals[i].v <= temp_split);
    pos_weight_sum_lte_split += vals[i].pos_w;
    neg_weight_sum_lte_split += vals[i].neg_w;

    double pos_weight_sum_gt_split = pos_weight_sum - pos_weight_sum_lte_split;
    double neg_weight_sum_gt_split = neg_weight_sum - neg_weight_sum_lte_split;
    int temp_sign = 1;
    double temp_score = 0.0;
    if( neg_weight_sum_lte_split+pos_weight_sum_gt_split >= pos_weight_sum_lte_split+neg_weight_sum_gt_split)
    {
      temp_sign = 1;
      temp_score = pos_weight_sum_lte_split+neg_weight_sum_gt_split;
    }
    else
    {
      temp_sign = -1;
      temp_score = neg_weight_sum_lte_split+pos_weight_sum_gt_split;
    }
//     if(!(temp_score <= 0.5))
//     {
//       vcl_cout << temp_score << " " << neg_weight_sum_lte_split+pos_weight_sum_gt_split << " "
//                << pos_weight_sum_lte_split+neg_weight_sum_gt_split << " " << temp_score-0.5 << vcl_endl;
//     }
    if( vcl_abs(temp_score-0.5) < 1.e-10)
    {
      temp_score = 0.5;
    }
    assert(temp_score <= 0.5);

    if(temp_score < best_score || best_sign == 2)
    {
      best_score = temp_score;
      best_split_v = temp_split;
      best_sign = temp_sign;
    }
  }

  s = static_cast<int>( best_sign );
  v = best_split_v;
  stump_weak_learner tmp(s,d,v,name_,descriptor_);
  double result = 0.5;
  if(!all_equal)
  {
    result = this->error_rate(&tmp, datas, wgts);
    if(!(vcl_abs(result-best_score) < 1e-7))
    {
      vcl_cerr << " error " << result << "  " << best_score << vcl_endl;
    }
    assert(vcl_abs(result-best_score) < 1e-7);
  }
  if( vcl_abs(result-0.5) < 1.e-10)
  {
    result = 0.5;
  }
  if( !(result <= 0.5) )
  {
    vcl_cerr << "\t\ts " << s << " d " << d << " " << v << "  " << name_ << " " << descriptor_
             << " error " << result << "  " << best_score << "    " << vals.size() << vcl_endl;
  }
  assert(result <= 0.5);
  assert(!vnl_math::isinf(result));
  assert(!vnl_math::isinf(best_split_v));
  return result;
}

bool
stump_weak_learner
::read(vcl_istream & in)
{
  bool r = weak_learner::read(in);
  assert( in );
  in >> sign_;
  assert( in );
  in >> dim_;
  assert( in );
  in >> value_;
  return r && in;
}

bool
stump_weak_learner
::write(vcl_ostream & out) const
{
  bool r = weak_learner::write(out);
  out << sign_ << " " << dim_ << " " << value_ << vcl_endl;
  return r && out;
}

} //namespace vidtk
