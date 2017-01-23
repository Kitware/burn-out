/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "stump_tree_weak_learners.h"

#include <cassert>
#include <iostream>
#include <algorithm>

#include <vnl/vnl_math.h>

#include <logger/logger.h>

VIDTK_LOGGER("stump_tree_weak_learners_cxx");


namespace vidtk
{

stump_tree_weak_learner
::stump_tree_weak_learner( int s, unsigned int d, double v,
                           std::string const & name, int desc )
      : weak_learner(name, desc),
        sign_(s), dim_(d), value_(v),
        root_(NULL), max_depth_(3), min_number_leaf_(1.0/10.)
{
}

stump_tree_weak_learner
::stump_tree_weak_learner( )
  : root_(NULL), max_depth_(3), min_number_leaf_(1.0/10.)
{
}

stump_tree_weak_learner
::~stump_tree_weak_learner( )
{
  if(root_)
  {
    delete root_;
  }
}

weak_learner_sptr
stump_tree_weak_learner
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
  return (weak_learner *)(new stump_tree_weak_learner( best_sign, best_dim,
                                                       best_value, name_,descriptor_) );
}

int
stump_tree_weak_learner
::classify(learner_data const & data)
{
  double v = data.get_value_at(descriptor_,dim_);
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

node *
stump_tree_weak_learner
::build( std::vector<value_weight> const & vals,
         unsigned int d,
         unsigned int begin, unsigned int end,
         double & score)
{
  score = 1e23;
  if(end <= begin )
  {
    return NULL;
  }
  if(d >= max_depth_ )
  {
    return NULL;
  }
  if(end-begin < 2)
  {
    return NULL;
  }
  double pos_weight_sum = 0.0;
  double neg_weight_sum = 0.0;
  double pos_weight_sum_lte_split = 0.0;
  double neg_weight_sum_lte_split = 0.0;
  for( unsigned int i = beign; i < end; ++i )
  {
    pos_weight_sum += vals[i].pos_w;
    neg_weight_sum += vals[i].neg_w;
  }
  if(pos_weight_sum + neg_weight_sum < min_number_leaf_)
  {
    return NULL;
  }
  double best_split_v = 0.0;
  int best_sign = 2;
  double best_l = 0.0;
  double best_r = 0.0;
  double best_score = 0.0;
  unsigned int best_ip1;

  for(unsigned int i = begin; i < end-1; ++i)
  {

    if(!(vals[i].v < vals[i+1].v))
    {
      LOG_INFO( i << " " << i+1 << " " << vals[i].v << " " << vals[i+1].v << "\t" << vals[i+1].v-vals[i].v );
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
    double temp_l = 0.0;
    double temp_r = 0.0;
    double tmp_score = 0.0;
    if( neg_weight_sum_lte_split+pos_weight_sum_gt_split >= pos_weight_sum_lte_split+neg_weight_sum_gt_split)
    {
      temp_sign = 1;
      temp_l = pos_weight_sum_lte_split;
      temp_r = neg_weight_sum_gt_split;
      temp_score = pos_weight_sum_lte_split+neg_weight_sum_gt_split;
    }
    else
    {
      temp_sign = -1;
      temp_r = pos_weight_sum_gt_split;
      temp_l = neg_weight_sum_lte_split;
      temp_score = neg_weight_sum_lte_split+pos_weight_sum_gt_split;
    }

    if(temp_score < best_score || best_sign == 2)
    {
      best_score = temp_score;
      best_split_v = temp_split;
      best_sign = temp_sign;
      best_ip1 = i+1;
      best_l = temp_l;
      best_r = temp_r;
    }
  }
  double score_left;
  double score_right;
  score = 0;
  node * n = new node(best_split_v,best_sign);
  node * l = build( vals, d + 1, begin, best_ip1, score_left);
  node * r = build( vals, d + 1, best_ip1, end, score_right);
  if ( score_left < best_l )
  {
    score += score_left;
    n.left_ = l;
  }
  else
  {
    delete l;
    score += best_l;
    n.left_ = NULL;
  }
  if ( score_right < best_r )
  {
    score += score_right;
    n.right_ = r;
  }
  else
  {
    delete r;
    score += best_r;
    n.right_ = NULL;
  }
  return n;
}

stump_tree_weak_learner::node
::node( double v, double s )
  : value_(v), sign_(s)
{
}

int
stump_tree_weak_learner::node
::classify(double v)
{
  if( v <= value_ )
  {
    if(left_)
    {
      return left_->classify(v);
    }
    else
    {
      return -sign_;
    }
  }
  else
  {
    if(right_)
    {
      return right_->classify(v);
    }
    else
    {
      return sign_;
    }
  }
}

bool
stump_tree_weak_learner::node
::write(std::ostream & out) const
{
  out << sign_ << " " << value_;
  if(left_)
  {
    out << " 1 ";
    left_->write()
  }
  else
  {
    out << " 0";
  }
  if(right_)
  {
    out << " 2 ";
    right_->write()
  }
  else
  {
    out << " 0";
  }
  return out;
}

bool
stump_tree_weak_learner::node
::read(std::istream & in)
{
  int n;
  in >> sign_ >> value_;
  in >> n;
  if(n)
  {
    left_ = new node();
    left_->read(in);
  }
  in >> n;
  if(n)
  {
    right_ = new node();
    right_->read(in);
  }
  return in;
}

bool
stump_tree_weak_learner
::read(std::istream & in)
{
  bool r = weak_learner::read(in);
  int re;
  in >> sign_ >> re;
  if(re)
  {
    root = new node();
    root->read(in);
  }
  return r;
}

bool
stump_tree_weak_learner
::write(std::ostream & out) const
{
  bool r = weak_learner::write(out);
  out << dim_;
  if(root_)
  {
    out << " 1 ";
    r = r & root_->write(out);
  }
  else
  {
    out << " 0";
  }
  out << std::endl;
  return r && out;
}

} //namespace vidtk
