/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tree_weak_learners_h_
#define vidtk_tree_weak_learners_h_

#include <learning/learner_base.h>
#include <learning/weak_learner.h>

#include <sstream>

namespace vidtk
{

/// \brief A simple tree of stumps weak learner class.
class tree_weak_learner : public weak_learner
{
  public:

    tree_weak_learner( std::string const & _name, int desc );
    tree_weak_learner() {}

    tree_weak_learner( int s, unsigned int d, double v,
                        std::string const & name, int desc );

    virtual weak_learner_sptr clone() const
    {
      return new tree_weak_learner(*this);
    }

    virtual weak_learner_sptr
      train( training_feature_set const & datas,
             vnl_vector<double> const & weights );

    virtual int classify(learner_data const & data);

    virtual weak_learners::learner get_id() const
    {
      return weak_learners::tree_weak_learner;
    }

    virtual bool read(std::istream & in);
    virtual bool write(std::ostream & out) const;

    virtual std::string unique_id() const;

    virtual double get_debug_value() const
    {
      return 0.0;
    }

  protected:

    struct boundary
    {
      bool sign_;
      unsigned int dim_;
      double thresh_;
    };

    std::vector< boundary > tree_;

};

} //namespace vidtk

#endif
