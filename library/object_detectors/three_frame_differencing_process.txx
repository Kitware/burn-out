/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "three_frame_differencing_process.h"

#include <vil/algo/vil_threshold.h>
#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <vil/vil_transform.h>

#include <video_transforms/warp_image.h>
#include <video_transforms/jittered_image_difference.h>
#include <video_transforms/zscore_image.h>

#include <boost/thread/thread.hpp>

#include <vnl/vnl_int_2.h>

#include <limits>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_three_frame_differencing_process_txx__
VIDTK_LOGGER( "three_frame_differencing_process_txx" );

namespace vidtk
{

template < typename PixType, typename FloatType >
three_frame_differencing_process<PixType, FloatType>
::three_frame_differencing_process( std::string const& _name )
  : process( _name, "three_frame_differencing_process" ),
    difference_flag_( true )
{
  config_.merge( frame_differencing_settings().config() );
}

template <typename PixType, typename FloatType>
three_frame_differencing_process<PixType, FloatType>
::~three_frame_differencing_process()
{
}

template <typename PixType, typename FloatType>
config_block
three_frame_differencing_process<PixType, FloatType>
::params() const
{
  return config_;
}


template <typename PixType, typename FloatType>
bool
three_frame_differencing_process<PixType, FloatType>
::set_params( config_block const& blk )
{
  try
  {
    frame_differencing_settings algorithm_settings;
    algorithm_settings.read_config( blk );

    if( !algorithm_.configure( algorithm_settings ) )
    {
      throw config_block_parse_error( "Unable to set algorithm options!" );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <typename PixType, typename FloatType>
bool
three_frame_differencing_process<PixType, FloatType>
::initialize()
{
  return true;
}


template <typename PixType, typename FloatType>
bool
three_frame_differencing_process<PixType, FloatType>
::reset()
{
  return initialize();
}

template <typename PixType, typename FloatType>
bool
three_frame_differencing_process<PixType, FloatType>
::step()
{
  // Reset outputs so we can output using only shadow copies
  diff_image_ = vil_image_view< FloatType >();
  fg_image_ = vil_image_view< bool >();

  // Process frames
  if( difference_flag_ )
  {
    if( !algorithm_.process_frames( stab_frame_1_, stab_frame_2_,
          stab_frame_3_, fg_image_, diff_image_ ) )
    {
      LOG_ERROR( this->name() << ": Unable to process frames!" );
      return false;
    }
  }
  else if( stab_frame_1_.size() > 0 )
  {
    fg_image_.set_size( stab_frame_1_.ni(), stab_frame_1_.nj() );
    fg_image_.fill( false );

    diff_image_.set_size( stab_frame_1_.ni(), stab_frame_1_.nj() );
    diff_image_.fill( -std::numeric_limits< float >::max() );
  }

  // Reset input images
  stab_frame_1_ = vil_image_view< PixType >();
  stab_frame_2_ = vil_image_view< PixType >();
  stab_frame_3_ = vil_image_view< PixType >();
  return true;
}

template < typename PixType, typename FloatType >
void
three_frame_differencing_process<PixType, FloatType>
::set_first_frame( vil_image_view<PixType> const& image )
{
  stab_frame_1_ = image;
}

template < typename PixType, typename FloatType >
void
three_frame_differencing_process<PixType, FloatType>
::set_second_frame( vil_image_view<PixType> const& image )
{
  stab_frame_2_ = image;
}

template < typename PixType, typename FloatType >
void
three_frame_differencing_process<PixType, FloatType>
::set_third_frame( vil_image_view<PixType> const& image )
{
  stab_frame_3_ = image;
}

template < typename PixType, typename FloatType >
void
three_frame_differencing_process<PixType, FloatType>
::set_difference_flag( bool flag )
{
  difference_flag_ = flag;
}

template < typename PixType, typename FloatType >
vil_image_view<bool>
three_frame_differencing_process<PixType, FloatType>
::fg_image() const
{
  return fg_image_;
}

template < typename PixType, typename FloatType >
vil_image_view<FloatType>
three_frame_differencing_process<PixType, FloatType>
::diff_image() const
{
  return diff_image_;
}

} // end namespace vidtk
