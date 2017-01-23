/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/frame_data.h>

#include <vil/vil_image_view.h>
#include <vil/vil_convert.h>
#include <vil/vil_math.h>

#include <boost/thread/locks.hpp>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_frame_data_cxx__
VIDTK_LOGGER("frame_data_cxx");

namespace vidtk
{

// Constructors
frame_data::frame_data()
 : gsd_( -1.0 )
{
  this->reset_set_flags();
}

frame_data::~frame_data()
{
}

void frame_data::reset_set_flags()
{
  was_any_image_set_ = false;
  was_tracks_set_ = false;
  was_metadata_set_ = false;
  was_timestamp_set_ = false;
  was_homography_set_ = false;
  was_modality_set_ = false;
  was_gsd_set_ = false;
  was_sb_set_ = false;
}

// Accessor functions
vidtk::track::vector_t const& frame_data::active_tracks() const
{
  LOG_ASSERT( was_tracks_set_, "Active tracks accessed, but not set" );
  return active_tracks_;
}

video_metadata const& frame_data::metadata() const
{
  LOG_ASSERT( was_metadata_set_, "Metadata accessed, but not set" );
  return metadata_;
}

vidtk::timestamp const& frame_data::frame_timestamp() const
{
  LOG_ASSERT( was_timestamp_set_, "Timestamp accessed, but not set" );
  return timestamp_;
}

vidtk::homography::transform_t const& frame_data::world_homography() const
{
  LOG_ASSERT( was_homography_set_, "Homography accessed, but not set" );
  return world_homog_;
}

vidtk::video_modality const& frame_data::modality() const
{
  LOG_ASSERT( was_modality_set_, "Modality accessed, but not set" );
  return modality_;
}

double frame_data::average_gsd() const
{
  LOG_ASSERT( was_gsd_set_, "GSD accessed, but not set" );
  return gsd_;
}

double frame_data::frame_time() const
{
  LOG_ASSERT( was_timestamp_set_, "Timestamp accessed, but not set" );
  return timestamp_.time();
}

unsigned int frame_data::frame_number() const
{
  LOG_ASSERT( was_timestamp_set_, "Timestamp accessed, but not set" );
  return timestamp_.frame_number();
}

vidtk::shot_break_flags const& frame_data::shot_breaks() const
{
  LOG_ASSERT( was_sb_set_, "Shot breaks accessed, but not set" );
  return sb_flags_;
}

// Set inputs
void frame_data::set_active_tracks( std::vector<track_sptr> const& tracks )
{
  was_tracks_set_ = true;
  active_tracks_ = tracks;
}

void frame_data::set_metadata( vidtk::video_metadata const& meta )
{
  was_metadata_set_ = true;
  metadata_ = meta;
}

void frame_data::set_timestamp( vidtk::timestamp const& ts )
{
  was_timestamp_set_ = true;
  timestamp_ = ts;
}

void frame_data::set_image_to_world_homography( vidtk::homography::transform_t const& homog )
{
  was_homography_set_ = true;
  world_homog_ = homog;
}

void frame_data::set_modality( vidtk::video_modality _modality )
{
  was_modality_set_ = true;
  modality_ = _modality;
}

void frame_data::set_shot_break_flags( vidtk::shot_break_flags sb_flags )
{
  was_sb_set_ = true;
  sb_flags_ = sb_flags;
}

void frame_data::set_gsd( double gsd )
{
  // Don't croak if the GSD is invalid (<= 0.0) just do nothing. One common
  // use case may be to send a stream of -1 GSDs when they're not available.
  if( gsd > 0 )
  {
    was_gsd_set_ = true;
    gsd_ = gsd;
  }
}

class frame_data::image_data
{

public:

  typedef boost::mutex mutex_t;
  typedef boost::unique_lock< mutex_t > lock_t;

  image_data();
  ~image_data() {}

  vil_image_view< vxl_byte > multi_channel_image_8u_;
  vil_image_view< vxl_byte > single_channel_image_8u_;
  vil_image_view< vxl_uint_16 > multi_channel_image_16u_;
  vil_image_view< vxl_uint_16 > single_channel_image_16u_;

  bool was_mc_8u_set_;
  bool was_sc_8u_set_;
  bool was_mc_16u_set_;
  bool was_sc_16u_set_;

  mutex_t compute_mutex_;

