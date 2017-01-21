/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/gmm_background_model_process.h>

#include <vil/vil_transform.h>
#include <vil/vil_convert.h>
#include <vil/algo/vil_structuring_element.h>
#include <vnl/vnl_double_3.h>

#include <bbgm/bbgm_image_of.h>
#include <bbgm/bbgm_image_of.hxx>
#include <bbgm/bbgm_update.h>
#include <bbgm/bbgm_detect.h>
#include <bbgm/bbgm_apply.h>

#include <vpdl/vpdt/vpdt_gaussian.h>
#include <vpdl/vpdt/vpdt_mixture_of.h>
#include <vpdl/vpdt/vpdt_update_mog.h>
#include <vpdl/vpdt/vpdt_distribution_accessors.h>
#include <vpdl/vpdt/vpdt_gaussian_detector.h>
#include <vpdl/vpdt/vpdt_mixture_detector.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_gmm_background_model_process_txx__
VIDTK_LOGGER("gmm_background_model_process_txx");


// dummy function needed for bbgm_image_of
template <class T>
void vsl_b_read(vsl_b_istream& /* is */, vpdt_mixture_of<T>&){}
template <class T>
void vsl_b_write(vsl_b_ostream& /* os */, const vpdt_mixture_of<T>&){}

bool invert(bool val)
{
  return !val;
}


namespace
{

struct gmm_params
{
  std::string covar_mode;
  std::string update_algorithm;
  float init_variance;
  float gaussian_match_thresh;
  float mininum_stdev;
  int max_components;
  int lm_window_size;
  float sg_learning_rate;
  float sg_init_weight;
  float background_distance;
  float weight;
  float match_radius;
  unsigned int num_pix_comp;
  vnl_double_3 invalid_color;
};

template <class PixType>
class gmm_model_base
{
public:
  gmm_model_base() {}
  virtual ~gmm_model_base() {}

  virtual void initilize( unsigned ni, unsigned nj, const gmm_params& p) = 0;

  /// Clear the current background model
  virtual void clear() = 0;

  /// Width of the background model image
  virtual unsigned ni() const = 0;

  /// Height of the background model image
  virtual unsigned nj() const = 0;

  virtual void bg_detect(const vil_image_view<PixType>& img,
                      vil_image_view<bool>& bg_mask,
                      const vil_structuring_element& se) = 0;

  virtual void bg_update(const vil_image_view<PixType>& img) = 0;

  virtual vil_image_view<PixType> bg_mean(const vnl_double_3& invalid_color) const = 0;
};


template <class mix_gauss_type>
void make_updater(const gmm_params& p,
                 boost::scoped_ptr<vpdt_mog_lm_updater<mix_gauss_type> >& u)
{
  typedef vpdt_mog_lm_updater<mix_gauss_type> updater_type;
  typedef typename mix_gauss_type::component_type gauss_type;
  typedef typename gauss_type::field_type field_type;
  typedef typename gauss_type::covar_type covar_type;

  covar_type var;
  vpdt_fill(var, p.init_variance);
  gauss_type init_gauss(field_type(0.0f), var);
  u.reset(new updater_type( init_gauss,
                            p.max_components,
                            p.gaussian_match_thresh,
                            p.mininum_stdev,
                            p.lm_window_size ));
}

template <class mix_gauss_type>
void make_updater(const gmm_params& p,
                 boost::scoped_ptr<vpdt_mog_sg_updater<mix_gauss_type> >& u)
{
  typedef vpdt_mog_sg_updater<mix_gauss_type> updater_type;
  typedef typename mix_gauss_type::component_type gauss_type;
  typedef typename gauss_type::field_type field_type;
  typedef typename gauss_type::covar_type covar_type;

  covar_type var;
  vpdt_fill(var, p.init_variance);
  gauss_type init_gauss(field_type(0.0f), var);
  u.reset(new updater_type( init_gauss,
                            p.max_components,
                            p.gaussian_match_thresh,
                            p.sg_learning_rate,
                            p.sg_init_weight,
                            p.mininum_stdev ));
}


template <class PixType, class updater_type, class detector_type>
class gmm_model : public gmm_model_base<PixType>
{
public:
  typedef typename detector_type::distribution_type mix_gauss_type;
  typedef typename mix_gauss_type::component_type gauss_type;
  typedef typename gauss_type::field_type field_type;
  typedef typename gauss_type::covar_type covar_type;

