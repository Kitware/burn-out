/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "maptk_tracking_process.h"

#include <map>
#include <utility>

#include <boost/foreach.hpp>

#include <vital/algo/compute_ref_homography.h>
#include <vital/algo/convert_image.h>
#include <vital/algo/track_features.h>
#include <vital/algorithm_plugin_manager.h>
#include <vital/config/config_block.h>
#include <vital/types/homography.h>
#include <vital/types/track_set.h>
#include <vital/vital_types.h>

#include <vil/vil_transform.h>

#include <maptk/plugins/vxl/image_container.h>

static ::kwiver::vital::logger_handle_t vital_logger( ::kwiver::vital::get_logger( "maptk_shot_stitching_algo" ) );

namespace vidtk
{
// ===========================================================================
// Helper methods
//  -> should be put in common utilities file (currently copy/pasted in places)
// ---------------------------------------------------------------------------
namespace // anon
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
    // VidTK config block doesn't like it when you try to get an empty value string
    mv = p.second.has_value() ? static_cast< ::kwiver::vital::config_block_value_t >(p.second.value())
                              : "";
    md = static_cast< ::kwiver::vital::config_block_description_t >(p.second.description());

    mc->set_value(mk, mv, md);
  }

  return mc;
}

/// Functor for mask image inversion transformation
template< class inT, class outT >
class inverse_functor
{
public:

  outT operator()(inT const &v)
  {
    return static_cast<outT>( !v );
  }

};

} // end anon namespace


// ===========================================================================
// Private Implementation Class
// ---------------------------------------------------------------------------

// Shortcut macro for arbitrarilly acting over the impl's algorithm elements
// ``call`` macro must take two arguments: (algo_type, algo_name)
// ``name`` is used for both member variable and configuration block.
#define impl_algos(call)                                                \
  /*      type                                    name               */   \
  call( ::kwiver::vital::algo::compute_ref_homography,  ref_homog_computer ); \
  call( ::kwiver::vital::algo::convert_image,           image_converter ); \
  call( ::kwiver::vital::algo::track_features,          feature_tracker )

class maptk_tracking_process::pimpl
{
public: // Attributes

  typedef std::map< ::kwiver::vital::track_id_t, klt_track_ptr > klt_track_map_t;
  typedef klt_track_map_t::iterator klt_track_map_iter_t;

  //+ TODO  - this cache crows without bound at one entry per frame.
  // Needs to be cleaned up or use another structure
  typedef std::map< ::kwiver::vital::frame_id_t, timestamp > timestamp_cache_t;

  // MAP-Tk algorithms
#define def_algos(type, name) \
  type ## _sptr name

  impl_algos( def_algos );

#undef def_algos

  /// Tracks generated from MAP-Tk tracking
  ::kwiver::vital::track_set_sptr tracks_;
  /// Cache of converted KLT tracks for extension and propagation
  klt_track_map_t klt_track_map_;
  /// Cache of timestamp objects for use in reference setting
  timestamp_cache_t timestamp_cache_;

  /// Was the last frame a new ref frame?
  bool prev_frame_was_new_ref_;

  // Input value holders
  timestamp                   source_time_;
  vil_image_view< vxl_byte >  source_image_;
  vil_image_view< bool >      source_mask_;

  // Output value holders
  image_to_image_homography     output_src_to_ref_H_;
  std::vector< klt_track_ptr >  output_active_tracks_;
  shot_break_flags              output_sb_flags_;

public: // Functions

  /// constructor
  /**
   * TODO: Add default value for mask for when its not provided and vital
   *        feature tracking actually uses it
   */
  pimpl();
  /// destructor
  ~pimpl();

  /// vital config block generator for current state of pimpl
  ::kwiver::vital::config_block_sptr params() const;
  bool set_params( ::kwiver::vital::config_block_sptr c );

  /// Reset state to initial conditions
  /**
   * Return false if algorithms fail to configure. Otherwise true.
   */
  bool reset();

};


// ------------------------------------------------------------------
/// Constructor
maptk_tracking_process::pimpl
::pimpl()
{
  this->reset();
}


/// Destructor
maptk_tracking_process::pimpl
::~pimpl()
{
}


