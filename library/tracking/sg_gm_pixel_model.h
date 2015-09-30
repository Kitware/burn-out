/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sg_gm_pixel_model_h_
#define vidtk_sg_gm_pixel_model_h_

#include <vcl_vector.h>
#include <vcl_cmath.h>
#include <vcl_iosfwd.h>
#include <vcl_string.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_math.h>

namespace vidtk
{

/// \brief The parameters governing the SG pixel model.
///
/// \sa sg_gm_pixel_model
struct sg_gm_pixel_model_params
{
  sg_gm_pixel_model_params() :
    mahan_dist_sqr_thres_( 0.0 ),
    initial_variance_( 1.0 ),
    initial_weight_( 0.0 ),
    alpha_weights_(0.),
    alpha_means_(0.),
    alpha_variance_(0.),
    learning_alpha_weights_(0.),
    learning_alpha_means_(0.),
    learning_alpha_variance_(0.),
    rho_mean_(0.),
    rho_variance_(0.),
    order_by_weight_only_( false ),
    background_thres_( 0.0 ),
    num_pix_comp_( 0 ),
    num_mixture_comp_( 0 ),
    min_samples_for_detection_( 0 ),
    no_update_threshold_( 0 ),
    take_first_matched_(false)
  {

  }


  /// The threshold used to determine if a value belongs to a
  /// component.  It is compared against the sqaure of the Mahanalobis
  /// distance.
  float mahan_dist_sqr_thres_;

  /// The initial variance used for a new component.
  float initial_variance_;

  /// The initial weight used for a new component.
  float initial_weight_;

  /// The learning rate for the weights.
  float alpha_weights_;
  /// The learning rate for the means.
  float alpha_means_;
  ///The learning rate for the variance
  float alpha_variance_;
  /// The learning rate for the weights for learning mode.
  float learning_alpha_weights_;
  /// The learning rate for the means for learning mode.
  float learning_alpha_means_;
  ///The learning rate for the variance for learning mode
  float learning_alpha_variance_;

  /// If this is positive, we use this constant learning rate to
  /// update the mean and variance.  If it is zero, we compute the
  /// data dependent rate as alpha*N(x|mu,sigma). If it is \c -1, we
  /// use the value of alpha.
  float rho_mean_;
  float rho_variance_;

  /// If set, the components are ordered by weight.  If unset, the
  /// components are ordered by weight/variance, as described in the
  /// paper.
  bool order_by_weight_only_;

  /// The sum of weights threshold used to determine if a component is
  /// a background component or a foreground component.
  float background_thres_;

  /// Number of pixel components.
  unsigned num_pix_comp_;

  /// Number of components in the Gaussian mixture.
  unsigned num_mixture_comp_;

  /// Minimum number of samples required before a pixel can be classified
  /// as foreground
  unsigned min_samples_for_detection_;

  /// A pixel must have a higher value than this for its model to be updated
  float no_update_threshold_;
  
  /// Number of interations to learn the background
  unsigned number_of_learning_iterations_;

  /// Types of learning modes
  enum learningEnumType{ useSlackWeight, useEvenDisWeight };
  learningEnumType  learning_type_;
  
  /// Takes the first gmm that is below the threshold
  bool take_first_matched_;

};


/// \brief Model pixel values using a Gaussian mixture.
///
/// This represents the pixel using a mixture of Gaussians as
/// described in the Stauffer-Grimson paper.
class sg_gm_pixel_model
{
public:
  typedef float float_type;

  sg_gm_pixel_model( sg_gm_pixel_model_params* param );
  sg_gm_pixel_model( const sg_gm_pixel_model& model );

  /// \brief Update the model with the given pixel.
  ///
  /// \a value must be an array containing the pixel component values.
  ///
  /// The return value will be \c true if the pixel is considered
  /// foreground according to the model.
  bool update( float_type* value );

  /// \brief A representative background pixel value.
  ///
  /// Typically, this will be a weighted average of the "background"
  /// mixture components.
  ///
  /// \a value must be pre-allocated to hold the appropriate number of
  /// pixel components.
  void background( float_type* value ) const;

  void debug_dump( vcl_ostream& ostr ) const;

protected:
  /// First, find out into which mixture component the new data fits,
  /// if any.  Return N if there is no fit.
  unsigned best_fit_component( float_type* value );

  /// Update the weight, mean, and variance of component \a idx.
  void update_component( unsigned idx, float_type* value );

  /// Sort the components in descending order of score_. \a best_idx
  /// will be adjusted to continue pointing to the same component
  /// after the sort. This routine assumes that only the score of \a
  /// idx has changed.
  void sort_components( unsigned& best_idx );

  /// Update the scores associated with each component.  This are used
  /// to sort the components to determine which ones are considered
  /// background and which ones are considered foreground.
  void update_scores();

  /// Renormalize the weights to sum to 1.
  void normalize_weights();

  /// "Initialize" the component with index \a idx with the pixel
  /// values in \a value, and set the variance and weight to the
  /// initial variance and weight.
  void copy_to_component( unsigned idx, float_type* value );

  /// The index refers to the sorted components, so that index 0
  /// refers to the component with the highest score.
  inline float_type euclidean_dist_sqr( unsigned idx, float_type* value ) const;

  /// The index refers to the sorted components, so that index 0
  /// refers to the component with the highest score.
  inline float_type mahanalobis_dist_sqr( unsigned idx, float_type* value ) const;

  /// The index refers to the sorted components, so that index 0
  /// refers to the component with the highest score.
  inline float_type likelihood( unsigned idx, float_type* value ) const;

  /// The function returns true if this mixture model is in learning mode
  inline bool in_learning_mode() const
  {
    return sample_count_ <= param_->min_samples_for_detection_;
  }

protected:
  /// mean_[i][*] is the mean of the i-th Gaussian component.
  vcl_vector<float_type> mean_data_blk_;
  /// variance_[i] is the single value representing the variance
  /// matrix of the i-th Gaussian component. The SG paper assumes that
  /// the variance matrix is of the form sI.
  vcl_vector<float_type> variance_data_blk_;
  /// weight_[i] is the weight of the i-th component.
  vcl_vector<float_type> weight_;
  /// Pointers to the rows of the mean_data_ matrix. This allows the
  /// mixtures to be quickly re-ordered.
  vcl_vector<float_type*> mean_ptr_;
  /// Pointers to the variance data.
  vcl_vector<float_type*> variance_ptr_;
  /// The value used to order the components. weight/sigma in the
  /// paper. The score corresponds to the indicies in mean_, not to
  /// the rows of mean_data_.
  vcl_vector<float_type> score_;
  /// The number of samples accumulated at the pixel location
  unsigned sample_count_;

  sg_gm_pixel_model_params* param_;
  float_type slack_weight_;
  unsigned count_zero_mixures_;

};


} // end namespace vidtk


#endif // vidtk_sg_gm_pixel_model_h_
