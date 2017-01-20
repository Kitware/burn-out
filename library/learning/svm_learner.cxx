/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "svm_learner.h"
#include <svm.h>
#include <iostream>
#include <vnl/vnl_double_2.h>
#include <string>
#include <algorithm>
#include <math.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_svm_cxx__
VIDTK_LOGGER("svm_learner_cxx");

namespace vidtk
{
// implementation class
class svm_learner_impl
{
  public:
    svm_learner_impl();
    ~svm_learner_impl();
    struct svm_problem prob; // used by libsvm for training
    struct svm_model *model; // libsvm's trained model
    struct svm_parameter param; // training parameters
    struct svm_node *x_space;
};

// ------------------------------------
svm_learner_impl::svm_learner_impl()
{
  prob.y = NULL;
  prob.x = NULL;
  x_space = NULL;
}

// ------------------------------------
svm_learner_impl::~svm_learner_impl()
{
  svm_destroy_param(&param);
  if (model)
    svm_destroy_model(model);
  if (prob.y)
    delete [] prob.y;
  if (prob.x)
    delete [] prob.x;
  if (x_space)
    delete [] x_space;
}

// ------------------------------------
svm_learner::svm_learner()
  : impl_(NULL)
{
  impl_ = new svm_learner_impl();
  set_defaults();
}

// ------------------------------------
svm_learner::~svm_learner()
{
  delete impl_;
}

// ------------------------------------
// utility function
bool is_nonzero(double d) {return d != 0;}

// ------------------------------------
///  train a model
void
svm_learner
::train(std::vector<learner_training_data_sptr> const & training_set)
{
  int num_features = training_set[0]->number_of_components(0);
  int num_elements = 0; // non-zero elements
  std::vector<learner_training_data_sptr>::const_iterator it;

  // count non-zero elements
  for(it = training_set.begin();
      it != training_set.end();
      it++)
  {
    vnl_vector<double> v = (*it)->vectorize();
    num_elements += std::count_if(v.begin(), v.end(), is_nonzero);
    num_elements++; // count the '-1' element
  }

  // initialize problem structure (needs to be compatible with libsvm)
  delete [] impl_->prob.y;
  delete [] impl_->prob.x;
  delete [] impl_->x_space;
  impl_->prob.l = static_cast<int>(training_set.size());
  impl_->prob.y = new double[impl_->prob.l];
  impl_->prob.x = new svm_node*[impl_->prob.l];
  impl_->x_space = new svm_node[num_elements];

  // fill in problem structure
  int curr_node = 0;
  for(int i = 0; i < impl_->prob.l; i++)
  {
    vnl_vector<double> v = training_set[i]->vectorize();

    impl_->prob.x[i] = &impl_->x_space[curr_node];
    impl_->prob.y[i] = static_cast<double>(training_set[i]->label());

    for(int j = 0; j < num_features; j++)
    {
      if(v(j) != 0)
      {
        impl_->x_space[curr_node].index = j + 1;
        impl_->x_space[curr_node].value = v(j);
        curr_node++;
      }
    }
    impl_->x_space[curr_node++].index = -1;
  }

  // default gamma if it hasn't been set
  if(impl_->param.gamma == 0 && num_features > 0)
    impl_->param.gamma = 1.0 / num_features;

  // call into libsvm to train the model
  impl_->model = svm_train(&impl_->prob, &impl_->param);
}

// ------------------------------------
/// predict label for a test example
int
svm_learner
::classify(learner_data const & data)
{
  assert(impl_->model != 0);

  vnl_vector<double> v = data.vectorize();
  int num_features = v.size();

  // allocate space for each feature, + 1 for a terminator
  struct svm_node *x;
  x = new svm_node[num_features+1];

  for(int i = 0; i < num_features; i++)
  {
    x[i].index = i+1;
    x[i].value = v(i);
  }

  x[num_features].index = -1;

  // call into libsvm
  double pred_label = svm_predict(impl_->model, x);

  delete [] x;

  return static_cast<int>(pred_label);
}

// ------------------------------------
/// calculate vector of probability estimates (one for each class)
/// for the given test example
void
svm_learner
::probability(learner_data const & data, std::vector<double> & probs)
{
  // make sure the model exists and was trained to calculate probabilties
  if (!supports_probabilities())
  {
    LOG_WARN("We are assuming that a model exists and was trained to support "
             "probabilities. This should be assured externally.");
    return;
  }

  vnl_vector<double> v = data.vectorize();
  int num_features = v.size();

  // allocate space for each feature, + 1 for a terminator
  struct svm_node *x;
  x = new svm_node[num_features+1];

  for(int i = 0; i < num_features; i++)
  {
    x[i].index = i+1;
    x[i].value = v(i);
  }

  x[num_features].index = -1;

  int num_classes = get_num_classes_in_model();
  probs.resize(num_classes);

  // call into libsvm
  svm_predict_probability(impl_->model, x, &probs[0]);

  delete [] x;
}

// ------------------------------------
/// check how many classes are in the trained model
int
svm_learner
::get_num_classes_in_model()
{
  if (!impl_->model)
  {
    return 0;
  }

  // call into libsvm
  return svm_get_nr_class(impl_->model);
}

// ------------------------------------
/// check what labels were used in the model
void
svm_learner
::get_labels_in_model(std::vector<int> & labels)
{
  if (!impl_->model)
  {
    LOG_WARN("We are assuming that a model exists and was trained to support "
             "probabilities. This should be assured externally.");
    return;
  }

  int num_classes = get_num_classes_in_model();
  labels.resize(num_classes);

  // call into libsvm
  svm_get_labels(impl_->model, &labels[0]);
}

// ------------------------------------
/// for compatibility with libsvm, we define separate
/// load/save methods below. This shouldn't be called.
bool
svm_learner
::read(std::istream & /*in*/)
{
  assert(false);
  LOG_ERROR("This method is not implemented. "
            "load_model should be used instead.");
  return false;
}

// ------------------------------------
/// for compatibility with libsvm, we define separate
/// load/save methods below. This shouldn't be called.
bool
svm_learner
::write(std::ostream & /*out*/) const
{
  assert(false);
  LOG_ERROR("This method is not implemented. "
            "save_model should be used instead.");
  return false;
}

// ------------------------------------
/// read in a trained model from the given file
bool
svm_learner
::load_model(const std::string& filename)
{
  // call into libsvm to load model file
  return (impl_->model = svm_load_model(filename.c_str())) != 0;
}

// -------------------------------------
/// save a trained model to the given file
bool
svm_learner
::save_model(const std::string& filename)
{
  if (!impl_->model)
  {
    return false;
  }

  // call into libsvm to save model file
  return !svm_save_model(filename.c_str(), impl_->model);
}

// ------------------------------------
/// check if the model supports probability estimates
bool
svm_learner
::supports_probabilities()
{
  return impl_->model && svm_check_probability_model(impl_->model);
}

// ------------------------------------
/// set svm model default parameters
void
svm_learner::
set_defaults()
{
  // default values
  impl_->param.svm_type = C_SVC;
  impl_->param.kernel_type = RBF;
  impl_->param.degree = 3;
  impl_->param.gamma = 0; // will get defaulted to 1/num_features in training
  impl_->param.coef0 = 0;
  impl_->param.nu = 0.5;
  impl_->param.cache_size = 100;
  impl_->param.C = 1;
  impl_->param.eps = 1e-3;
  impl_->param.p = 0.1;
  impl_->param.shrinking = 1;
  impl_->param.probability = 0;
  impl_->param.nr_weight = 0;
  impl_->param.weight_label = NULL;
  impl_->param.weight = NULL;
  set_verbose(false);
}

// ------------------------------------
/// set svm training options for a linear kernel
void svm_learner::set_training_kernel_linear()
{
  impl_->param.kernel_type = LINEAR;
}

// ------------------------------------
/// set svm training options for a polynomial kernel
void svm_learner::set_training_kernel_poly(double gamma, double coef0, double degree)
{
  impl_->param.kernel_type = POLY;
  impl_->param.gamma = gamma;
  impl_->param.coef0 = coef0;
  impl_->param.degree = degree;
}

// ------------------------------------
/// set svm training options for an RBF kernel
void svm_learner::set_training_kernel_rbf(double gamma)
{
  impl_->param.kernel_type = RBF;
  impl_->param.gamma = gamma;
}

// ------------------------------------
/// set svm training option for cache memory size (in MB)
void svm_learner::set_training_cache_size(double m)
{
  impl_->param.cache_size = m;
}


// ------------------------------------
/// set svm training option for cost parameter
void svm_learner::set_training_cost(double c)
{
  impl_->param.C = c;
}

// ------------------------------------
/// set svm training option for tolerance of termination criterion
void svm_learner::set_training_epsilon(double e)
{
  impl_->param.eps = e;
}

// ------------------------------------
/// whether to train using libsvm's shrinking heuristics
void svm_learner::set_training_shrinking(bool use_shrinking)
{
  impl_->param.shrinking = use_shrinking;
}

// ------------------------------------
/// whether to train model to support probability estimates
void svm_learner::set_training_use_probabilities(bool use_probs)
{
  impl_->param.probability = use_probs;
}

// ------------------------------------
// to allow suppressing output from libsvm
void print_null(const char * /*s*/) {}

// send output to LOG_DEBUG
void print_debug(const char * s)
{
  LOG_DEBUG(s);
}

// control where output goes
void svm_learner::set_verbose(bool show_output)
{
  void (*print_func)(const char*);

  if (show_output)
  {
    print_func = &print_debug;
  }
  else
  {
    print_func = &print_null;
  }

  // set where libsvm output goes
  svm_set_print_string_function(print_func);
}

} //namespace vidtk
