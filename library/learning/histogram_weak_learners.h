/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_histogram_weak_learners_h_
#define vidtk_histogram_weak_learners_h_

#include <learning/weak_learner.h>

#include <vnl/vnl_vector.h>

namespace vidtk
{

///Models the classes useing a histogram on the chosen axis.
class histogram_weak_learner : public weak_learner
{
  public:
    histogram_weak_learner( unsigned int axis, std::string const & _name, int desc )
      : weak_learner(_name, desc), axis_(axis)
    {}
    histogram_weak_learner( )
    {}
    virtual weak_learner_sptr clone() const
    { return new histogram_weak_learner(*this); }
    virtual weak_learner_sptr train( training_feature_set const & datas,
                                     vnl_vector<double> const & weights );
    virtual int classify(learner_data const & data);
    virtual void debug(unsigned int i) const;
    virtual weak_learners::learner get_id() const
    { return  weak_learners::histogram_weak_learner; }
    virtual bool read(std::istream & in);
    virtual bool write(std::ostream & out) const;
  protected:
    std::vector<char> bin_class_;
//     vnl_vector<double> histogram_positive_, histogram_negative_;
    double min_, max_;
    unsigned int axis_;
    unsigned int bin(double v) const
    {
      if(v<min_) return 0;
      if(v>=max_) return 999;
      return static_cast<unsigned int>((v-min_)/(max_-min_)*1000);
    }
    double smoothed_value(int at, vnl_vector<double> & hist);
};

///Models the classes useing a histogram on the chosen axis.
class histogram_weak_learner_inSTD : public histogram_weak_learner
{
  public:
    histogram_weak_learner_inSTD( unsigned int axis, std::string const & _name, int desc )
      : histogram_weak_learner( axis, _name, desc)
    {}
    histogram_weak_learner_inSTD( )
    {}
    virtual weak_learner_sptr clone() const
    { return new histogram_weak_learner_inSTD(*this); }
    virtual weak_learner_sptr train( training_feature_set const & datas,
                                     vnl_vector<double> const & weights );
  protected:
};

} //namespace vidtk

#endif
