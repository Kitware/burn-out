/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/gmm_background_model_process.h>
#include <vnl/vnl_double_3.h>
#include <utilities/log.h>

#include <vil/vil_save.h>
#include <vil/vil_transform.h>

// dummy function needed for bbgm_image_of
template <class T>
void vsl_b_read(vsl_b_istream& is, vpdt_mixture_of<T>& dummy){}
template <class T>
void vsl_b_write(vsl_b_ostream& os, const vpdt_mixture_of<T>& dummy){}

bool invert(bool val)
{
  return !val;
}

namespace vidtk
{


template <class PixType>
gmm_background_model_process<PixType>
::gmm_background_model_process( vcl_string const& name )
  : fg_image_process<PixType>( name, "gmm_background_model_process" ),
    param_( new gmm_background_model_params ),
    inp_frame_( NULL ), updater_(NULL), detector_(NULL)
{
  config_.add( "initial_variance", "6502.5" );
  config_.add( "gaussian_match_threshold", "3.0" );
  config_.add( "mininum_stdev", "25.1" );
  config_.add( "num_mixture_components", "7" );
  config_.add( "size_of_window", "100" );
  config_.add( "background_distance_threshold", "3.0");
  config_.add( "weight", "0.7" );
  config_.add( "search_radius", "4.0" );
}


template <class PixType>
bool
gmm_background_model_process<PixType>
::set_params( config_block const& blk )
{
  param_->init_variance_ = blk.get<float>( "initial_variance" );
  param_->gaussian_match_thresh_ = blk.get<float>( "gaussian_match_threshold" );
  param_->mininum_stdev_ = blk.get<float>( "mininum_stdev" );
  param_->max_components_ = blk.get<int>( "num_mixture_components" );
  param_->window_size_ = blk.get<int>("size_of_window");
  param_->background_distance_ = blk.get<float>("background_distance_threshold");
  param_->weight_ = blk.get<float>("weight");
  param_->match_radius_ = blk.get<float>("search_radius");

  // TODO: add some validation.
  return true;
}


template <class PixType>
void
gmm_background_model_process<PixType>
::set_source_image( vil_image_view<PixType> const& src )
{
  inp_frame_ = &src;
}


template <class PixType>
void
gmm_background_model_process<PixType>
::set_homography( vgl_h_matrix_2d<double> homog )
{
  image_to_world_homog_ = homog;
}


template <class PixType>
vil_image_view<bool> const&
gmm_background_model_process<PixType>
::fg_image() const
{
  return fgnd_mask_;
}


template <class PixType>
vil_image_view<PixType>
gmm_background_model_process<PixType>
::bg_image() const
{
  unsigned const ni = bg_model_.ni();
  unsigned const nj = bg_model_.nj();
  unsigned const nplanes = param_->num_pix_comp_;

  vil_image_view<PixType> img( ni, nj, 1, nplanes );

//   vcl_vector<sg_gm_pixel_model::float_type> value( nplanes );
  //TODO:

//   for( unsigned j = 0; j < nj; ++j )
//   {
//     for( unsigned i = 0; i < ni; ++i )
//     {
//       models_(j,i)->background( &value[0] );
//       for( unsigned p = 0; p < nplanes; ++p )
//       {
//         img(i,j,p) = PixType( value[p] );
//       }
//     }
//   }

  return img;
}


template <class PixType>
void
gmm_background_model_process<PixType>
::initialize_array( unsigned ni, unsigned nj, unsigned nplanes )
{
  delete_array();
  param_->num_pix_comp_ = nplanes;
  gauss_type init_gauss(field_type(0.0f/*,nplanes*/), /*field_type(*/param_->init_variance_/*,nplanes*//*)*/);
  updater_ = new updater_type( init_gauss, param_->max_components_, param_->gaussian_match_thresh_,
                               /*alpha, init_weight,*/ param_->mininum_stdev_, param_->window_size_ );
  mix_gauss_type mixture;
  bg_model_ = bbgm_image_of<mix_gauss_type>(ni,nj,mixture);

  gdetector_type gmd(param_->background_distance_);
  detector_ = new detector_type(gmd, param_->weight_);

  se_.set_to_disk(param_->match_radius_);

//   models_.resize( nj, ni );
//   for( unsigned j = 0; j < nj; ++j )
//   {
//     for( unsigned i = 0; i < ni; ++i )
//     {
//       models_(j,i) = new sg_gm_pixel_model( &*param_ );
//     }
//   }

  fgnd_mask_.set_size( ni, nj );
  fgnd_mask_.fill( false );
}


template <class PixType>
gmm_background_model_process<PixType>
::~gmm_background_model_process()
{
  delete_array();
}


template <class PixType>
config_block
gmm_background_model_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
gmm_background_model_process<PixType>
::initialize()
{
   delete_array();

  // We defer actual the initialization until we have the first frame,
  // so we can get the frame size and number of pixel components.

  // Set padding translation homography
//   padding_shift_.set_identity();
//   padding_shift_.set_translation( array_padding_, array_padding_ );

  // Set initial homography in case it is not initialized correctly
//   image_to_world_homog_.set_identity();

//   ref_image_homog_ = image_to_world_homog_ * padding_shift_.get_inverse();

  return true;
}


template <class PixType>
void
gmm_background_model_process<PixType>
::warp_array()
{
//   vbl_array_2d< sg_gm_pixel_model* > new_models;
//   new_models.resize( models_.rows(), models_.cols() );
// 
//   vnl_double_3 new_point;
//   new_point[2] = 1;
//   vnl_double_3 old_point;
//   vgl_h_matrix_2d<double> H;
//   H = ref_image_homog_.get_inverse() * image_to_world_homog_ * padding_shift_.get_inverse();
//   int old_j, old_i;
//   for( unsigned j = 0; j < new_models.rows(); j++ )
//   {
//     for( unsigned i = 0; i < new_models.cols(); i++ )
//     {
//       // Backproject from new array to old array
//       new_point[0] = i;
//       new_point[1] = j;
//       old_point = H.get_matrix() * new_point;
//       // nearest neighbor
//       old_j = vnl_math_rnd(old_point[1] / old_point[2]);
//       old_i = vnl_math_rnd(old_point[0] / old_point[2]);
//       if( old_j < 0 || old_i < 0 ||
//           old_j >= (int)models_.rows() ||
//           old_i >= (int)models_.cols() )
//       {
//         new_models(j,i) = new sg_gm_pixel_model( &*param_ );
//       }
//       else
//       {
//         new_models(j,i) = new sg_gm_pixel_model( *models_(old_j,old_i) );
//       }
//     }
//   }
//   delete_array();
//   models_ = new_models;
}


template <class PixType>
void
gmm_background_model_process<PixType>
::delete_array()
{
  if(updater_)
  {
    delete updater_;
    updater_ = NULL;
  }
  if(detector_)
  {
    delete detector_;
    detector_ = NULL;
  }
//   unsigned const ni = models_.cols();
//   unsigned const nj = models_.rows();
//   for( unsigned j = 0; j < nj; ++j )
//   {
//     for( unsigned i = 0; i < ni; ++i )
//     {
//       delete models_(j,i);
//     }
//   }
// 
//   models_.clear();
}


template <class PixType>
bool
gmm_background_model_process<PixType>
::step()
{
  assert( inp_frame_ != NULL );

  vil_image_view<PixType> const& img = *inp_frame_;

  if( fgnd_mask_.ni() != img.ni() || fgnd_mask_.nj() != img.nj() )
  {
    // Make array larger here to accomodate warping and translation
    this->initialize_array( img.ni(), img.nj(), img.nplanes() );

    // Set reference image - factor array offset into homography
    // ref_image_homog_ = image_to_world_homog_ * padding_shift_.get_inverse();
  }

  // unsigned const nplanes = img.nplanes();
  // vcl_cout << field_type::dimension << "  " << nplanes << vcl_endl;

  update(bg_model_,img,*updater_);


  // clear the mask
  fgnd_mask_.fill(false);
  // detect the background pixels
  detect(bg_model_,img,fgnd_mask_,*detector_,se_);

  vil_transform(fgnd_mask_,fgnd_mask_,invert);

//   typedef sg_gm_pixel_model::float_type float_type;
//   vcl_vector<float_type> pix_value( nplanes );
// 
//   vnl_double_3 current_point;
//   current_point[2] = 1;
//   // Projected point and projection matrix
//   vnl_double_3 proj_point;
// //   vgl_h_matrix_2d<double> proj_matrix;
// //   proj_matrix = ref_image_homog_.get_inverse() * image_to_world_homog_;
//   unsigned new_x, new_y;
// 
//   for( unsigned j = 0; j < img.nj(); ++j )
//   {
//     for( unsigned i = 0; i < img.ni(); ++i )
//     {
//       current_point[0] = i;
//       current_point[1] = j;
//       proj_point = proj_matrix.get_matrix() * current_point;
//       // nearest neighbor
//       new_x = vnl_math_rnd(proj_point[0] / proj_point[2]);
//       new_y = vnl_math_rnd(proj_point[1] / proj_point[2]);
// 
//       // Check to see if warped point is outside array bounds
//       if( new_x < 0 || new_y < 0 ||
//           new_x >= models_.cols() ||
//           new_y >= models_.rows() )
//       {
//         vcl_cout << "Warped image out of array bounds. Setting new reference image.\n";
//         // Warp array to new reference frame
//         warp_array();
//         // Set current image as reference (create new homog)
//         ref_image_homog_ = image_to_world_homog_ * padding_shift_.get_inverse();
//         // Set proj_matrix to translation
//         proj_matrix.set_identity();
//         proj_matrix.set_translation( array_padding_, array_padding_ );
//         // Calculate new new_x and new_y (should be i,j + array_padding_)
//         proj_point = proj_matrix.get_matrix() * current_point;
//         new_x = vnl_math_rnd(proj_point[0] / proj_point[2]);
//         new_y = vnl_math_rnd(proj_point[1] / proj_point[2]);
//       }
// 
//       for( unsigned p = 0; p < nplanes; ++p )
//       {
//         pix_value[p] = float_type( img(i,j,p) );
//       }
//       fgnd_mask_(i,j) = models_(new_y,new_x)->update( &pix_value[0] );
//     }
//   }
  return true;
}

} // end namespace vidtk
