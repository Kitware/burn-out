/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_adaboost_h_
#define vidtk_adaboost_h_

#include <vnl/vnl_vector.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_double_2.h>

#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_sstream.h>
#include <vcl_iostream.h>
#include <vcl_map.h>

#include <learning/learner_base.h>
#include <learning/training_feature_set.h>
#include <learning/weak_learner.h>

extern vcl_ofstream out_1111;

namespace vidtk
{

///The adaboost classifier class
///This classifier can take multiple different weak learner.  Each of these
///week classifier could focus on a specific descriptor.
///The algorithm automatically selects the best one based its error rate.
class adaboost : public learner_base
{
  public:
    adaboost( vcl_vector< weak_learner_sptr > const & weak_learner_factories,
              unsigned int max = 100 );
    adaboost(){};
    ~adaboost();
    ///Trains the adaboost algorithm based on the provided examples
    void train(vcl_vector<learner_training_data_sptr> const & datas);
    void trainPlatt(vcl_vector<learner_training_data_sptr> const & datas);
    ///Classifies based on the set of learned weak learner
    virtual int classify(learner_data const & data);
    virtual int classify(learner_data const & data,unsigned int use_upto);
    ///Returns Platt Scaled Value
    virtual double probability(learner_data const & data);
    ///Read and write
    virtual bool read(vcl_istream & in);
    virtual bool write(vcl_ostream & out) const;
    unsigned int number_of_weak_learners() const
    { return learners_.size(); }
    double f_x(learner_data const & data);
    void calculate_stats( vcl_map< vcl_string, unsigned int > & weak_learner_counts,
                          vcl_map< vcl_string, vnl_double_2 > & average_value,
                          vcl_map< vcl_string, vnl_double_2 > & average_importance,
                          vcl_map< vcl_string, vnl_double_2 > & average_number_of_times );
  protected:
    ///Updates the distribution based on the new weak classifier and previous weights
    void update_distribution( weak_learner * cl,
                              training_feature_set const & datas,
                              double alpha_t,
                              vnl_vector<double> & wgts ) const;
    ///The set of weak learner it can choose from to create a new one for used in
    ///classification
    vcl_vector<weak_learner_sptr> weak_learner_factories_;
    ///The set of weak learner used when classifying
    vcl_vector<weak_learner_sptr> learners_;
    ///The weights used in voting when classifying
    vcl_vector<double> weights_;
    unsigned int max_number_of_iterations_;
    double platt_A_, platt_B_;
    bool platt_trained_;
};

}//namespace vidtk

#endif