// ------------------------------------------------------------------
// MAPTk config block generator for current state of pimpl
::kwiver::vital::config_block_sptr
maptk_tracking_process::pimpl
::params() const
{
  ::kwiver::vital::config_block_sptr mc = ::kwiver::vital::config_block::empty_config();

  // get configuration from implementation algorithms
#define get_algo_config(type, name) \
  type::get_nested_algo_configuration( #name, mc, this->name )

  impl_algos( get_algo_config );

#undef get_algo_config

  // Fill in default parameters if an implementation algorithm is not yet
  // initialized
  unsigned const default_min_inliers = 12;
  unsigned const default_min_track_len = 2;
  if ( !ref_homog_computer )
  {
    mc->set_value( "ref_homog_computer:type", "core" );
    mc->set_value( "ref_homog_computer:core:estimator:type", "vxl" );
    mc->set_value( "ref_homog_computer:core:min_matches_threshold", default_min_inliers );
    mc->set_value( "ref_homog_computer:core:min_track_length", default_min_track_len );
    mc->set_value( "ref_homog_computer:core:use_backproject_error", true );
    mc->set_value( "ref_homog_computer:core:allow_ref_frame_regression", false );
  }
  if ( !image_converter )
  {
    mc->set_value( "image_converter:type", "bypass" );
  }
  if ( !feature_tracker )
  {
    mc->set_value( "feature_tracker:type", "core" );

    mc->set_value( "feature_tracker:core:feature_detector:type", "ocv_SURF" );
    mc->set_value( "feature_tracker:core:feature_detector:ocv_SURF:extended", "false" );
    mc->set_value( "feature_tracker:core:feature_detector:ocv_SURF:hessian_threshold", 1000 );
    mc->set_value( "feature_tracker:core:feature_detector:ocv_SURF:n_octave_layers", 3 );
    mc->set_value( "feature_tracker:core:feature_detector:ocv_SURF:upright", true );

    mc->set_value( "feature_tracker:core:descriptor_extractor:type", "ocv_SURF" );
    mc->set_value( "feature_tracker:core:descriptor_extractor:ocv_SURF:extended", "false" );
    mc->set_value( "feature_tracker:core:descriptor_extractor:ocv_SURF:hessian_threshold", 1000 );
    mc->set_value( "feature_tracker:core:descriptor_extractor:ocv_SURF:n_octave_layers", 3 );
    mc->set_value( "feature_tracker:core:descriptor_extractor:ocv_SURF:upright", true );

    mc->set_value( "feature_tracker:core:feature_matcher:type", "homography_guided" );
    mc->set_value( "feature_tracker:core:feature_matcher:homography_guided:feature_matcher1:type", "ocv_flann_based" );
    mc->set_value( "feature_tracker:core:feature_matcher:homography_guided:feature_matcher1:ocv_flann_based:cross_check", "true" );
    mc->set_value( "feature_tracker:core:feature_matcher:homography_guided:feature_matcher1:ocv_flann_based:cross_check_k", "1" );
    mc->set_value( "feature_tracker:core:feature_matcher:homography_guided:homography_estimator:type", "vxl" );
    mc->set_value( "feature_tracker:core:feature_matcher:homography_guided:inlier_scale", 10 );
    mc->set_value( "feature_tracker:core:feature_matcher:homography_guided:min_required_inlier_count", 0 );
    mc->set_value( "feature_tracker:core:feature_matcher:homography_guided:min_required_inlier_percent", 0 );

    mc->set_value( "feature_tracker:core:loop_closer:type", "bad_frames_only" );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:enabled", false );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:percent_match_req", 0.35 );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:type", "homography_guided" );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:homography_guided:feature_matcher1:type", "ocv_flann_based" );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:homography_guided:feature_matcher1:ocv_flann_based:cross_check", "true" );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:homography_guided:feature_matcher1:ocv_flann_based:cross_check_k", "1" );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:homography_guided:homography_estimator:type", "vxl" );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:homography_guided:inlier_scale", 10 );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:homography_guided:min_required_inlier_count", 0 );
    mc->set_value( "feature_tracker:core:loop_closer:bad_frames_only:feature_matcher:homography_guided:min_required_inlier_percent", 0 );
  }

  return mc;
}


