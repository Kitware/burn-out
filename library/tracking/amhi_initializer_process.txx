/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "amhi_initializer_process.h"

#include <utilities/log.h>
#include <utilities/vsl/timestamp_io.h>
#include <utilities/unchecked_return_value.h>


namespace vidtk
{

template <class PixType>
amhi_initializer_process<PixType>
::amhi_initializer_process( vcl_string const& name )
  : process( name, "amhi_initializer_process" ),
    amhi_( NULL ),
    src_trks_( NULL ),
    image_buffer_( NULL ),
    enabled_( false )
{
  config_.add( "disabled", "true" );
  config_.add( "alpha", "0.1" );
  config_.add( "padding_factor", "0.5" );
  config_.add( "w_bbox_online", "0.1" );
  config_.add( "remove_zero_thr", "0.001" );
  config_.add( "max_valley_width", "0.3" );
  config_.add( "min_valley_depth", "0.2" );
  config_.add( "use_weights", "false" );
  config_.add( "max_bbox_side", "200" );
}

template <class PixType>
amhi_initializer_process<PixType>
::~amhi_initializer_process()
{
}


template <class PixType>
config_block
amhi_initializer_process<PixType>
::params() const
{
  return config_;
}

template < class PixType >
bool
amhi_initializer_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    enabled_ = !blk.get<bool>( "disabled" );

    if( enabled_ )
    {
      double alpha = blk.get<double>( "alpha" );
      double padding = blk.get<double>( "padding_factor" );
      double weight_bbox = blk.get<double>( "w_bbox_online" );
      double almost_zero = blk.get<double>( "remove_zero_thr" );
      double max_frac_valley_width = blk.get<double>( "max_valley_width" );
      double min_frac_valley_depth = blk.get<double>( "min_valley_depth" );
      bool use_weights = blk.get<bool>( "use_weights" );
      unsigned max_bbox_side = blk.get<unsigned>( "max_bbox_side" );

      amhi_ = new amhi<PixType>( alpha,
                                  padding,
                                  weight_bbox,
                                  almost_zero,
                                  max_frac_valley_width,
                                  min_frac_valley_depth,
                                  use_weights,
                                  max_bbox_side );
    }
  }
  catch( unchecked_return_value & )
  {
    // restore previous state
    this->set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}

template < class PixType >
bool
amhi_initializer_process<PixType>
::initialize()
{
  return true;
}

template < class PixType >
bool
amhi_initializer_process<PixType>
::reset()
{
  return initialize();
}


template < class PixType >
bool
amhi_initializer_process<PixType>
::step()
{
  log_assert( src_trks_ != NULL, "source tracks not provided" );

  out_trks_ = *src_trks_;

  if( !enabled_ )
    return true;

  typedef vcl_vector< track_sptr >::const_iterator iter_type;
  iter_type const end = out_trks_.end();

  //TODO:validate the image_buffer length here.

  //For every new track being intialized, compute the intial appearance
  //model (amhi).
  for( iter_type it = out_trks_.begin(); it != end; ++it )
  {
    amhi_->gen_init_model( *it , image_buffer_);
  }

  src_trks_ = NULL;

  return true;
}

template < class PixType >
vcl_vector<track_sptr> const&
amhi_initializer_process<PixType>
::out_tracks() const
{
  return out_trks_;
}

template < class PixType >
void
amhi_initializer_process<PixType>
::set_source_tracks( vcl_vector<track_sptr> const &trks)
{
  src_trks_ = &trks;
}

template < class PixType >
void
amhi_initializer_process<PixType>
::set_source_image_buffer( buffer< vil_image_view<PixType> > const& image_buf )
{
  image_buffer_ = &image_buf;
}

} // end namespace vidtk
