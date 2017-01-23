/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

// Descriptor Includes
#include <descriptors/online_descriptor_computer_process.h>
#include <descriptors/tot_descriptor_generator.h>
#include <descriptors/icosahedron_hog_generator.h>
#include <descriptors/integral_hog_generator.h>
#include <descriptors/meds_generator.h>

#ifdef USE_VLFEAT
#include <descriptors/dsift_bow_generator.h>
#include <descriptors/mfeat_bow_generator.h>
#endif

#ifdef USE_OPENCV
#include <descriptors/simple_color_histogram_generator.h>
#include <descriptors/multi_color_descriptor_generator.h>
#include <descriptors/pvohog_generator.h>
#include <descriptors/dpm_tot_generator.h>
#endif

#ifdef USE_CAFFE
#include <descriptors/cnn_generator.h>
#endif

// Other VIDTK
#include <utilities/config_block.h>
#include <utilities/video_modality.h>
#include <logger/logger.h>

// Standard Lib
#include <string>
#include <fstream>
#include <exception>
#include <memory>

namespace vidtk
{

VIDTK_LOGGER ("online_descriptor_computer_process");

// Descriptor List Macros [Type, Name, Template]
#define VXL_DESCRIPTORS( MACRO ) \
  MACRO( tot_descriptor, tot_descriptor, ) \
  MACRO( icosahedron_hog, icosahedron_hog, <PixType> ); \
  MACRO( integral_hog, integral_hog, <PixType> ); \
  MACRO( meds, meds, ); \

#define OPENCV_DESCRIPTORS( MACRO ) \
  MACRO( simple_color_histogram, simple_color_histogram, <PixType> ); \
  MACRO( multi_color_descriptor, multi_color_descriptor, <PixType> ); \
  MACRO( pvohog, pvohog, ); \
  MACRO( pvohog, pvohog_2, ); \
  MACRO( dpm_tot, dpm_tot, ); \

#define VLFEAT_DESCRIPTORS( MACRO ) \
  MACRO( dsift_bow, dsift_bow, <PixType> ); \
  MACRO( dsift_bow, dsift_bow_2, <PixType> ); \
  MACRO( mfeat_bow, mfeat_bow, <PixType> ); \
  MACRO( mfeat_bow, mfeat_bow_2, <PixType> ); \
  MACRO( mfeat_bow, mfeat_bow_3, <PixType> ); \

#define CAFFE_DESCRIPTORS( MACRO ) \
  MACRO( cnn, cnn, <PixType> ); \
  MACRO( cnn, cnn_2, <PixType> ); \
  MACRO( cnn, cnn_3, <PixType> ); \


// Compile List of All Descriptors
#ifndef USE_OPENCV
  #undef OPENCV_DESCRIPTORS
  #define OPENCV_DESCRIPTORS( MACRO )
#endif

#ifndef USE_VLFEAT
  #undef VLFEAT_DESCRIPTORS
  #define VLFEAT_DESCRIPTORS( MACRO )
#endif

#ifndef USE_CAFFE
  #undef CAFFE_DESCRIPTORS
  #define CAFFE_DESCRIPTORS( MACRO )
#endif

#define ALL_DESCRIPTORS( MACRO ) \
  VXL_DESCRIPTORS( MACRO ) \
  OPENCV_DESCRIPTORS( MACRO ) \
  VLFEAT_DESCRIPTORS( MACRO ) \
  CAFFE_DESCRIPTORS( MACRO )


// Process Definition
template <typename PixType>
online_descriptor_computer_process<PixType>
::online_descriptor_computer_process( std::string const& _name )
  : process( _name, "online_descriptor_computer_process" ),
    disabled_( false ),
    use_ir_settings_( false ),
    was_last_eo_( true ),
    require_valid_timestamp_( false ),
    require_valid_image_( false ),
    minimum_shot_size_( 0 ),
    shot_size_counter_( 0 )
{
  config_.add_parameter( "disabled",
                         "false",
                         "Completely disable this process and pass no outputs." );
  config_.add_parameter( "thread_count",
                         "10",
                         "Number of threads to utilize." );
  config_.add_parameter( "safe_mode",
                         "true",
                         "Set to true if we want to run in safe mode, which checks "
                         "output descriptors for errors such as having NaN values "
                         "and making sure their reported history is continuous." );
  config_.add_parameter( "buffer_smart_ptrs_only",
                         "false",
                         "Set to true if we know that our inputs are thread safe and "
                         "are reallocated every frame." );
  config_.add_parameter( "use_ir_settings",
                         "false",
                         "Set to true if we want to use seperate config files for IR "
                         "mode, specified seperate from the EO settings." );
  config_.add_parameter( "require_valid_timestamp",
                         "false",
                         "If the input timestamp is invalid, should this process not "
                         "attempt to process any descriptors?" );
  config_.add_parameter( "require_valid_image",
                         "false",
                         "If the input image is invalid, should this process not "
                         "attempt to process any descriptors?" );
  config_.add_parameter( "minimum_shot_size",
                         "0",
                         "Minimum shot length before generating any descriptors for "
                         "this shot, set to 0 to disable." );

#define ADD_DESCRIPTOR_CONFIG( TYPE, NAME, TEMPLATE ) \
  config_.add_parameter( "enable_" #NAME , \
                         "false" , \
                         "Set to true, if we should generate " #NAME " descriptors." ); \
  config_.add_parameter( #NAME "_config_file", \
                         "", \
                         "Path to the " #NAME " config file." ); \
  config_.add_parameter( #NAME "_ir_config_file", \
                         "", \
                         "Path to the " #NAME " config file." ); \

  ALL_DESCRIPTORS( ADD_DESCRIPTOR_CONFIG )

#undef ADD_DESCRIPTOR_CONFIG

  //config_.add_subblock( descriptor_filter_settings().config(), "descriptor_filter" );
  config_.add_subblock( net_descriptor_computer_settings().config(), "net_computer" );

  // Reset  net computer
  net_computer_.reset( new net_descriptor_computer() );
}


template <typename PixType>
online_descriptor_computer_process<PixType>
::~online_descriptor_computer_process()
{
}


template <typename PixType>
config_block
online_descriptor_computer_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
online_descriptor_computer_process<PixType>
::set_params( config_block const& blk )
{
  // Declare all descriptor variables
#define ADD_DESCRIPTOR_VARIABLES( TYPE, NAME, TEMPLATE ) \
  bool enable_ ## NAME = false; \
  std::string NAME ## _config_file; \

  ALL_DESCRIPTORS( ADD_DESCRIPTOR_VARIABLES )

#undef ADD_DESCRIPTOR_VARIABLES

  // Reset joint generator
  joint_generator_.clear();

  // Load internal settings
  try
  {
    LOG_INFO( this->name() << ": Loading Parameters" );

    disabled_ = blk.get<bool>( "disabled" );

    if( !disabled_ )
    {
      LOG_INFO( this->name() << ": Loading Descriptor Parameters" );

      require_valid_timestamp_ = blk.get<bool>( "require_valid_timestamp" );
      require_valid_image_ = blk.get<bool>( "require_valid_image" );
      minimum_shot_size_ = blk.get<unsigned>( "minimum_shot_size" );

      joint_generator_settings_.thread_count = blk.get<unsigned>( "thread_count" );
      joint_generator_settings_.run_in_safe_mode = blk.get<bool>( "safe_mode" );
      joint_generator_settings_.buffer_smart_ptrs_only = blk.get<bool>( "buffer_smart_ptrs_only" );

#define LOAD_SETTINGS( TYPE, NAME, TEMPLATE ) \
      enable_ ## NAME = blk.get<bool>("enable_" #NAME ); \
      if( enable_ ## NAME ) \
      { \
        if( was_last_eo_ ) \
        { \
          NAME ## _config_file = blk.get<std::string>( #NAME "_config_file" ); \
        } \
        else \
        { \
          NAME ## _config_file = blk.get<std::string>( #NAME "_ir_config_file" ); \
        } \
      } \

      ALL_DESCRIPTORS( LOAD_SETTINGS )

#undef LOAD_SETTINGS

      //descriptor_filter_settings filter_settings( blk.subblock( "descriptor_filter" ) );
      net_descriptor_computer_settings net_computer_settings( blk.subblock( "net_computer" ) );

      //if( !filter_.configure( filter_settings ) )
      //{
      //  throw config_block_parse_error( "Unable to configure descriptor filter." );
      //}

      if( !net_computer_->configure( net_computer_settings ) )
      {
        throw config_block_parse_error( "Unable to configure net descriptor computer." );
      }
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Configure descriptors
  if( !disabled_ )
  {
    try
    {
      joint_generator_.configure( joint_generator_settings_ );

#define CONFIGURE_GENERATOR( TYPE, NAME, TEMPLATE )                     \
      if( enable_ ## NAME )                                             \
      {                                                                 \
        TYPE ## _generator TEMPLATE * NAME ## _generator_ptr = new TYPE ## _generator TEMPLATE; \
        LOG_INFO(this->name() << " - Configuring " #NAME );             \
        std::auto_ptr< external_settings > descriptor_settings( NAME ## _generator_ptr->create_settings() ); \
        if( NAME ## _config_file.size() > 0 )                           \
        {                                                               \
          if( !descriptor_settings->load_config_from_file( NAME ## _config_file ) ) \
          {                                                             \
            LOG_ERROR( this->name() << ": Unable to parse " <<  NAME ## _config_file ); \
            return false;                                               \
          }                                                             \
        }                                                               \
        if( ! NAME ## _generator_ptr->configure( *descriptor_settings ) ) \
        {                                                               \
          LOG_ERROR( this->name() << ": failed to configure " #NAME );  \
          return false;                                                 \
        }                                                               \
        generator_sptr NAME ## _generator_sptr = generator_sptr( NAME ## _generator_ptr ); \
        joint_generator_.add_generator( NAME ## _generator_sptr );      \
      }

    ALL_DESCRIPTORS( CONFIGURE_GENERATOR )

#undef CONFIGURE_GENERATOR

    }
    catch(...)
    {
      LOG_ERROR( this->name() << ": unable to configure descriptor generators." );
      return false;
    }
  }

  // Set internal config block
  config_.update( blk );

  // Create a new buffer to hold inputs
  inputs_ = frame_data_sptr( new frame_data() );

  return true;
}


template <typename PixType>
bool
online_descriptor_computer_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
bool
online_descriptor_computer_process<PixType>
::step()
{
  descriptors_.clear();

  if( disabled_ )
  {
    return true;
  }

  // If the option is enabled, check if the current timestamp is valid. If it is
  // not, output all outstanding descriptors and return false.
  if( ( require_valid_timestamp_ &&
        !( inputs_->was_timestamp_set() && inputs_->frame_timestamp().is_valid() &&
           inputs_->frame_timestamp() > last_timestamp_ ) ) ||
      ( require_valid_image_ &&
        !( inputs_->was_image_set() && inputs_->image<PixType>().size() > 0 ) ) )
  {
    LOG_INFO( "Not processing invalid or duplicate frame." );

    if( !joint_generator_.terminate_all_tracks() )
    {
      LOG_ERROR( "Unable to terminate all track descriptors." );
      return false;
    }

    this->collect_descriptors();
    return false;
  }

  if( require_valid_timestamp_ )
  {
    last_timestamp_ = inputs_->frame_timestamp();
  }

  // Check for modality changes if enabled
  if( use_ir_settings_ && inputs_->was_modality_set() )
  {
    bool is_eo = is_video_modality_eo( inputs_->modality() );

    // Trigger a shot reset and load new params
    if( is_eo != was_last_eo_ )
    {
      // Reset the process with the new settings
      joint_generator_.clear();
      this->collect_descriptors();
      was_last_eo_ = is_eo;

      // Keep this here - it's safe since it's not in set_params. It will reload
      // descriptors with their new parameters.
      this->set_params( config_ );
    }
  }

  // Check for minimum shot size
  bool process_frame = true;

  if( minimum_shot_size_ > 0 )
  {
    shot_size_counter_++;

    if( inputs_->was_modality_set() &&
        inputs_->shot_breaks().is_shot_end() )
    {
      shot_size_counter_ = 0;
    }

    if( shot_size_counter_ < minimum_shot_size_ )
    {
      process_frame = false;
    }
  }

  // Generate descriptors
  if( process_frame && !this->step_joint_generator() )
  {
#ifndef NDEBUG
    // Simply exit (an error was thrown, we should not continue).
    return false;
#else
    // Reset internal descriptors and attempt to continue processing.
    LOG_INFO( this->name() << ": attempting to restart failed descriptors." );

    if( !joint_generator_.reset_all() )
    {
      LOG_ERROR( this->name() << ": could not reset all descriptors." );
      return false;
    }
#endif
  }

  // Compute net descriptor
  net_computer_->compute( input_tracks_, descriptors_ );

  // Filter descriptors
  //filter_.filter( descriptors_ );

  // Reset inputs
  inputs_ = frame_data_sptr( new frame_data() );

  // Return success
  return true;
}

template <typename PixType>
bool
online_descriptor_computer_process<PixType>
::step_joint_generator()
{
  try
  {
    joint_generator_.set_input_frame( inputs_ );

    if( !joint_generator_.step() )
    {
      LOG_ERROR( this->name() << ": could not step generator!" );
      return false;
    }

    this->collect_descriptors();
  }
  catch( const boost::thread_interrupted& e )
  {
    LOG_INFO( this->name() << ": caught interrupt, exiting." );
    throw e;
  }
  catch( const std::exception& e )
  {
    LOG_ERROR( this->name() << ": received exception: " << e.what() );
    return false;
  }
  catch( ... )
  {
    LOG_ERROR( this->name() << ": received unknown exception." );
    return false;
  }

  return true;
}

template <typename PixType>
bool
online_descriptor_computer_process<PixType>
::flush()
{
  if( joint_generator_.terminate_all_tracks() )
  {
    this->collect_descriptors();
    return true;
  }

  return false;
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::collect_descriptors()
{
  const raw_descriptor::vector_t& genout = joint_generator_.get_descriptors();
  descriptors_.insert( descriptors_.end(), genout.begin(), genout.end() );
}

// Dumps a config file for each descriptor to some directory
template <typename PixType>
bool
online_descriptor_computer_process<PixType>
::dump_descriptor_configs( const std::string& directory )
{
  bool ret_val = true;

#define DUMP_DESCRIPTOR_CONFIG( TYPE, NAME, TEMPLATE ) \
  {                                                    \
    std::string out_fn = directory + "/" + #NAME ;     \
    std::ofstream out( out_fn.c_str() );               \
                                                       \
    if( !out )                                         \
    {                                                  \
      ret_val = false;                                 \
    }                                                  \
    else                                               \
    {                                                  \
      TYPE ## _settings config_to_dump;                \
      config_to_dump.config().print( out );            \
    }                                                  \
                                                       \
    out.close();                                       \
  }                                                    \

  ALL_DESCRIPTORS( DUMP_DESCRIPTOR_CONFIG )

#undef DUMP_DESCRIPTOR_CONFIG

  return ret_val;
}

// Accessor functions
template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_image( vil_image_view<PixType> const& image )
{
  inputs_->set_image( image );
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_tracks( std::vector<track_sptr> const& trks )
{
  input_tracks_ = trks;
  inputs_->set_active_tracks( trks );
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_metadata( video_metadata const& meta )
{
  inputs_->set_metadata( meta );
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_metadata_vector( std::vector< video_metadata > const& meta_vec )
{
  if( !meta_vec.empty() )
  {
    inputs_->set_metadata( meta_vec[ meta_vec.size()-1 ] );
  }
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  inputs_->set_timestamp( ts );
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_image_to_world_homography( image_to_utm_homography const& homog )
{
  if( homog.is_valid() )
  {
    inputs_->set_image_to_world_homography( homog.get_transform() );
  }
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_modality( video_modality modality )
{
  inputs_->set_modality( modality );
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_shot_break_flags( shot_break_flags sb_flags )
{
  inputs_->set_shot_break_flags( sb_flags );
}

template <typename PixType>
void
online_descriptor_computer_process<PixType>
::set_source_gsd( double gsd )
{
  inputs_->set_gsd( gsd );
}

template <typename PixType>
std::vector<track_sptr>
online_descriptor_computer_process<PixType>
::active_tracks() const
{
  return input_tracks_;
}

template <typename PixType>
raw_descriptor::vector_t
online_descriptor_computer_process<PixType>
::descriptors() const
{
  return descriptors_;
}

// Undef macros
#undef VXL_DESCRIPTORS
#undef VLFEAT_DESCRIPTORS
#undef OPENCV_DESCRIPTORS
#undef ALL_DESCRIPTORS


} // end namespace vidtk