  // this type should really be determined from the detector_type,
  // but it is currently not exposed.
  typedef vpdt_gaussian_mthresh_detector<gauss_type> gdetector_type;

  virtual void initilize( unsigned width, unsigned height, const gmm_params& p)
  {
    mix_gauss_type mixture;
    bg_model_ = bbgm_image_of<mix_gauss_type>(width, height, mixture);

    make_updater(p, updater_);

    gdetector_type gmd(p.background_distance);
    detector_.reset( new detector_type(gmd, p.weight) );
  }

  /// Clear the current background model
  virtual void clear()
  {
    bg_model_ = bbgm_image_of<mix_gauss_type>();
  }

  /// Width of the background model image
  virtual unsigned ni() const
  {
    return bg_model_.ni();
  }

  /// Height of the background model image
  virtual unsigned nj() const
  {
    return bg_model_.nj();
  }

  virtual void bg_detect(const vil_image_view<PixType>& img,
                      vil_image_view<bool>& bg_mask,
                      const vil_structuring_element& se)
  {
    detect(bg_model_, img, bg_mask, *detector_, se);
  }

  virtual void bg_update(const vil_image_view<PixType>& img)
  {
    update(bg_model_, img, *updater_);
  }

  virtual vil_image_view<PixType> bg_mean(const vnl_double_3& invalid_color) const
  {
    // create a functor to extract the mean image
    vpdt_mean_accessor<mix_gauss_type> mean_func;

    vil_image_view<double> double_image;
    bbgm_apply(bg_model_, mean_func, double_image,
               invalid_color.data_block());

    vil_image_view<PixType> img;
    vil_convert_cast(double_image, img);

    return img;
  }

private:
  /// The per-pixel background model
  bbgm_image_of<mix_gauss_type> bg_model_;

  /// The per-pixel updater function
  boost::scoped_ptr<updater_type> updater_;

  /// The per-pixel detector function
  boost::scoped_ptr<detector_type> detector_;
};

} // end anonymous name space


namespace vidtk
{


template <class PixType>
class gmm_background_model_process<PixType>::priv
{
public:

  priv() {}
  ~priv() {}

  /// The config block
  config_block config_;

  /// The background model helper including updater and detector functors
  boost::scoped_ptr<gmm_model_base<PixType> > bg_model;

  /// The structuring element for jitter in detection
  vil_structuring_element se_;

  /// The foreground mask for the current image.
  vil_image_view<bool> fgnd_mask_;

  /// The cached input frame
  vil_image_view<PixType> inp_frame_;

  /// The cached input homography
  vgl_h_matrix_2d<double> image_to_world_homog_;

