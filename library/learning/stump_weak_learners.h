/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_stump_weak_learners_h_

#include <learning/learner_base.h>
#include <learning/weak_learner.h>

#include <vcl_sstream.h>

namespace vidtk
{

///A simple stump learner

class stump_weak_learner : public weak_learner
{
  public:
    stump_weak_learner( vcl_string const & name, int desc )
      : weak_learner(name, desc), sign_(1), value_(0)
    {}
    stump_weak_learner( )
    {}
    stump_weak_learner( int s, unsigned int d, double v,
                        vcl_string const & name, int desc );
    virtual weak_learner_sptr clone() const
    { return new stump_weak_learner(*this); }
    virtual weak_learner_sptr
      train( training_feature_set const & datas,
             vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data);
/*    virtual vcl_string gplot_command() const;*/
    virtual weak_learners::learner get_id() const
    { return weak_learners::stump_weak_learner; }
    virtual bool read(vcl_istream & in);
    virtual bool write(vcl_ostream & out) const;
    virtual vcl_string unique_id() const
    {
      vcl_stringstream ss;
      ss << name_ << "_dem_" << dim_;
      vcl_string r;
      ss >> r;
      return r;
    }
    virtual double get_debug_value() const
    { return value_; }
  protected:
    int sign_;
    unsigned int dim_;
    double value_;

    ///Returns the error for the calculate signed and value
    double calculate_singed( training_feature_set const & datas,
                             vnl_vector<double> const & weights,
                             unsigned int d,
                             int & s,
                             double & v);
};

} //namespace vidtk

#endif
