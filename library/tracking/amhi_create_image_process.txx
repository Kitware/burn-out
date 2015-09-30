/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "amhi_create_image_process.h"

#include <utilities/vsl/timestamp_io.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vil/vil_crop.h>
#include <vil/vil_convert.h>
#include <vil/vil_math.h>

namespace vidtk
{


template <class PixType>
amhi_create_image_process<PixType>
::amhi_create_image_process( vcl_string const& name)
  : process( name, "amhi_create_image_process" ),
    src_trks_( NULL ),
    src_img_( NULL ),
    enabled_(false)
{
  config_.add_parameter( "disabled", "true" , "Generates the amhi image." );
}

template <class PixType>
amhi_create_image_process<PixType>
::~amhi_create_image_process()
{
}

template <class PixType>
config_block
amhi_create_image_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
amhi_create_image_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    enabled_ = !blk.get< bool >( "disabled" );
  }
  catch( unchecked_return_value const& e )
  {
    log_error( name() << ": " << e.what() << "\n" );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
amhi_create_image_process<PixType>
::initialize()
{
  return true;
}

template <class PixType>
bool
amhi_create_image_process<PixType>
::step()
{
  // Check to see if all required inputs are present.
  // Terminate if not.
  if ( (src_trks_ == NULL)
       || ( src_img_ == NULL)
       || ( cur_ts_ == NULL)
    )
  {
    return false;
  }

  if( !enabled_ )
  {
    return true;
  }

  amhi_image_.set_size( src_img_->ni(), src_img_->nj());
  amhi_image_.fill( 0 ) ;

  for( unsigned i = 0; i < src_trks_->size(); i++ )
  {

    //if updated in current frame.

    if( (*src_trks_)[i]->last_state()->time_ == *cur_ts_ )
    {
      vil_image_view<float> const & W = (*src_trks_)[i]->amhi_datum().weight;
      vgl_box_2d<unsigned> const & bbox = (*src_trks_)[i]->amhi_datum().bbox;

      vil_image_view<vxl_byte> bbox_view = vil_crop( amhi_image_,
                                                     bbox.min_x(),
                                                     bbox.width(),
                                                     bbox.min_y(),
                                                     bbox.height() );


      vil_image_view<vxl_byte> W_0_255;
      W_0_255.set_size( bbox_view.ni(), bbox_view.nj() );

      //[0,1] --> [0,255]
      vil_convert_stretch_range( W, W_0_255);

      vil_image_view<vxl_byte> merged_view;
      vil_math_image_max( W_0_255, bbox_view, merged_view );

      bbox_view.deep_copy( merged_view );
    }
  }

  src_trks_ = NULL;

  return true;
}


template <class PixType>
void
amhi_create_image_process<PixType>
::set_enabled( bool const &start)
{
  enabled_ = start;
}

template <class PixType>
void
amhi_create_image_process<PixType>
::set_source_tracks( vcl_vector<track_sptr> const &trks)
{
  src_trks_ = &trks;
}

template <class PixType>
vil_image_view<vxl_byte> const&
amhi_create_image_process<PixType>
::amhi_image() const
{
  return amhi_image_;
}

template <class PixType>
void
amhi_create_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& img)
{
  src_img_ = &img;
}

template <class PixType>
void
amhi_create_image_process<PixType>
::set_timestamp( timestamp const& ts )
{
  cur_ts_ = &ts;
}

} // end namespace vidtk