  void compute_mc_8u();
  void compute_sc_8u();
  void compute_mc_16u();
  void compute_sc_16u();

  void deep_copy( const frame_data::image_data& other );
};

frame_data::image_data::image_data()
{
  was_mc_8u_set_ = false;
  was_sc_8u_set_ = false;
  was_mc_16u_set_ = false;
  was_sc_16u_set_ = false;
}

#define LOCK_AND_CHECK_BOOLEAN( BVAR ) \
  lock_t( mutex_ ); \
  if( BVAR ) \
  { \
    return; \
  }

void upcast_to_16( const vil_image_view<vxl_byte>& src,
                   vil_image_view<vxl_uint_16>& dst )
{
  vil_convert_stretch_range_limited( src, dst,
                      static_cast<vxl_byte>(0),
                      static_cast<vxl_byte>(255),
                      static_cast<vxl_uint_16>(0),
                      static_cast<vxl_uint_16>(65535) );
}

template< typename PixType >
void convert_to_single_channel( const vil_image_view<PixType>& src,
                                vil_image_view<PixType>& dst )
{
  if( src.nplanes() == 3 )
  {
    vil_convert_planes_to_grey( src, dst );
  }
  else if( src.nplanes() == 1 )
  {
    dst = src;
  }
  else
  {
    vil_math_mean_over_planes( src, dst );
  }
}

void frame_data::image_data::compute_mc_8u()
{
  if( was_mc_16u_set_ )
  {
    LOCK_AND_CHECK_BOOLEAN( was_mc_8u_set_ );
    vil_convert_stretch_range( multi_channel_image_16u_, multi_channel_image_8u_ );
  }
  else
  {
    compute_sc_8u();
    LOCK_AND_CHECK_BOOLEAN( was_mc_8u_set_ );
    multi_channel_image_8u_ = single_channel_image_8u_;
  }
  was_mc_8u_set_ = true;
}

void frame_data::image_data::compute_sc_8u()
{
  if( was_mc_8u_set_ )
  {
    LOCK_AND_CHECK_BOOLEAN( was_sc_8u_set_ );
    convert_to_single_channel( multi_channel_image_8u_, single_channel_image_8u_ );
  }
  else
  {
    compute_sc_16u();
    LOCK_AND_CHECK_BOOLEAN( was_sc_8u_set_ );
    vil_convert_stretch_range( single_channel_image_16u_, single_channel_image_8u_ );
  }
  was_sc_8u_set_ = true;
}

void frame_data::image_data::compute_mc_16u()
{
  if( was_mc_8u_set_ )
  {
    LOCK_AND_CHECK_BOOLEAN( was_mc_16u_set_ );
    upcast_to_16( multi_channel_image_8u_, multi_channel_image_16u_ );
  }
  else
  {
    compute_sc_16u();
    LOCK_AND_CHECK_BOOLEAN( was_mc_16u_set_ );
    multi_channel_image_16u_ = single_channel_image_16u_;
  }
  was_mc_16u_set_ = true;
}

void frame_data::image_data::compute_sc_16u()
{
  if( was_mc_16u_set_ )
  {
    LOCK_AND_CHECK_BOOLEAN( was_sc_16u_set_ );
    convert_to_single_channel( multi_channel_image_16u_, single_channel_image_16u_ );
  }
  else
  {
    compute_sc_8u();
    LOCK_AND_CHECK_BOOLEAN( was_sc_16u_set_ );
    upcast_to_16( single_channel_image_8u_, single_channel_image_16u_ );
  }
  was_sc_16u_set_ = true;
}

void frame_data::image_data::deep_copy( const frame_data::image_data& other )
{
  was_sc_8u_set_ = other.was_sc_8u_set_;
  was_sc_16u_set_ = other.was_sc_16u_set_;
  was_mc_8u_set_ = other.was_mc_8u_set_;
  was_mc_16u_set_ = other.was_mc_16u_set_;

  if( was_sc_8u_set_ )
  {
    single_channel_image_8u_.deep_copy( other.single_channel_image_8u_ );
  }
  if( was_sc_16u_set_ )
  {
    single_channel_image_16u_.deep_copy( other.single_channel_image_16u_ );
  }
  if( was_mc_8u_set_ )
  {
    multi_channel_image_8u_.deep_copy( other.multi_channel_image_8u_ );
  }
  if( was_mc_16u_set_ )
  {
    multi_channel_image_16u_.deep_copy( other.multi_channel_image_16u_ );
  }
}

void frame_data::set_internal_image( const vil_image_view< vxl_byte >& img )
{
  set_image_8u( img );
}

void frame_data::set_internal_image( const vil_image_view< vxl_uint_16 >& img )
{
  set_image_16u( img );
}

vil_image_view< vxl_byte > const& frame_data::internal_image( vxl_byte ) const
{
  LOG_ASSERT( image_data_, "Image data accessed, but not set!" );
  if( !image_data_->was_mc_8u_set_ )
  {
    image_data_->compute_mc_8u();
  }
  return image_data_->multi_channel_image_8u_;
}

vil_image_view< vxl_uint_16 > const& frame_data::internal_image( vxl_uint_16 ) const
{
  LOG_ASSERT( image_data_, "Image data accessed, but not set!" );
  if( !image_data_->was_mc_16u_set_ )
  {
    image_data_->compute_mc_16u();
  }
  return image_data_->multi_channel_image_16u_;
}

void frame_data::set_image_8u( vil_image_view< vxl_byte > const& img )
{
  if( !image_data_ )
  {
    image_data_.reset( new image_data() );
  }

  if( img.nplanes() == 1 )
  {
    image_data_->single_channel_image_8u_ = img;
    image_data_->was_sc_8u_set_ = true;
  }

  image_data_->multi_channel_image_8u_ = img;
  image_data_->was_mc_8u_set_ = true;

  was_any_image_set_ = true;
}

void frame_data::set_image_16u( vil_image_view< vxl_uint_16 > const& img )
{
  if( !image_data_ )
  {
    image_data_.reset( new image_data() );
  }

  if( img.nplanes() == 1 )
  {
    image_data_->single_channel_image_16u_ = img;
    image_data_->was_sc_16u_set_ = true;
  }

  image_data_->multi_channel_image_16u_ = img;
  image_data_->was_mc_16u_set_ = true;

  was_any_image_set_ = true;
}

void frame_data::set_image_8u_gray( vil_image_view< vxl_byte > const& img )
{
  if( !image_data_ )
  {
    image_data_.reset( new image_data() );
  }

  image_data_->single_channel_image_8u_ = img;
  image_data_->was_sc_8u_set_ = true;

  was_any_image_set_ = true;
}

void frame_data::set_image_16u_gray( vil_image_view< vxl_uint_16 > const& img )
{
  if( !image_data_ )
  {
    image_data_.reset( new image_data() );
  }

  image_data_->single_channel_image_16u_ = img;
  image_data_->was_sc_16u_set_ = true;

  was_any_image_set_ = true;
}

vil_image_view< vxl_byte > const& frame_data::image_8u()
{
  LOG_ASSERT( image_data_, "Image data accessed, but not set!" );
  if( !image_data_->was_mc_8u_set_ )
  {
    image_data_->compute_mc_8u();
  }
  return image_data_->multi_channel_image_8u_;
}

vil_image_view< vxl_uint_16 > const& frame_data::image_16u()
{
  LOG_ASSERT( image_data_, "Image data accessed, but not set!" );
  if( !image_data_->was_mc_16u_set_ )
  {
    image_data_->compute_mc_16u();
  }
  return image_data_->multi_channel_image_16u_;
}

vil_image_view< vxl_byte > const& frame_data::image_8u_gray()
{
  LOG_ASSERT( image_data_, "Image data accessed, but not set!" );
  if( !image_data_->was_sc_8u_set_ )
  {
    image_data_->compute_sc_8u();
  }
  return image_data_->single_channel_image_8u_;
}

vil_image_view< vxl_uint_16 > const& frame_data::image_16u_gray()
{
  LOG_ASSERT( image_data_, "Image data accessed, but not set!" );
  if( !image_data_->was_sc_16u_set_ )
  {
    image_data_->compute_sc_16u();
  }
  return image_data_->single_channel_image_16u_;
}

void frame_data::deep_copy( const frame_data& other )
{
  *this = other;

  if( other.was_any_image_set_ )
  {
    this->image_data_.reset( new image_data() );
    this->image_data_->deep_copy( *other.image_data_ );
  }
}

#undef LOCK_AND_CHECK_BOOLEAN

} // end namespace vidtk
