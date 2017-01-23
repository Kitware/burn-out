/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_consolidator_process.h"

#include <logger/logger.h>

#include <vil/vil_plane.h>

namespace vidtk
{

VIDTK_LOGGER( "image_consolidator_process" );


template<typename PixType>
image_consolidator_process<PixType>
::image_consolidator_process( std::string const& _name )
  : process( _name, "image_consolidator_process" )
{
  config_.add_parameter( "disabled",
    "false",
    "Should we not run this process?" );
}


template<typename PixType>
config_block
image_consolidator_process<PixType>
::params() const
{
  return config_;
}


template<typename PixType>
bool
image_consolidator_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template<typename PixType>
bool
image_consolidator_process<PixType>
::initialize()
{
  return true;
}


template<typename PixType>
bool
image_consolidator_process<PixType>
::step()
{
  // Exit right away if we're disabled
  if( disabled_ )
  {
    return true;
  }

  // Reset output to current input list (empty if not set)
  output_list_ = input_list_;

  // Perform consolidation process
  for( unsigned i = 0; i < MAX_INPUTS; i++ )
  {
    if( input_images_[i] )
    {
      if( input_images_[i].nplanes() == 1 )
      {
        output_list_.push_back( input_images_[i] );
      }
      else
      {
        for( unsigned p = 0; p < input_images_[i].nplanes(); p++ )
        {
          output_list_.push_back( vil_plane( input_images_[i], p ) );
        }
      }

      // Reset image for next step since we are done using it
      input_images_[i] = image_type();
    }
  }

  // Reset auxiliary input list and return
  input_list_.clear();
  return true;
}

// Input and output accessor functions
#define ADD_INPUT_IMAGE_FUNCTION( ID ) \
template<typename PixType> \
void \
image_consolidator_process<PixType> \
::set_image_ ## ID ( image_type const& img ) \
{ \
 input_images_[ ID - 1 ] = img; \
} \

ADD_INPUT_IMAGE_FUNCTION( 1 )
ADD_INPUT_IMAGE_FUNCTION( 2 )
ADD_INPUT_IMAGE_FUNCTION( 3 )
ADD_INPUT_IMAGE_FUNCTION( 4 )
ADD_INPUT_IMAGE_FUNCTION( 5 )
ADD_INPUT_IMAGE_FUNCTION( 6 )
ADD_INPUT_IMAGE_FUNCTION( 7 )
ADD_INPUT_IMAGE_FUNCTION( 8 )
ADD_INPUT_IMAGE_FUNCTION( 9 )
ADD_INPUT_IMAGE_FUNCTION( 10 )
ADD_INPUT_IMAGE_FUNCTION( 11 )
ADD_INPUT_IMAGE_FUNCTION( 12 )
ADD_INPUT_IMAGE_FUNCTION( 13 )
ADD_INPUT_IMAGE_FUNCTION( 14 )
ADD_INPUT_IMAGE_FUNCTION( 15 )
ADD_INPUT_IMAGE_FUNCTION( 16 )
ADD_INPUT_IMAGE_FUNCTION( 17 )
ADD_INPUT_IMAGE_FUNCTION( 18 )
ADD_INPUT_IMAGE_FUNCTION( 19 )
ADD_INPUT_IMAGE_FUNCTION( 20 )

#undef ADD_INPUT_IMAGE_FUNCTION

template<typename PixType>
void
image_consolidator_process<PixType>
::set_input_list( image_list_type const& list )
{
  input_list_ = list;
}

template<typename PixType>
std::vector< vil_image_view< PixType > >
image_consolidator_process<PixType>
::output_list() const
{
  return output_list_;
}


} // end namespace vidtk