// ------------------------------------------------------------------
bool
maptk_tracking_process::pimpl
::set_params( ::kwiver::vital::config_block_sptr c )
{
  // Set configuration for all implementation algorithms
#define set_algo(type, name) \
  type::set_nested_algo_configuration( #name, c, this->name )

  impl_algos( set_algo );

#undef reset_algo

  // Check for post-set algorithm validity
  bool is_valid = true;

#define config_failure(msg)                                             \
  LOG_WARN( vital_logger, "maptk_tracking_process: Failed algorithm parameter check: " << msg); \
  is_valid = false

#define check_algo_config(type, name)                                  \
  if (!type::check_nested_algo_configuration( #name, c ))              \
  {                                                                    \
    config_failure("Configuration for algorithm '" << #name << "' " << \
                   "of type '" << #type << "' was invalid.");          \
  }

  impl_algos( check_algo_config );

#undef check_algo_config
#undef config_failure

  return is_valid;
}


// ------------------------------------------------------------------
/// Reset state to initial conditions
bool
maptk_tracking_process::pimpl
::reset()
{
  tracks_ = ::kwiver::vital::track_set_sptr( new ::kwiver::vital::simple_track_set() );
  klt_track_map_ = klt_track_map_t();
  timestamp_cache_ = timestamp_cache_t();

  // On the first frame through the system, if we pretend like the -1st frame
  // was a new reference, then the first actual frame does not get a shot_end
  // flag.
  prev_frame_was_new_ref_ = true;

  // Reset algorithm states by getting the current configuration, resetting
  // sptrs to NULL, and calling nested set config methods.
#define reset_algo(type, name) \
    this->name = type##_sptr(); \
  impl_algos( reset_algo );
#undef reset_algo

  return this->set_params( this->params() );
}

// ===========================================================================
// maptk_tracking_process implementation
// ---------------------------------------------------------------------------

maptk_tracking_process
::maptk_tracking_process( std::string const &inst_name )
  : process( inst_name, "maptk_tracking_process" )
  , impl_( new maptk_tracking_process::pimpl() )
{
  ::kwiver::vital::algorithm_plugin_manager::instance().load_plugins_once();

  // Reset private impl (constructs algos with defaults for parameter exposure)
  impl_->reset();
}


maptk_tracking_process
::~maptk_tracking_process()
{
  delete impl_;
}


// ------------------------------------------------------------------
config_block
maptk_tracking_process
::params() const
{
  LOG_DEBUG( vital_logger, name() << ": Getting parameters" );
  return convert_config_block( impl_->params() );
}


// ------------------------------------------------------------------
bool
maptk_tracking_process
::set_params(config_block const &vc )
{
  LOG_DEBUG( vital_logger, name() << ": Setting parameters" );

  // merge new parameters on top of current
  ::kwiver::vital::config_block_sptr mc = impl_->params();
  mc->merge_config( convert_config_block(vc) );

  return impl_->set_params( mc );
}


// ------------------------------------------------------------------
bool
maptk_tracking_process
::initialize()
{
  return impl_->reset();
}


// ------------------------------------------------------------------
bool
maptk_tracking_process
::reset()
{
  return this->initialize();
}


// ------------------------------------------------------------------
process::step_status
maptk_tracking_process
::step2()
{
  LOG_DEBUG( vital_logger, name() << ": Starting step" );

  // Variables for data generated this step
  // Will overwrite into output holders
  image_to_image_homography src_to_ref_h;
  shot_break_flags sb_flags;
  std::vector< klt_track_ptr > active_klt_tracks;

  // Convert input data into MAP-Tk usable data
  ::kwiver::vital::frame_id_t frame_num = impl_->source_time_.frame_number();
  ::kwiver::vital::image_container_sptr source_ic( new ::kwiver::maptk::vxl::image_container( impl_->source_image_ ) );
  // TODO: When we get to using the mask, invert into MAPTk desired mask format

  // Update timestamp cache
  impl_->timestamp_cache_[frame_num] = impl_->source_time_;

  // Convert mask to MAP-Tk format if set
  ::kwiver::vital::image_container_sptr maptk_mask;

  if( impl_->source_mask_ )
  {
    vil_image_view<vxl_byte> mask_inv;
    inverse_functor<bool, vxl_byte> inv_f;
    vil_transform( impl_->source_mask_, mask_inv, inv_f );
    maptk_mask = ::kwiver::vital::image_container_sptr(
      new ::kwiver::maptk::vxl::image_container( mask_inv ) );
  }

  // MAP-Tk tracking and src-to-ref homography estimation
  // - Loop closure is turned off (by default), so every source-to-ref
  //   identity matrix to be a new-reference, i.e. start of a new shot.
  LOG_DEBUG( vital_logger, name() << ": performing feature tracking and s2r-H estimation" );
  source_ic = impl_->image_converter->convert( source_ic );

  if( impl_->source_mask_ )
  {
    impl_->tracks_ = impl_->feature_tracker->track( impl_->tracks_, frame_num, source_ic, maptk_mask );
  }
  else
  {
    impl_->tracks_ = impl_->feature_tracker->track( impl_->tracks_, frame_num, source_ic );
  }

  ::kwiver::vital::f2f_homography_sptr s2r_h = impl_->ref_homog_computer->estimate( frame_num, impl_->tracks_ );

  // convert maptk homog to vidtk homog
  // There are no invalid homographies. If there is a stabilization break,
  // the new frame will stabilize with itself and return identity homography
  image_to_image_homography::transform_t src_to_ref_h_transform;
  auto matrix = s2r_h->homography()->matrix();
  for( unsigned int i = 0; i < 3; ++i )
  {
    for( unsigned int j = 0; j < 3; ++j )
    {
      src_to_ref_h_transform.set( i, j, matrix(i, j) );
    }
  }
  src_to_ref_h.set_transform( src_to_ref_h_transform );  // sets valid to true
  src_to_ref_h.set_source_reference( impl_->timestamp_cache_[ s2r_h->from_id() ] );
  src_to_ref_h.set_dest_reference( impl_->timestamp_cache_[ s2r_h->to_id() ] );
  src_to_ref_h.set_new_reference( s2r_h->from_id() == s2r_h->to_id() );

  // Delete up to but not including dest ref element from map
  // This is needed to keep the cache from growing without bound
  auto last_used = impl_->timestamp_cache_.lower_bound( s2r_h->to_id() );
  impl_->timestamp_cache_.erase( impl_->timestamp_cache_.begin(), last_used );

  // All frames are initially "usable", as they at least stabilize to themselves
  sb_flags.set_frame_usable( true );
  // New reference and end of current shot with an identity matrix
  // TODO: MAPTK might need an is_valid() flag in the homography object to
  //       differentiate between an invalid estimation and a valid estimation
  //       that happens to be an identity matrix.
  if ( src_to_ref_h_transform.is_identity() )
  {
    src_to_ref_h.set_new_reference( true );

    // Shot end flags should be assigned only to the frame after the last valid
    // frame of a shot, just like the EOF flag occurs after the last valid
    // character in a file. Valid in this case means the last frame had a
    // non-identity homography, meaning it was successfully referenced back
    if ( ! impl_->prev_frame_was_new_ref_ )
    {
      sb_flags.set_shot_end( true );
    }

    impl_->prev_frame_was_new_ref_ = true;
  }
  else
  {
    // "prev" frame, i.e. this frame from the perspective of the next step
    impl_->prev_frame_was_new_ref_ = false;
  }

  // --- BACKWARDS COMPAT ---
  // Update KLT track map with current frame active track states in order to
  //    keep things in sync
  // + Create vector of active KLT tracks (those just updated)
  ::kwiver::vital::track_set_sptr m_active_tracks = impl_->tracks_->active_tracks( frame_num );
  BOOST_FOREACH( ::kwiver::vital::track_sptr t, m_active_tracks->tracks() )
  {
    // because its an active track, its last frame is guarenteed to be the current frame
    ::kwiver::vital::track::track_state t_last_state = *(t->find( frame_num ));

    klt_track::point_ klt_pt;
    klt_pt.x = t_last_state.feat->loc().x();
    klt_pt.y = t_last_state.feat->loc().y();
    klt_pt.frame = t_last_state.frame_id;

    // Define the klt track tail to either be the existing klt track for this
    //    track's ID or 0 (new klt track)
    // If the current track is only of size 1, it is new. Otherwise, we have
    //    seen this track before and should have reference for this track's ID.
    klt_track_ptr tail;
    if ( t->size() > 1 )
    {
      tail = impl_->klt_track_map_[t->id()];
    }

    impl_->klt_track_map_[t->id()] = klt_track::extend_track( klt_pt, tail );
    active_klt_tracks.push_back( impl_->klt_track_map_[t->id()] );
  } // end foreach

  // Populate output holders
  impl_->output_src_to_ref_H_ = src_to_ref_h;
  impl_->output_active_tracks_ = active_klt_tracks;
  impl_->output_sb_flags_ = sb_flags;

  LOG_INFO( vital_logger, name()
            << " :: " << impl_->source_time_
            << " :: " << impl_->output_sb_flags_
            << " :: " << impl_->output_src_to_ref_H_ );

  return process::SUCCESS;
}


// ------------------------------------------------------------------
void
maptk_tracking_process
::set_timestamp( timestamp const &ts )
{
  impl_->source_time_ = ts;
}


// ------------------------------------------------------------------
void
maptk_tracking_process
::set_image( vil_image_view<vxl_byte> const &img )
{
  impl_->source_image_ = img;
}


// ------------------------------------------------------------------
void
maptk_tracking_process
::set_mask( vil_image_view<bool> const &mask )
{
  impl_->source_mask_ = mask;
}


// ------------------------------------------------------------------
image_to_image_homography
maptk_tracking_process
::src_to_ref_homography() const
{
  return impl_->output_src_to_ref_H_;
}


// ------------------------------------------------------------------
image_to_image_homography
maptk_tracking_process
::ref_to_src_homography() const
{
  return impl_->output_src_to_ref_H_.get_inverse();
}


// ------------------------------------------------------------------
std::vector< klt_track_ptr >
maptk_tracking_process
::active_tracks() const
{
  return impl_->output_active_tracks_;
}


// ------------------------------------------------------------------
shot_break_flags
maptk_tracking_process
::get_output_shot_break_flags() const
{
  return impl_->output_sb_flags_;
}

} // end vidtk namespace
