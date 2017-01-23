/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//training_feature_set.h
#ifndef vidtk_training_feature_set_h_
#define vidtk_training_feature_set_h_

#include <learning/learner_data.h>

#include <vector>

namespace vidtk
{

class training_feature_set
{
  public:
    training_feature_set(std::vector< learner_training_data_sptr > samples);
    double operator()(unsigned int desc, unsigned int component, unsigned int sample) const;
    learner_training_data_sptr operator[](unsigned int i) const
    { return data_[i]; }
    unsigned int get_location(unsigned int desc, unsigned int component, unsigned int sample) const;
    unsigned int size() const;
    void sort();
  protected:
    bool is_sorted_;
    std::vector< learner_training_data_sptr > data_;
    std::vector< std::vector< std::vector< unsigned int > > > mapping_;
};

}

#endif
