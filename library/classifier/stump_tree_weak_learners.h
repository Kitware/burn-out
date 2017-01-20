/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_stump_tree_weak_learners_h_

#include <learning/learner_base.h>
#include <learning/weak_learner.h>

#include <sstream>

namespace vidtk
{

///A simple stump tree learner

class stump_tree_weak_learner : public weak_learner
{
  protected:
    struct node;
  public:
    stump_tree_weak_learner( std::string const & name, unsigned d, int desc )
      : weak_learner(name, desc), sign_(1)
    {}
    stump_tree_weak_learner( );
    ~stump_tree_weak_learner();
    virtual weak_learner_sptr clone() const
    { return new stump_tree_weak_learners(*this); }
    virtual weak_learner_sptr
      train( training_feature_set const & datas,
             vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data);
/*    virtual std::string gplot_command() const;*/
    virtual weak_learners::learner get_id() const
    { return weak_learners::stump_tree_weak_learner; }
    virtual bool read(std::istream & in);
    virtual bool write(std::ostream & out) const;
    virtual std::string unique_id() const
    {
      std::stringstream ss;
      ss << name_ << "_dem_" << dim_;
      std::string r;
      ss >> r;
      return r;
    }
    virtual double get_debug_value() const
    { return value_; }
  protected:
    unsigned int dim_;

    struct value_weight;
    node * build(std::vector<value_weight> const & vals,
                 unsigned int d,
                 unsigned int begin, unsigned int end,
                 double & score);
    struct node
    {
      node() : left_(NULL),right_(NULL), sign_(0)
      {}
      node( double v, double s );
      ~node(){ delete left_; delete right_; }
      int classify(double v);
      bool write(std::ostream & out) const;
      bool read(std::istream & in);
      double value_;
      double sign_;
      node * left_, * right_;
    }
    stump_tree_weak_learner( node * r );
    node * root_;
    unsigned int max_depth_;
    double min_number_leaf_;
};

} //namespace vidtk

#endif
