/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_svm_h_
#define vidtk_svm_h_

#include <vnl/vnl_vector.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_double_2.h>

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include <learning/learner_base.h>
#include <learning/learner_data_class_single_vector.h>

namespace vidtk
{
// implementation class to hide libsvm structures
class svm_learner_impl;

/// The svm classifier class
/// This classifier wraps the libsvm library to provide basic functionality for
/// support vector machines.
class svm_learner : public learner_base
{
  public:
    svm_learner();
    ~svm_learner();
    /// Trains an svm model based on the provided examples
    void train(std::vector<learner_training_data_sptr> const & training_set);
    /// Classifies based on the learned model
    virtual int classify(learner_data const & data);
    /// Returns probability estimates for classes
    void probability(learner_data const & data, std::vector<double> & probs);
    /// Read and write
    virtual bool read(std::istream & in);
    virtual bool write(std::ostream & out) const;
    /// load model from file
    bool load_model(const std::string & filename);
    /// Check if model includes info to estimate probabilities
    bool supports_probabilities();
    bool save_model(const std::string & filname);
    void set_verbose(bool show_output); // to allow suppressing output from libsvm
    int get_num_classes_in_model();
    void get_labels_in_model(std::vector<int> & labels);

    /// Training parameters
    void set_training_kernel_linear();
    void set_training_kernel_poly(double gamma, double coef0, double degree);
    void set_training_kernel_rbf(double gamma);
    void set_training_cost(double c);
    void set_training_epsilon(double e);
    void set_training_cache_size(double m);
    void set_training_shrinking(bool use_shrinking);
    void set_training_use_probabilities(bool use_probs);

  protected:
    void set_defaults();
    svm_learner_impl *impl_;
};

}//namespace vidtk

#endif