  /// The configuration parameters
  gmm_params params_;
};


template <class PixType>
gmm_background_model_process<PixType>
::gmm_background_model_process( std::string const& _name )
  : fg_image_process<PixType>( _name, "gmm_background_model_process" ),
    d(new priv)
{
  d->config_.add_parameter( "covar_mode", "indep",
                            "Mode for modeling Gaussian covariance.  "
                            "Takes one of the following string values: \n"
                            "  full -- Use a full 3x3 covariance matrix\n"
                            "  indep -- Use a diagonal covariance matrix\n"
                            "  sphere -- Use scalar variance times identity covariance\n");
  d->config_.add_parameter( "update_algorithm", "LM",
                            "Update equations used for online MOG learning.  "
                            "Takes one of the following string values: \n"
                            "  SG -- Stauffer-Grimson equations\n"
                            "  LM -- Leotta-Mundy equations");
  d->config_.add_parameter( "initial_variance", "6502.5",
                            "Initial variance of new Gaussians added to the mixture" );
  d->config_.add_parameter( "gaussian_match_threshold", "3.0",
                            "Threshold in Mahalanobis distance for a "
                            "sample to match a Gaussian component during "
                            "the model update step" );
  d->config_.add_parameter( "mininum_stdev", "25.1",
                            "Lower bound on the standard deviation of each "
                            "Gaussian component" );
  d->config_.add_parameter( "num_mixture_components", "5",
                            "Maximum number of components allowed in the mixture" );
  d->config_.add_parameter( "lm_window_size", "100",
                            "Number of frames in a temporal moving window.  "
                            "Controls the dynamic learning rate in the "
                            "Leotta-Mundy update equations\n"
                            "Only used when update_algorithm = LM");
  d->config_.add_parameter( "sg_learning_rate", "0.1",
                            "Learning rate used in the Stauffer-Grimson "
                            "update equations (between 0 and 1).\n"
                            "Only used when update_algorithm = SG");
  d->config_.add_parameter( "sg_init_weight", "0.1",
                            "Initial weight of new components added to the "
                            "mixture in the Stauffer-Grimson "
                            "update equations (between 0 and 1).\n"
                            "Only used when update_algorithm = SG");
  d->config_.add_parameter( "background_distance_threshold", "3.0",
                            "Threshold in Mahalanobis distance for a "
                            "sample to match a Gaussian component during "
                            "the background detection step" );
  d->config_.add_parameter( "weight", "0.7",
                            "Fraction of the total weight of the top "
                            "Gaussian components that are classified as "
                            "background during detection");
  d->config_.add_parameter( "search_radius", "4.0",
                            "Radius of adjacent pixels to test during detection. "
                            "This parameter represents the ammount of jitter "
                            "to compensate for" );
  d->config_.add_parameter( "invalid_bg_color", "0 255 0",
                            "Color to draw invalid pixels when generating an "
                            "image of the background.  This only applies when "
                            "rendering components that don't yet exist.  "
                            "In the current implementation, we only look at "
                            "the whole mixture, so this should have no effect" );
}


template <class PixType>
bool
gmm_background_model_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    d->params_.covar_mode = blk.get<std::string>( "covar_mode" );
    d->params_.update_algorithm = blk.get<std::string>( "update_algorithm" );
    d->params_.init_variance = blk.get<float>( "initial_variance" );
    d->params_.gaussian_match_thresh = blk.get<float>( "gaussian_match_threshold" );
    d->params_.mininum_stdev = blk.get<float>( "mininum_stdev" );
    d->params_.max_components = blk.get<int>( "num_mixture_components" );
    d->params_.lm_window_size = blk.get<int>("lm_window_size");
    d->params_.sg_learning_rate = blk.get<float>("sg_learning_rate");
    d->params_.sg_init_weight = blk.get<float>("sg_init_weight");
    d->params_.background_distance = blk.get<float>("background_distance_threshold");
    d->params_.weight = blk.get<float>("weight");
    d->params_.match_radius = blk.get<float>("search_radius");
    d->params_.invalid_color = blk.get<vnl_double_3>("invalid_bg_color");
  }
  catch( const config_block_parse_error & e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }


  if ( d->params_.update_algorithm != "SG" &&
       d->params_.update_algorithm != "LM" )
  {
    LOG_ERROR( this->name() << ": unrecognized update_algorithm: "
               << d->params_.update_algorithm );
    return false;
  }

  if( d->params_.covar_mode == "sphere" )
  {
    // use a spherical Gaussian with scalar variance
    typedef vpdt_gaussian< vnl_vector_fixed<float,3>, float > gauss_type;
    typedef vpdt_mixture_of<gauss_type> mix_gauss_type;
    typedef vpdt_gaussian_mthresh_detector<gauss_type> gdetector_type;
    typedef vpdt_mixture_top_weight_detector<mix_gauss_type, gdetector_type> detector_type;
    if( d->params_.update_algorithm == "SG" )
    {
      typedef vpdt_mog_sg_updater<mix_gauss_type> updater_type;
      d->bg_model.reset( new gmm_model<PixType, updater_type, detector_type>() );
    }
    else if ( d->params_.update_algorithm == "LM" )
    {
      typedef vpdt_mog_lm_updater<mix_gauss_type> updater_type;
      d->bg_model.reset( new gmm_model<PixType, updater_type, detector_type>() );
    }
  }
  else if ( d->params_.covar_mode == "indep" )
  {
    // use an axis-independent Gaussian with diagonal covariance
    typedef vpdt_gaussian< vnl_vector_fixed<float,3>,
                           vnl_vector_fixed<float,3> > gauss_type;
    typedef vpdt_mixture_of<gauss_type> mix_gauss_type;
    typedef vpdt_gaussian_mthresh_detector<gauss_type> gdetector_type;
    typedef vpdt_mixture_top_weight_detector<mix_gauss_type, gdetector_type> detector_type;
    if( d->params_.update_algorithm == "SG" )
    {
      typedef vpdt_mog_sg_updater<mix_gauss_type> updater_type;
      d->bg_model.reset( new gmm_model<PixType, updater_type, detector_type>() );
    }
    else if ( d->params_.update_algorithm == "LM" )
    {
      typedef vpdt_mog_lm_updater<mix_gauss_type> updater_type;
      d->bg_model.reset( new gmm_model<PixType, updater_type, detector_type>() );
    }
  }
  else if ( d->params_.covar_mode == "full" )
  {
    // use a full covariance Gaussian
    typedef vpdt_gaussian< vnl_vector_fixed<float,3> > gauss_type;
    typedef vpdt_mixture_of<gauss_type> mix_gauss_type;
    typedef vpdt_gaussian_mthresh_detector<gauss_type> gdetector_type;
    typedef vpdt_mixture_top_weight_detector<mix_gauss_type, gdetector_type> detector_type;
    if( d->params_.update_algorithm == "SG" )
    {
      typedef vpdt_mog_sg_updater<mix_gauss_type> updater_type;
      d->bg_model.reset( new gmm_model<PixType, updater_type, detector_type>() );
    }
    else if ( d->params_.update_algorithm == "LM" )
    {
      typedef vpdt_mog_lm_updater<mix_gauss_type> updater_type;
      d->bg_model.reset( new gmm_model<PixType, updater_type, detector_type>() );
    }
  }
  else
  {
    LOG_ERROR( this->name() << ": unrecognized covar_mode: " << d->params_.covar_mode );
    return false;
  }

  d->se_.set_to_disk(d->params_.match_radius);

  d->config_.update(blk);

  return true;
}


