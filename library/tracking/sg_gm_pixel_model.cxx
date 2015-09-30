/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/sg_gm_pixel_model.h>

#include <vcl_iostream.h>
#include <vcl_fstream.h>
#include <vcl_cassert.h>

#include <vnl/vnl_math.h>

#include <vcl_sstream.h>

namespace vidtk
{


sg_gm_pixel_model
::sg_gm_pixel_model( sg_gm_pixel_model_params* param )
  : mean_data_blk_( param->num_mixture_comp_ * param->num_pix_comp_, 0 ),
    variance_data_blk_( param->num_mixture_comp_, 1 ),
    weight_( param->num_mixture_comp_, 0.0f ),
    mean_ptr_( param->num_mixture_comp_, NULL ),
    variance_ptr_( param->num_mixture_comp_, NULL ),
    score_( param->num_mixture_comp_, 0.0f ),
    sample_count_( 0 ),
    param_( param ),
    slack_weight_(1.0),
    count_zero_mixures_(param->num_mixture_comp_)
{
  for( unsigned i = 0; i < param->num_mixture_comp_; ++i )
  {
    mean_ptr_[i] = &mean_data_blk_[ i*param->num_pix_comp_ ];
    variance_ptr_[i] = &variance_data_blk_[i];
  }
}


sg_gm_pixel_model
::sg_gm_pixel_model( const sg_gm_pixel_model& model )
  : mean_data_blk_( model.mean_data_blk_ ),
    variance_data_blk_( model.variance_data_blk_ ),
    weight_( model.weight_ ),
    mean_ptr_( model.param_->num_mixture_comp_, NULL ),
    variance_ptr_( model.param_->num_mixture_comp_, NULL ),
    score_( model.score_ ),
    sample_count_( model.sample_count_ ),
    param_( model.param_ ),
    slack_weight_( model.slack_weight_ ),
    count_zero_mixures_(model.count_zero_mixures_)
{
  for( unsigned i = 0; i < this->param_->num_mixture_comp_; i++ )
  {
    this->mean_ptr_[i] = &this->mean_data_blk_[0] + (model.mean_ptr_[i] - &model.mean_data_blk_[0]);
    this->variance_ptr_[i] = &this->variance_data_blk_[0] + (model.variance_ptr_[i] - &model.variance_data_blk_[0]);
  }
}

bool
sg_gm_pixel_model
::update( float_type* value )
{
  // If pixel is empty (below threshold), do not update model
  bool all_no_update = true;
  for( unsigned int i = 0; i < param_->num_pix_comp_; ++i )
  {
    all_no_update = all_no_update && value[i] <= param_->no_update_threshold_;
  }
  if( all_no_update )
  {
    return false;
  }
  // Increment sample count and update model(only if pixel is above the threshold)
  sample_count_++;

  // Number of mixtures components
  unsigned const N = param_->num_mixture_comp_;

  // Test to see if we are exiting the learning mode.  If so, make sure the weights are ready.
  if(param_->min_samples_for_detection_ && sample_count_ == param_->min_samples_for_detection_ + 1)
  {
    switch(param_->learning_type_)
    {
      case sg_gm_pixel_model_params::useSlackWeight:
        if(count_zero_mixures_)
        {
          float_type const alpha = static_cast<float_type>( (1.)/(1.-slack_weight_) );
          for( unsigned i = 0; i < N; ++i )
          {
            weight_[i] = (alpha) * weight_[i];
          }
        }
        break;
      case sg_gm_pixel_model_params::useEvenDisWeight:
        normalize_weights();
        break;
    }
  }

  // If the first component has weight zero, then this is the first
  // image we are seeing. Just copy the image to create the
  // background, and return false to indicate that this pixel is not
  // foreground.
  if( weight_[0] == 0 && this->in_learning_mode())
  {
    copy_to_component( 0, value );
    return false; // not foreground.
  }
  else if( weight_[0] == 0 )
  {
    copy_to_component( 0, value );
    weight_[0] = 1.0;
    return false; // not foreground.
  }

  // Otherwise, we compare the pixel to the current background model.

  // Is the new pixel value a background value or a foreground value?
  bool foreground = false;

  // First, find out into which mixture component the new data fits,
  // if any.
  unsigned best_idx = best_fit_component( value );

  // If the pixel doesn't match any of the existing Gaussian
  // components, then it is a foreground pixel.  Create a new
  // Gaussian.
  if( best_idx == N )
  {
    copy_to_component( N-1, value );

    // Set best_idx to the Gaussian we just updated, so we can sort it
    // appropriately.
    best_idx = N-1;

    foreground = true;
  }
  else
  {
    update_component( best_idx, value );
  }

  // Renormalize the weights.  In principle, the Stauffer-Grimson
  // update rules should ensure that the sum remains at 1, but
  // floating point inaccuracies could cause drift, so we correct it
  // every time.  Also, the initial weights don't sum to 1, so we
  // definitely need to do this at least once.
  normalize_weights();

  update_scores();

  sort_components( best_idx );


  // Determine if best_idx is a background or foreground mixture
  // component, if we don't know already.
  if( foreground == false ) {
    float_type const background_thres = param_->background_thres_;
    unsigned i = 0;
    float_type sum_wgts = 0;
    while( i < N && sum_wgts < background_thres ) {
      sum_wgts += weight_[i];
      ++i;
    }
    // i is one beyond last background Gaussian
    if( best_idx >= i ) {
      foreground = true;
    }
  }

  // If not enough samples for accurate detection, do not classify as foreground
  if( this->in_learning_mode() )
  {
    foreground = false;
  }

  return foreground;
}


void
sg_gm_pixel_model
::background( float_type* value ) const
{
  unsigned const M = param_->num_pix_comp_;
  unsigned const N = param_->num_mixture_comp_;
  float_type const background_thres = param_->background_thres_;

  float_type sum_wgts = weight_[0];
  for( unsigned i = 0; i < M; ++i ) {
    value[i] = weight_[0] * this->mean_ptr_[0][i];
  }

  unsigned j = 1;
  while( j < N && sum_wgts < background_thres ) {
    float_type const* const mn = this->mean_ptr_[j];
    float_type w = weight_[j];
    for( unsigned i = 0; i < M; ++i ) {
      value[i] += w * mn[i];
    }
    sum_wgts += w;
    ++j;
  }

  float_type reciprocal_sum_wgts = 1 / sum_wgts;
  for( unsigned i = 0; i < M; ++i ) {
    value[i] *= reciprocal_sum_wgts;
  }
}

void
sg_gm_pixel_model
::debug_dump( vcl_ostream& ostr ) const
{
//   unsigned const M = param_->num_pix_comp_;
//   unsigned const N = param_->num_mixture_comp_;
// 
//   for( unsigned j = 0; j < N; ++j )
//   {
//     ostr << "     " << j << " " << weight_[j];
//     for( unsigned i = 0; i < M; ++i ) {
//       ostr << "  " << this->mean_ptr_[j][i];
//     }
//     ostr << " " << this->variance_ptr_[j][i];
//   }
  unsigned const M = param_->num_pix_comp_;
  unsigned const N = param_->num_mixture_comp_;

  ostr << "     sample count: " << sample_count_ << "  count zero mixures: " 
       << count_zero_mixures_ << " slack weight: " << slack_weight_ << vcl_endl;

  for( unsigned j = 0; j < N; ++j )
  {
    ostr << "          Mix ID: " << j << " Score: " << score_[j] << "  Weight: " << weight_[j];
    for( unsigned i = 0; i < M; ++i ) {
      ostr << "  Mean: " << this->mean_ptr_[j][i];
    }
    ostr << " Variance: " << *this->variance_ptr_[j] << vcl_endl;
    ostr << vcl_endl;
  }
}


unsigned
sg_gm_pixel_model
::best_fit_component( float_type* value )
{
  // Number of mixtures components
  unsigned const N = param_->num_mixture_comp_;

  unsigned best_idx = N;
  float_type best_dist = param_->mahan_dist_sqr_thres_;

  for( unsigned idx = 0; idx < N; ++idx )
  {
    float_type dist = mahanalobis_dist_sqr( idx, value );

    if ( dist < best_dist )
    {
      best_dist = dist;
      best_idx = idx;
      if(param_->take_first_matched_)
      {
        return idx;
      }
    }
  }

  return best_idx;
}


void
sg_gm_pixel_model
::update_component( unsigned best_idx, float_type* value )
{
  // Number of mixtures components
  unsigned const N = param_->num_mixture_comp_;

  // Number of components per pixel
  unsigned const M = param_->num_pix_comp_;

  // Learn the weights using alpha
  if(this->in_learning_mode())
  {
    float_type const alpha_weight = param_->learning_alpha_weights_;
    float_type const alpha_mean = param_->learning_alpha_means_;
    float_type const alpha_variance = param_->learning_alpha_variance_;
    float_type* const mn = mean_ptr_[best_idx];
    float_type* const var = variance_ptr_[best_idx];

    // Update the weights based on the learning type
    switch(param_->learning_type_)
    {
      case sg_gm_pixel_model_params::useSlackWeight:
        for( unsigned i = 0; i < N; ++i ) {
          weight_[i] = (1-alpha_weight) * weight_[i];
        }
        if(count_zero_mixures_)
        {
          slack_weight_ *= (1-alpha_weight);
        }
        weight_[best_idx] += alpha_weight;
        break;
      case sg_gm_pixel_model_params::useEvenDisWeight:
        for( unsigned i = 0; i < N && weight_[i]; ++i ) {
          weight_[i] = static_cast<float_type>( 1.0/N );
        }
        break;
    }
    float_type rho_mean = param_->rho_mean_;
    float_type rho_var = param_->rho_variance_;
    assert(!vnl_math::isnan(rho_mean));
    assert(!vnl_math::isnan(rho_var));
    if( rho_mean == 0.0f )
    {
      rho_mean = alpha_mean * likelihood( best_idx, value );
    }
    else if( rho_mean < 0.0f )
    {
      rho_mean = alpha_mean;
    }
    if( rho_var == 0.0f )
    {
      rho_var = alpha_variance * likelihood( best_idx, value );
    }
    else if( rho_var < 0.0f )
    {
      rho_var = alpha_variance;
    }
    assert(rho_mean<=1.0);
    assert(rho_var<=1.0);
    assert(rho_mean>=0.0);
    assert(rho_var>=0.0);

    assert(!vnl_math::isnan(rho_mean));
    assert(!vnl_math::isnan(rho_var));

    // Update the mean and variance using rho as the learning rate.
    for( unsigned i = 0; i < M; ++i ) {
      mn[i] += rho_mean*(value[i]-mn[i]);
      assert(!vnl_math::isnan(mn[i]));
    }

    *var += (rho_var) * (euclidean_dist_sqr( best_idx, value ) - *var);
    assert(!vnl_math::isnan(*var));
    assert(*var > 0);
  }
  else
  {
    float_type const alpha_weight = param_->alpha_weights_;
    float_type const alpha_mean = param_->alpha_means_;
    float_type const alpha_variance = param_->alpha_variance_;
    float_type* const mn = mean_ptr_[best_idx];
    float_type* const var = variance_ptr_[best_idx];
    // Update the weights
    for( unsigned i = 0; i < N; ++i ) {
      weight_[i] = (1-alpha_weight) * weight_[i];
    }
    weight_[best_idx] += alpha_weight;

    // Update the mean and variance
    float_type rho_mean = param_->rho_mean_;
    float_type rho_var = param_->rho_variance_;
    assert(!vnl_math::isnan(rho_mean));
    assert(!vnl_math::isnan(rho_var));
    if( rho_mean == 0.0f )
    {
//       vcl_cout << alpha_mean << "\t" << likelihood( best_idx, value ) << vcl_endl;
      rho_mean = alpha_mean * likelihood( best_idx, value );
    }
    else if( rho_mean < 0.0f )
    {
      rho_mean = alpha_mean;
    }

    if( rho_var == 0.0f )
    {
      rho_var = alpha_variance * likelihood( best_idx, value );
    }
    else if( rho_var < 0.0f )
    {
      rho_var = alpha_variance;
    }
    assert(!vnl_math::isnan(rho_mean));
    assert(!vnl_math::isnan(rho_var));

//     vcl_cout << rho_mean << vcl_endl;
    assert(rho_mean<=1.0);
    assert(rho_var<=1.0);

    // Update the mean and variance using rho as the learning rate.
    for( unsigned i = 0; i < M; ++i ) {
      //mn[i] = (1-rho_mean) * mn[i] + rho_mean * value[i];
      mn[i] += rho_mean*(value[i]-mn[i]);
      assert(!vnl_math::isnan(mn[i]));
    }

    //*var = (1-rho_var) * (*var) + rho_var * euclidean_dist_sqr( best_idx, value );
    *var += (rho_var) * (euclidean_dist_sqr( best_idx, value ) - *var);
    assert(!vnl_math::isnan(*var));
    assert(*var > 0.0);
  }
}


void
sg_gm_pixel_model
::sort_components( unsigned& best_idx_param )
{
  // Copy to a local variable to give the optimizer the best chance of
  // putting the value in a register.
  unsigned best_idx = best_idx_param;

  // Resort the score in descending order.  Since the scores were
  // previously sorted, and only the best_idx component changed, we
  // simply need to bubble that component.  If the score increased, we
  // need to bubble "up" (towards index 0).  If the score decreased,
  // we need to bubble "down".  The most efficient way is to try
  // bubble both ways; the first check of the while loops will ensure
  // that the loop doesn't execute for the "wrong" direction.

  #define SWAP( X, Y )                                                  \
    vcl_swap( score_[X], score_[Y] );                                   \
    vcl_swap( weight_[X], weight_[Y] );                                 \
    vcl_swap( variance_ptr_[X], variance_ptr_[Y] );                     \
    vcl_swap( mean_ptr_[X], mean_ptr_[Y] );

  // Bubble down, if necessary.
  while( best_idx > 0 && score_[best_idx-1] < score_[best_idx] ) {
    SWAP( best_idx-1, best_idx );
    --best_idx;
  }

  // Bubble up, if necessary
  unsigned const Nm1 = param_->num_mixture_comp_ - 1;
  while( best_idx < Nm1 && score_[best_idx] < score_[best_idx+1] ) {
    SWAP( best_idx, best_idx+1 );
    ++best_idx;
  }

  // Copy the new position out.
  best_idx_param = best_idx;
}


void
sg_gm_pixel_model
::update_scores()
{
  // Number of mixtures components
  unsigned const N = param_->num_mixture_comp_;

  // In principle, only the best_idx component changed.  However,
  // renormalizing the weights could have caused all the weights to
  // change, so we update all the scores.

  if( param_->order_by_weight_only_ )
  {
    for( unsigned i = 0; i < N; ++i ) {
      score_[i] = weight_[i];
    }
  }
  else
  {
    for( unsigned i = 0; i < N; ++i ) {
      score_[i] = weight_[i] / *variance_ptr_[i];
    }
  }
}


void
sg_gm_pixel_model
::normalize_weights()
{
  // Number of mixtures components
  unsigned const N = param_->num_mixture_comp_;

  float_type sum_wgts = 0;
  if(this->in_learning_mode())
  {
    switch(param_->learning_type_)
    {
      case sg_gm_pixel_model_params::useSlackWeight:
        if(count_zero_mixures_)
        {
          sum_wgts += slack_weight_;
        }
        else
        {
          slack_weight_ = 0;
        }
        break;
      case sg_gm_pixel_model_params::useEvenDisWeight:
        for( unsigned i = 0; i < N && weight_[i]; ++i ) {
          weight_[i] = static_cast<float_type>( 1.0/N );
        }
        break;
    }
  }
  for( unsigned i = 0; i < N; ++i ) {
    sum_wgts += weight_[i];
  }
  float_type reciprocal_sum_wgts = 1 / sum_wgts;
  for( unsigned i = 0; i < N; ++i ) {
    weight_[i] *= reciprocal_sum_wgts;
  }
  slack_weight_*= reciprocal_sum_wgts;
}


void
sg_gm_pixel_model
::copy_to_component( unsigned idx, float_type* value )
{
  unsigned const M = param_->num_pix_comp_;
  unsigned const N = param_->num_mixture_comp_;
  float_type* const mn = mean_ptr_[idx];
  float_type* const var = variance_ptr_[idx];
  for( unsigned i = 0; i < M; ++i ) {
    mn[i] = value[i];
  }
  *var = param_->initial_variance_;

  // Update the weights
  if(this->in_learning_mode())
  {
    switch(param_->learning_type_)
    {
      case sg_gm_pixel_model_params::useSlackWeight:
      {
        if(count_zero_mixures_==1 || slack_weight_ < param_->initial_weight_)
        {
          weight_[idx] = slack_weight_;
          count_zero_mixures_--;
          slack_weight_ = 0;
          float_type const alpha = static_cast<float_type>( (1.-param_->initial_weight_)/(1.-weight_[idx]) );
          for( unsigned i = 0; i < N; ++i ) {
            weight_[i] = (alpha) * weight_[i];
          }
        }
        else if(count_zero_mixures_)
        {
          slack_weight_ = slack_weight_ - param_->initial_weight_;
          count_zero_mixures_--;
        }
        else
        {
          float_type const alpha = static_cast<float_type>( (1.-param_->initial_weight_)/(1.-weight_[idx]) );
          for( unsigned i = 0; i < N; ++i ) {
            weight_[i] = (alpha) * weight_[i];
          }
        }

        weight_[idx] = param_->initial_weight_;
        break;
      }
      case sg_gm_pixel_model_params::useEvenDisWeight:
        weight_[idx] = static_cast<float_type>( 1./N );
        break;
    }
  }
  else
  {
    float_type const alpha = static_cast<float_type>( (1.-param_->initial_weight_)/(1.-weight_[idx]) );
    for( unsigned i = 0; i < N; ++i ) {
      weight_[i] = (alpha) * weight_[i];
    }
    weight_[idx] = param_->initial_weight_;
  }
}


sg_gm_pixel_model::float_type
sg_gm_pixel_model
::euclidean_dist_sqr( unsigned idx, float_type* value ) const
{
  // Number of components per pixel
  unsigned const M = param_->num_pix_comp_;

  float_type const* const mn = mean_ptr_[idx];
  float_type sum = 0;
  for( unsigned i = 0; i < M; ++i ) {
    float_type diff = value[i] - mn[i];
    sum += diff * diff;
  }
  assert(sum >= 0);

  return sum;
}


sg_gm_pixel_model::float_type
sg_gm_pixel_model
::mahanalobis_dist_sqr( unsigned idx, float_type* value ) const
{
  assert((*(variance_ptr_[idx]))>0);
  return euclidean_dist_sqr( idx, value ) / *(variance_ptr_[idx]);
}


sg_gm_pixel_model::float_type
sg_gm_pixel_model
::likelihood( unsigned idx, float_type* value ) const
{
  static float_type const two_pi = float_type( 2.0 * vnl_math::pi );
  float_type const M = static_cast<float_type>( param_->num_pix_comp_ );
  float_type const var = *variance_ptr_[idx];
  float_type const dist = mahanalobis_dist_sqr( idx, value );
  assert(var!=0.0);
  assert(dist>=0);

  return vcl_exp( -float_type(0.5)*dist ) / vcl_pow( two_pi * var,  M/float_type(2.0) );
}


} // end namespace vidtk
