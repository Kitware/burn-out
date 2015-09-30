/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//training_feature_set.h
#ifndef vidtk_training_feature_set_h_
#define vidtk_training_feature_set_h_

#include <learning/learner_data.h>

#include <vcl_vector.h>

namespace vidtk
{

class training_feature_set
{
  public:
    training_feature_set(vcl_vector< learner_training_data_sptr > samples);
    double operator()(unsigned int desc, unsigned int component, unsigned int sample) const;
    learner_training_data_sptr operator[](unsigned int i) const
    { return data_[i]; }
    unsigned int get_location(unsigned int desc, unsigned int component, unsigned int sample) const;
    unsigned int size() const;
    void sort();
  protected:
    bool is_sorted_;
    vcl_vector< learner_training_data_sptr > data_;
    vcl_vector< vcl_vector< vcl_vector< unsigned int > > > mapping_;
};

}

#endif
