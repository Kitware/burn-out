/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_adaboost_h_
#define vidtk_adaboost_h_

#include <vnl/vnl_vector.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_double_2.h>

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include <learning/learner_base.h>
#include <learning/training_feature_set.h>
#include <learning/weak_learner.h>

extern std::ofstream out_1111;

namespace vidtk
{


/// \brief The adaboost classifier class.
///
/// This classifier can take multiple different weak learners.  Each weak classifiers
/// can focus either on a specific descriptor, or multiple ones. The algorithm learning
/// portion of the algorithm automatically selects the best one based its error rate.
class adaboost : public learner_base
{
  public:

    /// Construct a classifier object, giving this class a list of weak learner
    /// variants to use, in addition to a maximum weak learner count.
    adaboost( std::vector< weak_learner_sptr > const & weak_learner_factories,
              unsigned int max = 100 );

    adaboost() {};
    ~adaboost();

    /// Trains the adaboost algorithm based on the provided examples using the
    /// default algorithm.
    void train( std::vector< learner_training_data_sptr > const & datas );

    /// Trains the adaboost algorithm based on the provided examples using the
    /// Platt algorithm.
    void train_platt( std::vector< learner_training_data_sptr > const & datas );

    /// Classifies data based on the internal set of weak learner, returning a
    /// binary decision {-1, 1} based on thresholding the classifier output.
    virtual int classify( learner_data const & data );

    /// Classifies data based on the internal set of weak learner, returning a
    /// binary decision {-1, 1} based on thresholding the classifier output.
    virtual int classify( learner_data const & data, unsigned int use_up_to );

    /// Classifies data based on the internal set of weak learner, returning a
    /// raw decesion weight as computed by the internal model.
    virtual double classify_raw( learner_data const & data );

    /// Returns platt scaled value.
    virtual double platt_probability( learner_data const & data );

    /// Returns number of weak learners in this classifier.
    unsigned int number_of_weak_learners() const
    {
      return learners_.size();
    }

    /// Parse an adaboost model given a stream operate.
    virtual bool read( std::istream & in );

    /// Reads an adaboost model from a file.
    virtual bool read_from_file( const std::string & fn );

    /// Write an adaboost model to an output stream.
    virtual bool write( std::ostream & out ) const;

    double f_x( learner_data const & data );

    /// Calculate various internal statistics.
    void calculate_stats( std::map< std::string, unsigned int > & weak_learner_counts,
                          std::map< std::string, vnl_double_2 > & average_value,
                          std::map< std::string, vnl_double_2 > & average_importance,
                          std::map< std::string, vnl_double_2 > & average_number_of_times );

    /// Has a valid classifier been loaded?
    bool is_valid() const;

  protected:

    // Updates the distribution based on the new weak classifier and previous weights
    void update_distribution( weak_learner * cl,
                              training_feature_set const & datas,
                              double alpha_t,
                              vnl_vector<double> & wgts ) const;

    // The set of weak learner it can choose from to create a new one for used in
    // classification
    std::vector<weak_learner_sptr> weak_learner_factories_;

    // The set of weak learner used when classifying
    std::vector<weak_learner_sptr> learners_;

    // The weights used in voting when classifying
    std::vector<double> weights_;
    unsigned int max_number_of_iterations_;
    double platt_A_, platt_B_;
    bool platt_trained_;
};


/// Adaboost smart pointer definition.
typedef vbl_smart_ptr< adaboost > adaboost_sptr;


}//namespace vidtk

#endif
