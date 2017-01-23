/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief MAP-Tk shot stitching algorithm implementation
 */

#include "maptk_shot_stitching_algo.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/foreach.hpp>

#include <vil/vil_convert.h>
#include <vil/vil_math.h>
#include <vil/vil_transform.h>

#include <vital/algo/convert_image.h>
#include <vital/algo/detect_features.h>
#include <vital/algo/estimate_homography.h>
#include <vital/algo/extract_descriptors.h>
#include <vital/algo/match_features.h>

#include <vital/config/config_block.h>
#include <vital/vital_types.h>

#include <vital/algorithm_plugin_manager.h>

#include <maptk/plugins/vxl/image_container.h>

static kwiver::vital::logger_handle_t vital_logger( kwiver::vital::get_logger( "maptk_shot_stitching_algo" ) );

namespace vidtk
{
//============================================================================
// Anon Namespace helper methods
//  -> should be put in common utilities file (currently copy/pasted in places)
//----------------------------------------------------------------------------

namespace
{


/// Convenience typedef for VidTK config_block value maps
typedef std::map<std::string, config_block_value>::value_type config_block_value_map_value_t;


/// Convert a MAP-Tk config block into a new VidTK config block
::vidtk::config_block
convert_config_block( ::kwiver::vital::config_block_sptr const mc )
{
  ::vidtk::config_block vc;
  std::string vk, vv, vd;
  BOOST_FOREACH( ::kwiver::vital::config_block_keys_t::value_type const &k,
                 mc->available_values() )
  {
    vk = static_cast<std::string>(k);
    vv = mc->get_value<std::string>(k);
    vd = static_cast<std::string>(mc->get_description(k));

    // only add a default value if there is any value set in original structure
    if( vv.length() == 0 )
    {
      vc.add_parameter(vk, vd);
    }
    else
    {
      vc.add_parameter(vk, vv, vd);
    }
  }

  return vc;
}


/// Convert a VidTK config block into a new MAP-Tk config block
::kwiver::vital::config_block_sptr
convert_config_block( ::vidtk::config_block const &vc )
{
  ::kwiver::vital::config_block_sptr          mc = ::kwiver::vital::config_block::empty_config();
  ::kwiver::vital::config_block_key_t         mk;
  ::kwiver::vital::config_block_value_t       mv;
  ::kwiver::vital::config_block_description_t md;

  BOOST_FOREACH( config_block_value_map_value_t const &p,
                 vc.enumerate_values() )
  {
    mk = static_cast< ::kwiver::vital::config_block_key_t >(p.first);
    mv = static_cast< ::kwiver::vital::config_block_value_t >(p.second.value());
    md = static_cast< ::kwiver::vital::config_block_description_t >(p.second.description());

    mc->set_value(mk, mv, md);
  }

  return mc;
}


// functor for mask image inversion transformation
template< class inT, class outT >
class inverse_functor
{
public:

  outT operator()(inT const &v)
  {
    return static_cast<outT>( !v );
  }

};


/// Convert/Stretch two images based on joint min/max range values
template <typename src_t>
void
stretch_two_images( vil_image_view< src_t > const &img1,
                    vil_image_view< src_t > const &img2,
                    vil_image_view< vxl_byte > &img1_stretched,
                    vil_image_view< vxl_byte > &img2_stretched )
{
  src_t img1_min_value, img1_max_value,
        img2_min_value, img2_max_value,
        global_min_v, global_max_v;
  vil_math_value_range(img1, img1_min_value, img1_max_value);
  vil_math_value_range(img2, img2_min_value, img2_max_value);
  global_min_v = std::min(img1_min_value, img2_min_value);
  global_max_v = std::max(img1_max_value, img2_max_value);

  vil_convert_stretch_range_limited( img1, img1_stretched,
                                     global_min_v, global_max_v );
  vil_convert_stretch_range_limited( img2, img2_stretched,
                                     global_min_v, global_max_v );
}


} // end anonymous namespace


//============================================================================
// maptk_shot_stitching_algo_impl
//----------------------------------------------------------------------------

// Shortcut macro for arbitrarilly acting over the impl's algorithm elements
// ``call`` macro must take two arguments: (algo_type, algo_name)
// ``name`` is used for both member variable and configuration block.
#define impl_algos(call) \
  /*    type                                name                 */ \
  call( ::kwiver::vital::algo::convert_image,       image_converter      );    \
  call( ::kwiver::vital::algo::detect_features,     feature_detector     ); \
  call( ::kwiver::vital::algo::extract_descriptors, descriptor_extractor ); \
  call( ::kwiver::vital::algo::match_features,      feature_matcher      ); \
  call( ::kwiver::vital::algo::estimate_homography, homog_estimator      )


class maptk_shot_stitching_algo_impl
{
public:
  maptk_shot_stitching_algo_impl();
  ~maptk_shot_stitching_algo_impl();

  /// Return current MAP-Tk parameter config block
  /**
   * If nothing has been set yet, default
   */
  ::kwiver::vital::config_block_sptr params() const;

