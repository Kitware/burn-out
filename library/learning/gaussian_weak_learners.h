/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gaussian_weak_learners_h_
#define vidtk_gaussian_weak_learners_h_

#include <learning/weak_learner.h>

#include <vpdl/vpdl_gaussian.h>

namespace vidtk
{

///Classifies the data using two gaussians
class weak_learner_gausian : public weak_learner
{
  public:
    weak_learner_gausian( vcl_string const & name, int desc ) : weak_learner(name, desc)
    {}
    weak_learner_gausian( )
    {}
    virtual weak_learner_sptr clone() const
    { return new weak_learner_gausian(*this); }
    virtual weak_learner_sptr train( training_feature_set const & datas,
                                     vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data)
    {
      vnl_vector<double> v = data.get_value(descriptor_);
      double gc = gaus_class_.density(v) * gaus_class_.norm_const();
      double gn = gaus_not_.density(v) * gaus_not_.norm_const();
      if(gc >= gn) return 1;
      return -1;
    }
    virtual weak_learners::learner get_id() const
    { return weak_learners::weak_learner_gausian; }
    virtual bool read(vcl_istream & in);
    virtual bool write(vcl_ostream & out) const;
  protected:
    vpdl_gaussian< double, 0 > gaus_class_;
    vpdl_gaussian< double, 0 > gaus_not_;
};

///Classifies the postitive numbers using a gaussian
class weak_learner_single_gausian : public weak_learner
{
  public:
    weak_learner_single_gausian( vcl_string const & name, int desc )
      : weak_learner(name, desc)
    {}
    weak_learner_single_gausian( )
    {}
    virtual weak_learner_sptr clone() const
    { return new weak_learner_single_gausian(*this); }
    virtual weak_learner_sptr
    train( training_feature_set const & datas,
           vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data)
    {
      vnl_vector<double> v = data.get_value(descriptor_);
      double gc = gaus_class_.sqr_mahal_dist(v);
      if(gc<=9.0) return 1;
      return -1;
    }
    virtual weak_learners::learner get_id() const
    { return weak_learners::weak_learner_single_gausian; }
    virtual bool read(vcl_istream & in);
    virtual bool write(vcl_ostream & out) const;
  protected:
    vpdl_gaussian< double, 0 > gaus_class_;
};

///Picks which class to model based on which one has the larest weight
class weak_learner_single_hw_gausian : public weak_learner
{
  public:
    weak_learner_single_hw_gausian( vcl_string const & name, int desc )
      : weak_learner(name, desc)
    {}
    weak_learner_single_hw_gausian( )
    {}
    virtual weak_learner_sptr clone() const
    { return new weak_learner_single_hw_gausian(*this); }
    virtual weak_learner_sptr
    train( training_feature_set const & datas,
           vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data)
    {
      vnl_vector<double> v = data.get_value(descriptor_);
      double gc = gaus_.sqr_mahal_dist(v);
      if(gc<=9.0) return sign_;
      return -sign_;
    }
    virtual weak_learners::learner get_id() const
    { return weak_learners::weak_learner_single_hw_gausian; }
    virtual bool read(vcl_istream & in);
    virtual bool write(vcl_ostream & out) const;
  protected:
    vpdl_gaussian< double, 0 > gaus_;
    int sign_;
};

} //namespace vidtk

#endif
