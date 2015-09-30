/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/sg_background_model_process.h>
#include <vnl/vnl_double_3.h>
#include <utilities/log.h>

#include <vil/vil_save.h>

namespace vidtk
{


template <class PixType>
sg_background_model_process<PixType>
::sg_background_model_process( vcl_string const& name )
  : fg_image_process<PixType>( name, "sg_background_model_process" ),
    param_( new sg_gm_pixel_model_params ),
    inp_frame_( NULL )
{
  config_.add( "sigma_threshold", "2.5" );
  config_.add( "initial_sigma", "10" );
  config_.add( "initial_weight", "0.01" );
  config_.add( "alpha_weights", "0.01" );
  config_.add( "alpha_means", "0.01" );
  config_.add( "alpha_variance", "0.01" );
  config_.add( "learning_alpha_weights", "0.01" );
  config_.add( "learning_alpha_means", "0.01" );
  config_.add( "learning_alpha_variance", "0.01" );
  config_.add( "rho_mean", "-1" );
  config_.add( "rho_variance", "-1" );
  config_.add( "background_threshold", "0.9" );
  config_.add( "order_by_weight_only", "false" );
  config_.add( "num_mixture_components", "5" );
  config_.add( "min_samples_for_detection", "0" );
  config_.add( "no_update_threshold", "-1.0" );
  config_.add( "stabilization_padding", "100" );
  config_.add( "take_first_matched", "true");
}


template <class PixType>
bool
sg_background_model_process<PixType>
::set_params( config_block const& blk )
{
  // TODO: add some validation.

  float sigma_t = blk.get<float>( "sigma_threshold" );
  param_->mahan_dist_sqr_thres_ = sigma_t * sigma_t;

  float init_sig = blk.get<float>( "initial_sigma" );
  param_->initial_variance_ = init_sig * init_sig;
  param_->initial_weight_ = blk.get<float>( "initial_weight" );

  param_->alpha_weights_ = blk.get<float>( "alpha_weights" );
  param_->alpha_means_ = blk.get<float>( "alpha_means" );
  param_->alpha_variance_ = blk.get<float>( "alpha_variance" );
  param_->learning_alpha_weights_ = blk.get<float>( "learning_alpha_weights" );
  param_->learning_alpha_means_ = blk.get<float>( "learning_alpha_means" );
  param_->learning_alpha_variance_ = blk.get<float>( "learning_alpha_variance" );
  param_->rho_mean_ = blk.get<float>( "rho_mean" );
  param_->rho_variance_ = blk.get<float>( "rho_variance" );
  param_->order_by_weight_only_ = blk.get<bool>( "order_by_weight_only" );
  param_->background_thres_ = blk.get<float>( "background_threshold" );

  param_->num_mixture_comp_ = blk.get<unsigned>( "num_mixture_components" );

  param_->min_samples_for_detection_ = blk.get<unsigned>( "min_samples_for_detection" );
  param_->no_update_threshold_ = blk.get<float>( "no_update_threshold" );

  array_padding_ = blk.get<unsigned>( "stabilization_padding" );

  param_->learning_type_ = sg_gm_pixel_model_params::useSlackWeight;

  param_->take_first_matched_ = blk.get<bool>( "take_first_matched" );

  config_.update( blk );

  return true;
}


template <class PixType>
void
sg_background_model_process<PixType>
::set_source_image( vil_image_view<PixType> const& src )
{
  inp_frame_ = &src;
}


template <class PixType>
void
sg_background_model_process<PixType>
::set_homography( vgl_h_matrix_2d<double> homog )
{
  image_to_world_homog_ = homog;
}


template <class PixType>
vil_image_view<bool> const&
sg_background_model_process<PixType>
::fg_image() const
{
  return fgnd_mask_;
}


template <class PixType>
vil_image_view<PixType>
sg_background_model_process<PixType>
::bg_image() const
{
  unsigned const ni = models_.cols();
  unsigned const nj = models_.rows();
  unsigned const nplanes = param_->num_pix_comp_;

  vil_image_view<PixType> img( ni, nj, 1, nplanes );

  vcl_vector<sg_gm_pixel_model::float_type> value( nplanes );

  for( unsigned j = 0; j < nj; ++j )
  {
    for( unsigned i = 0; i < ni; ++i )
    {
      models_(j,i)->background( &value[0] );
      for( unsigned p = 0; p < nplanes; ++p )
      {
        img(i,j,p) = PixType( value[p] );
      }
    }
  }

  return img;
}


template <class PixType>
void
sg_background_model_process<PixType>
::initialize_array( unsigned ni, unsigned nj, unsigned nplanes )
{
  assert( models_.size() == 0 );

  param_->num_pix_comp_ = nplanes;

  models_.resize( nj, ni );
  for( unsigned j = 0; j < nj; ++j )
  {
    for( unsigned i = 0; i < ni; ++i )
    {
      models_(j,i) = new sg_gm_pixel_model( &*param_ );
    }
  }

  fgnd_mask_.set_size( ni, nj );
  fgnd_mask_.fill( false );
}


template <class PixType>
sg_background_model_process<PixType>
::~sg_background_model_process()
{
  delete_array();
}


template <class PixType>
config_block
sg_background_model_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
sg_background_model_process<PixType>
::initialize()
{
  delete_array();

  // We defer actual the initialization until we have the first frame,
  // so we can get the frame size and number of pixel components.

  // Set padding translation homography
  padding_shift_.set_identity();
  padding_shift_.set_translation( array_padding_, array_padding_ );

  // Set initial homography in case it is not initialized correctly
  image_to_world_homog_.set_identity();

  ref_image_homog_ = image_to_world_homog_ * padding_shift_.get_inverse();

  return true;
}


template <class PixType>
void
sg_background_model_process<PixType>
::warp_array()
{
  vbl_array_2d< sg_gm_pixel_model* > new_models;
  new_models.resize( models_.rows(), models_.cols() );

  vnl_double_3 new_point;
  new_point[2] = 1;
  vnl_double_3 old_point;
  vgl_h_matrix_2d<double> H;
  H = ref_image_homog_.get_inverse() * image_to_world_homog_ * padding_shift_.get_inverse();
  int old_j, old_i;
  for( unsigned j = 0; j < new_models.rows(); j++ )
  {
    for( unsigned i = 0; i < new_models.cols(); i++ )
    {
      // Backproject from new array to old array
      new_point[0] = i;
      new_point[1] = j;
      old_point = H.get_matrix() * new_point;
      // nearest neighbor
      old_j = vnl_math::rnd(old_point[1] / old_point[2]);
      old_i = vnl_math::rnd(old_point[0] / old_point[2]);
      if( old_j < 0 || old_i < 0 ||
          old_j >= (int)models_.rows() ||
          old_i >= (int)models_.cols() )
      {
        new_models(j,i) = new sg_gm_pixel_model( &*param_ );
      }
      else
      {
        new_models(j,i) = new sg_gm_pixel_model( *models_(old_j,old_i) );
      }
    }
  }
  delete_array();
  models_ = new_models;
}


template <class PixType>
void
sg_background_model_process<PixType>
::delete_array()
{
  unsigned const ni = models_.cols();
  unsigned const nj = models_.rows();
  for( unsigned j = 0; j < nj; ++j )
  {
    for( unsigned i = 0; i < ni; ++i )
    {
      delete models_(j,i);
    }
  }

  models_.clear();
}


template <class PixType>
bool
sg_background_model_process<PixType>
::step()
{
  assert( inp_frame_ != NULL );

  vil_image_view<PixType> const& img = *inp_frame_;

  if( models_.size() == 0 )
  {
    // Make array larger here to accomodate warping and translation
    this->initialize_array( img.ni() + array_padding_ * 2 , img.nj() + array_padding_ * 2, img.nplanes() );

    // Set reference image - factor array offset into homography
    ref_image_homog_ = image_to_world_homog_ * padding_shift_.get_inverse();
  }

  unsigned const nplanes = img.nplanes();

  typedef sg_gm_pixel_model::float_type float_type;
  vcl_vector<float_type> pix_value( nplanes );

  vnl_double_3 current_point;
  current_point[2] = 1;
  // Projected point and projection matrix
  vnl_double_3 proj_point;
  vgl_h_matrix_2d<double> proj_matrix;
  proj_matrix = ref_image_homog_.get_inverse() * image_to_world_homog_;
  unsigned new_x, new_y;

  for( unsigned j = 0; j < img.nj(); ++j )
  {
    for( unsigned i = 0; i < img.ni(); ++i )
    {
      current_point[0] = i;
      current_point[1] = j;
      proj_point = proj_matrix.get_matrix() * current_point;
      // nearest neighbor
      new_x = vnl_math::rnd(proj_point[0] / proj_point[2]);
      new_y = vnl_math::rnd(proj_point[1] / proj_point[2]);

      // Check to see if warped point is outside array bounds
      if( new_x >= models_.cols() ||
          new_y >= models_.rows() )
      {
        vcl_cout << "Warped image out of array bounds. Setting new reference image.\n";
        // Warp array to new reference frame
        warp_array();
        // Set current image as reference (create new homog)
        ref_image_homog_ = image_to_world_homog_ * padding_shift_.get_inverse();
        // Set proj_matrix to translation
        proj_matrix.set_identity();
        proj_matrix.set_translation( array_padding_, array_padding_ );
        // Calculate new new_x and new_y (should be i,j + array_padding_)
        proj_point = proj_matrix.get_matrix() * current_point;
        new_x = vnl_math::rnd(proj_point[0] / proj_point[2]);
        new_y = vnl_math::rnd(proj_point[1] / proj_point[2]);
      }

      for( unsigned p = 0; p < nplanes; ++p )
      {
        pix_value[p] = float_type( img(i,j,p) );
      }
      fgnd_mask_(i,j) = models_(new_y,new_x)->update( &pix_value[0] );
    }
  }

  return true;
}

} // end namespace vidtk