template <class PixType>
void
gmm_background_model_process<PixType>
::set_source_image( vil_image_view<PixType> const& src )
{
  d->inp_frame_ = src;
}


template <class PixType>
void
gmm_background_model_process<PixType>
::set_homography( vgl_h_matrix_2d<double> homog )
{
  d->image_to_world_homog_ = homog;
}


template <class PixType>
vil_image_view<bool>
gmm_background_model_process<PixType>
::fg_image() const
{
  return d->fgnd_mask_;
}


template <class PixType>
vil_image_view<PixType>
gmm_background_model_process<PixType>
::bg_image() const
{
  return d->bg_model->bg_mean(d->params_.invalid_color);
}


template <class PixType>
gmm_background_model_process<PixType>
::~gmm_background_model_process()
{
}


template <class PixType>
config_block
gmm_background_model_process<PixType>
::params() const
{
  return d->config_;
}


template <class PixType>
bool
gmm_background_model_process<PixType>
::initialize()
{
  // We defer actual the initialization until we have the first frame,
  // so we can get the frame size and number of pixel components.
  // Emptying the foreground mask and background model signals that
  // the model needs to be initialized
  d->fgnd_mask_.clear();
  d->bg_model->clear();

  return true;
}


template <class PixType>
bool
gmm_background_model_process<PixType>
::reset()
{
  return initialize();
}


template <class PixType>
void
gmm_background_model_process<PixType>
::warp_array()
{
  // TODO: not implemented yet
}


template <class PixType>
bool
gmm_background_model_process<PixType>
::step()
{
  if( !d->inp_frame_ )
  {
    return false;
  }

  vil_image_view<PixType> const& img = d->inp_frame_;


  // check if the model has been initialized
  if( d->bg_model->ni() != img.ni() || d->bg_model->nj() != img.nj() )
  {
    // Initialize the background model
    d->bg_model->initilize( img.ni(), img.nj(), d->params_ );

    // use the first frame to update the background model
    d->bg_model->bg_update( img );

    // initially there is no model to compare against for detection,
    // so return an empty detection image
    d->fgnd_mask_ = vil_image_view<bool>(img.ni(), img.nj());
    d->fgnd_mask_.fill( false );

    return true;
  }

  // allocate new memory for the mask for async processing
  d->fgnd_mask_ = vil_image_view<bool>(img.ni(), img.nj());
  d->fgnd_mask_.fill(false);

  // detect the background pixels (see bbgm_detect.h)
  d->bg_model->bg_detect( img, d->fgnd_mask_, d->se_);

  // invert to get a foreground pixel mask
  vil_transform(d->fgnd_mask_, d->fgnd_mask_, invert);

  // update the background model (see bbgm_update.h)
  // updating should always happen after detection so that the model
  // does not include the current frame at detection time
  d->bg_model->bg_update( img );

  return true;
}

} // end namespace vidtk
