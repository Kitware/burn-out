/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_stump_weak_learners_h_

#include <learning/learner_base.h>
#include <learning/weak_learner.h>

#include <sstream>

namespace vidtk
{

///A simple stump learner

class stump_weak_learner : public weak_learner
{
  public:
    stump_weak_learner( std::string const & _name, int desc )
      : weak_learner(_name, desc), sign_(1), value_(0)
    {}
    stump_weak_learner( )
    {}
    stump_weak_learner( int s, unsigned int d, double v,
                        std::string const & name, int desc );
    virtual weak_learner_sptr clone() const
    { return new stump_weak_learner(*this); }
    virtual weak_learner_sptr
      train( training_feature_set const & datas,
             vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data);
/*    virtual std::string gplot_command() const;*/
    virtual weak_learners::learner get_id() const
    { return weak_learners::stump_weak_learner; }
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
