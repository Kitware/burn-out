/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_learner_data_class_vector_h_
#define vidtk_learner_data_class_vector_h_

#include <learning/learner_data.h>

namespace vidtk
{

///A data store for data with multiple descriptors
class vectors_of_descriptors_learner_data : public learner_training_data
{
  public:
    vectors_of_descriptors_learner_data() : learner_training_data(1) {}
    vectors_of_descriptors_learner_data( std::vector< vnl_vector<double> > const & v, int l = 1 )
      : learner_training_data(l), data_(v)
    {}
    virtual vnl_vector<double> get_value( int i ) const
    { assert( static_cast<unsigned int>(i) < data_.size()); return data_[i]; }
    virtual double get_value_at(int desc, unsigned int loc) const
    { assert( static_cast<unsigned int>(desc) < data_.size()); return data_[desc][loc]; }
    virtual unsigned int number_of_descriptors() const
    { return data_.size(); }
    virtual unsigned int number_of_components(unsigned int d) const
    { assert(d < data_.size()); return data_[d].size(); }
    virtual vnl_vector<double> vectorize() const
    {
      unsigned int size = 0;
      for( unsigned int i = 0; i < data_.size(); ++i )
      {
        size += data_[i].size();
      }
      vnl_vector<double> result(size);
      unsigned int at = 0;
      for( unsigned int i = 0; i < data_.size(); ++i )
      {
        for( unsigned int j = 0; j < data_[i].size(); ++j )
        {
          result[at++] = data_[i][j];
        }
      }
      return result;
    }
    virtual void write(std::ostream& os) const
    {
      for(unsigned int i = 0; i < data_.size(); ++i)
      {
        os << " ([" << i << "] " << data_[i] << ")";
      }
    }
  protected:
    std::vector< vnl_vector<double> > data_;
};

} //namespace vidtk

#endif
