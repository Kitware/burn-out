/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "training_feature_set.h"

#include <algorithm>

#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>

#include <vnl/vnl_math.h>

struct training_set_value_pointer_struct
{
  training_set_value_pointer_struct( unsigned int d, unsigned int a,
                                     vidtk::learner_training_data_sptr v,
                                     unsigned int l )
    : desc(d), at(a), value(v), loc(l)
  {}
  unsigned int desc, at;
  vidtk::learner_training_data_sptr value;
  unsigned int loc;
  double get() const
  { return value->get_value_at(desc,at); }
};

bool operator<( training_set_value_pointer_struct const & l,
                training_set_value_pointer_struct const & r)
{
  return l.get()<r.get();
}

namespace vidtk
{

void
training_feature_set
::sort()
{
  for(unsigned int d = 0; d < this->mapping_.size(); ++d)
  {
    for(unsigned int c = 0; c < this->mapping_[d].size(); ++c)
    {
      std::vector<training_set_value_pointer_struct> t;
      t.reserve(this->mapping_[d][c].size());
      for(unsigned int i = 0; i < this->data_.size(); ++i)
      {
        t.push_back( training_set_value_pointer_struct( d, c,
                                                        this->data_[this->mapping_[d][c][i]],
                                                        i ) );
      }
      std::sort( t.begin(), t.end() );
      for(unsigned int i = 0; i < this->data_.size(); ++i)
      {
        this->mapping_[d][c][i] = t[i].loc;
      }
    }
  }
}

double
training_feature_set
::operator()(unsigned int desc, unsigned int component, unsigned int sample) const
{
  return this->data_[this->mapping_[desc][component][sample]]->get_value_at(desc,component);
}

unsigned int
training_feature_set
::get_location(unsigned int desc, unsigned int component, unsigned int sample) const
{
  return this->mapping_[desc][component][sample];
}

unsigned int
training_feature_set
::size() const
{
  return this->data_.size();
}

training_feature_set
::training_feature_set(std::vector< learner_training_data_sptr > samples)
  : data_(samples)
{
  assert(samples.size());
  assert(samples[0]);
  unsigned int numberOfDesc = samples[0]->number_of_descriptors();
  this->mapping_.resize(numberOfDesc);
  std::vector<unsigned int> t(this->data_.size());
  for(unsigned int i = 0; i < this->data_.size(); ++i)
  {
    t[i] = i;
  }
  for(unsigned int d = 0; d < numberOfDesc; ++d)
  {
    unsigned int numberOfComponents = samples[0]->number_of_components(d);
    this->mapping_[d].resize(numberOfComponents);
    for(unsigned int c = 0; c < numberOfComponents; ++c)
    {
      this->mapping_[d][c] = t;
    }
  }
  this->sort();
}

}//namespace vidtk