  /// Create homography describing transform from src to dest
  image_to_image_homography
  stitch( vil_image_view<vxl_byte> const &src,
          vil_image_view<bool>     const &src_mask,
          vil_image_view<vxl_byte> const &dest,
          vil_image_view<bool>     const &dest_mask );

  /// Inlier limit for determining a valid estimation result
  unsigned int min_inliers_;

  /// Define component algorithm member variables
# define define_algos(type, name) \
  type##_sptr name
  impl_algos( define_algos );
# undef define_algos

};


maptk_shot_stitching_algo_impl
::maptk_shot_stitching_algo_impl()
  : min_inliers_( 12 )
{
}


maptk_shot_stitching_algo_impl
::~maptk_shot_stitching_algo_impl()
{
}


::kwiver::vital::config_block_sptr
maptk_shot_stitching_algo_impl
::params() const
{
  ::kwiver::vital::config_block_sptr mc = ::kwiver::vital::config_block::empty_config();

  mc->set_value( "min_inliers", this->min_inliers_,
                 "The minimum number of match inliers required for an "
                 "estimation to be considered valid." );

  // Populating configuration with component algorithm configurations.
  // Configuration block names are the same as the variable name.
# define get_algo_config(type, name) \
  type::get_nested_algo_configuration( #name, mc, this->name )

  impl_algos( get_algo_config );

# undef get_algo_config

  // Set defaults for algorithms not yet defined
  if( ! this->image_converter )
  {
    mc->set_value( "image_converter:type", "bypass" );
  }

  if( ! this->feature_detector )
  {
    mc->set_value( "feature_detector:type", "ocv_SURF" );
    mc->set_value( "feature_detector:ocv_SURF:hessian_threshold", 250 );
    mc->set_value( "feature_detector:ocv_SURF:upright", true );
  }

  if( ! this->descriptor_extractor )
  {
    mc->set_value( "descriptor_extractor:type", "ocv_SURF" );
    mc->set_value( "descriptor_extractor:ocv_SURF:hessian_threshold", 250 );
    mc->set_value( "descriptor_extractor:ocv_SURF:upright", true );
  }

  if( ! this->feature_matcher )
  {
    mc->set_value( "feature_matcher:type", "ocv_flann_based" );
    mc->set_value( "feature_matcher:type:ocv_flann_based:cross_check", true );
    mc->set_value( "feature_matcher:type:ocv_flann_based:cross_check_k", 1 );
  }

  if( ! this->homog_estimator )
  {
    mc->set_value( "homog_estimator:type", "vxl" );
  }

  return mc;
}


/// Create homography describing transform from src to dest
image_to_image_homography
maptk_shot_stitching_algo_impl
::stitch( vil_image_view<vxl_byte> const &src,
          vil_image_view<bool>     const &src_mask,
          vil_image_view<vxl_byte> const &dest,
          vil_image_view<bool>     const &dest_mask )
{
  image_to_image_homography H_src2dst;

  // If either of our input images are of zero size, we won't be able to
  // stitch them
  if( !src || !dest )
  {
    LOG_WARN(vital_logger, "maptk_shot_stitching_algo: One or both input images does not "
             "contain valid data");
    return H_src2dst;
  }

  // Need to convert given masks from the given format, where positive values
  // indicate the masked out regions, to the format where positive values
  // indicate the regions where features should be detected, i.e. flip the
  // given masks' values.
  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: Creating src/dest mask copies for inversion" );
  vil_image_view<vxl_byte> src_mask_inv, dest_mask_inv;
  inverse_functor<bool, vxl_byte> inv_f;
  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: Inverting src_mask" );
  vil_transform( src_mask, src_mask_inv, inv_f );
  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: Inverting dest_mask" );
  vil_transform( dest_mask, dest_mask_inv, inv_f );

  // Positive values in masks define areas where features should be detected
  ::kwiver::vital::image_container_sptr
      i1_image( new ::kwiver::maptk::vxl::image_container( src ) ),
      i1_mask ( new ::kwiver::maptk::vxl::image_container( src_mask_inv ) ),
      i2_image( new ::kwiver::maptk::vxl::image_container( dest ) ),
      i2_mask ( new ::kwiver::maptk::vxl::image_container( dest_mask_inv ) );

  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: converting input images..." );
  i1_image = this->image_converter->convert( i1_image );
  i2_image = this->image_converter->convert( i2_image );

  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: Generating features..." );
  ::kwiver::vital::feature_set_sptr i1_features = this->feature_detector->detect( i1_image, i1_mask ),
                            i2_features = this->feature_detector->detect( i2_image, i2_mask );

  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: Generating descriptors..." );
  ::kwiver::vital::descriptor_set_sptr
    i1_descriptors = this->descriptor_extractor->extract( i1_image, i1_features ),
    i2_descriptors = this->descriptor_extractor->extract( i2_image, i2_features );

  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: -- Img1 features / descriptors: " <<
             i1_features->size() << " / " << i1_descriptors->size() );
  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: -- Img2 features / descriptors: " <<
             i2_features->size() << " / " << i2_descriptors->size() );

  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: Matching features..." );
  ::kwiver::vital::match_set_sptr matches = this->feature_matcher->match( i1_features, i1_descriptors ,
                                                                   i2_features, i2_descriptors );
  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: -- Number of matches: " << matches->size() );

