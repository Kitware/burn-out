/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_learner_data_base_h_
#define vidtk_learner_data_base_h_

#include <vnl/vnl_vector.h>

#include <cassert>
#include <ostream>

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

#include <vnl/vnl_vector.h>

namespace vidtk
{

///Base class for the adaboost data
class learner_data : public vbl_ref_count
{
  public:
    ///Returns the value of the for descriptor.
    virtual vnl_vector<double> get_value( int ) const = 0;
    virtual double get_value_at(int desc, unsigned int loc) const = 0;
    virtual unsigned int number_of_descriptors() const = 0;
    virtual unsigned int number_of_components(unsigned int d) const = 0;
    virtual vnl_vector<double> vectorize() const = 0;
    virtual void write(std::ostream& os) const = 0;
};

///Base class of the training data
class learner_training_data : public learner_data
{
  public:
    learner_training_data( int lbl ) : label_(lbl)
    {
    }
    /// Returns the class of the ground truth
    virtual int label() const
    { return label_; }
    /// Sets the class of the ground truth.
    virtual void set_label( int lbl )
    {
      label_ = lbl;
    }
  protected:
    int label_;
};

typedef vbl_smart_ptr<learner_training_data> learner_training_data_sptr;
typedef vbl_smart_ptr<learner_data> learner_data_sptr;

}

std::ostream& operator<< (std::ostream& os, const vidtk::learner_data &p);

std::ostream& operator<< (std::ostream& os, const vidtk::learner_training_data &p);

#endif
