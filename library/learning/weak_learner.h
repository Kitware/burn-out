/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_weak_learner_h_
#define vidtk_weak_learner_h_

#include <learning/learner_base.h>

namespace vidtk
{

///Weak learner ids
namespace weak_learners
{
  enum learner
  {
    stump_weak_learner = 0,
    histogram_weak_learner,
    weak_learner_gausian,
    weak_learner_single_gausian,
    weak_learner_single_hw_gausian,
    linear_weak_learner,
    tree_weak_learner
  };
};

class weak_learner;
typedef vbl_smart_ptr<weak_learner> weak_learner_sptr;

///Base class for adaboost weak learners
class weak_learner : public learner_base
{
  public:
    weak_learner( std::string const & _name, int desc )
      : name_(_name), descriptor_(desc) { }
    weak_learner( ) {}
    virtual weak_learner_sptr clone() const = 0;
    /// Creates and trains a new instance of the weak learner
    virtual weak_learner_sptr train( training_feature_set const &,
                                                 vnl_vector<double> const & ) = 0;
    virtual int classify(learner_data const &) = 0;
    /// Returns 0 or 1 instead of the normal -1 1.  This is useful for the Platt Score stuff
    virtual int classify01(learner_data const & data)
    {
      return ((this->classify(data) == 1)?1:0);
    }
    std::string const & name() const{ return name_; }
    ///Useful in debuging.  Please note that it is assumed if the string constains
    ///useful information that it will have the endline in it.
    virtual std::string gplot_command() const
    {
      std::string r("");
      return r;
    }
    ///Function useful for debuging
    virtual void debug(unsigned int /*i*/) const
    {
    }
    virtual bool read(std::istream & in)
    {
      in >> name_;
      in >> descriptor_;
      return true;
    }
    virtual bool write(std::ostream & out) const
    {
      out << name_ << " " << descriptor_ << std::endl;
      return true;
    }
    virtual weak_learners::learner get_id() const = 0;
    virtual std::string unique_id() const
    {
      return "";
    }
    virtual double get_debug_value() const
    { return 0.; }
  protected:
    ///Name of the learner
    std::string name_;
    ///Id number of the descritor this learner uses
    int descriptor_;
};

} //namespace vidtk

#endif
