/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_learner_data_class_single_vector_h
#define vidtk_learner_data_class_single_vector_h

#include <learning/learner_data.h>

namespace vidtk
{

///Data store for data with a signle descriptor
class value_learner_data : public learner_training_data
{
  public:
    value_learner_data() : learner_training_data(1) {}
    value_learner_data( vnl_vector<double> const & v, int l = 1 )
      : learner_training_data(l), data_(v)
    {}
    value_learner_data( double v, int l = 1 ) : learner_training_data(l), data_(1,v)
    {}
    virtual vnl_vector<double> get_value( int ) const
    { return data_; }
    virtual double get_value_at(int, unsigned int loc) const
    { return data_[loc]; }
    virtual unsigned int number_of_descriptors() const
    { return 1; }
    virtual unsigned int number_of_components(unsigned int) const
    { return data_.size(); }
    virtual vnl_vector<double> vectorize() const
    { return data_; }
    virtual void write(std::ostream& os) const
    {
        os << data_;
    }
  protected:
    vnl_vector<double> data_;
};

}

#endif