  // Estimating homography describing transformation from src -> dest
  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: Estimating src->dest homography..." );
  std::vector<bool> inliers;
  ::kwiver::vital::homography_sptr mh_src2dst = this->homog_estimator->estimate( i1_features, i2_features,
                                                                     matches, inliers );
  // Reporting inlier count
  size_t inlier_count = 0;
  BOOST_FOREACH( bool b, inliers )
  {
    if( b )
    {
      ++inlier_count;
    }
  }
  LOG_TRACE( vital_logger, "maptk_shot_stitching_algo: -- Inliers / Matches: " << inlier_count
             << " / " << inliers.size() );

  // Only transform maptk->vidtk homography if there are a valid number of
  // inliers (i >= min_inliers).
  if( inlier_count < this->min_inliers_ )
  {
    LOG_WARN( vital_logger, "maptk_shot_stitching_algo: Insufficient inliers for stitching "
              "(require " << this->min_inliers_ <<
              ", found " << inlier_count << ")" );
  }
  else
  {
    // converting from MAP-Tk homography to VidTK homography
    image_to_image_homography::transform_t H_src2dst_transform;
    for( unsigned int i=0; i<3; ++i )
    {
      for( unsigned int j=0; j<3; ++j )
      {
        H_src2dst_transform.set( i, j, mh_src2dst->matrix()(i, j) );
      }
    }
    H_src2dst.set_transform( H_src2dst_transform );
  }

  return H_src2dst;
}


//============================================================================
// maptk_shot_stitching_algo
//----------------------------------------------------------------------------

template< typename PixType >
maptk_shot_stitching_algo<PixType>
::maptk_shot_stitching_algo()
  : impl_( new maptk_shot_stitching_algo_impl() )
{
  ::kwiver::vital::algorithm_plugin_manager::instance().register_plugins();

  // Perform a get/set cycle to expose surrounding parameters for default
  // algorithms.
  this->set_params( this->params() );
}


template< typename PixType >
maptk_shot_stitching_algo<PixType>
::~maptk_shot_stitching_algo()
{
}


template< typename PixType >
::vidtk::config_block
maptk_shot_stitching_algo<PixType>
::params() const
{
  return convert_config_block( impl_->params() );
}


template< typename PixType >
bool
maptk_shot_stitching_algo<PixType>
::set_params( config_block const &vc )
{
  // merge new parameters on top of current
  ::kwiver::vital::config_block_sptr mc = impl_->params();
  mc->merge_config( convert_config_block(vc) );

  impl_->min_inliers_ = mc->get_value( "min_inliers", impl_->min_inliers_ );

# define set_algo_configs(type, name) \
  type::set_nested_algo_configuration( #name, mc, impl_->name )

  impl_algos( set_algo_configs );

# undef set_algo_configs

  // Check for algorithm validity after set
  bool is_valid = true;

#define config_failure(msg)                              \
  LOG_WARN(vital_logger, "Failed algorithm parameter check: " << msg); \
  is_valid = false

#define check_algo_config(type, name)                                  \
  if (!type::check_nested_algo_configuration( #name, mc ))             \
  {                                                                    \
    config_failure("Configuration for algorithm '" << #name << "' " << \
                   "of type '" << #type << "' was invalid.");          \
  }

  impl_algos( check_algo_config );

#undef check_algo_config
#undef config_failure

  return is_valid;
}


// top-level stitch function specialization for vxl_byte imagery
template<>
image_to_image_homography
maptk_shot_stitching_algo<vxl_byte>
::stitch( vil_image_view < vxl_byte > const &src,
          vil_image_view < bool >     const &src_mask,
          vil_image_view < vxl_byte > const &dest,
          vil_image_view < bool >     const &dest_mask )
{
  // Native image type, no need for conversion
  return impl_->stitch( src, src_mask, dest, dest_mask );
}


// top-level stitch function specialization for vxl_uint_16 imagery
template <>
image_to_image_homography
maptk_shot_stitching_algo<vxl_uint_16>
::stitch( vil_image_view < vxl_uint_16 > const &src,
          vil_image_view < bool >        const &src_mask,
          vil_image_view < vxl_uint_16 > const &dest,
          vil_image_view < bool >        const &dest_mask )
{
  // Determine min/max value range across given given images, then convert to
  // vxl_byte for underlying function.
  vil_image_view<vxl_byte> src_stretched, dest_stretched;
  stretch_two_images( src, dest, src_stretched, dest_stretched );
  return impl_->stitch( src_stretched, src_mask,
                        dest_stretched, dest_mask );
}


} // end vidtk namespace
