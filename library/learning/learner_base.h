/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_learner_base_h_
#define vidtk_learner_base_h_

#include <learning/learner_data.h>
#include <learning/training_feature_set.h>

#include <vector>
#include <string>
#include <iostream>

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

namespace vidtk
{

///The base classifier class used in adaboost
class learner_base : public vbl_ref_count
{
  public:
    /// Calculates the error rate of a classifier
    static double error_rate( learner_base * cl,
                              training_feature_set const & datas,
                              vnl_vector<double> const & wgts );
    /// Returns either -1 or 1 depending on what class the data falls in
    virtual int classify(learner_data const & data) = 0;
    /// Read and write the classifier
    virtual bool read(std::istream &) = 0;
    virtual bool write(std::ostream &) const = 0;
};

} //namespace vidtk

#endif
