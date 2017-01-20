/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "integral_image_process.h"

#include <utilities/config_block.h>

#include <vil/vil_math.h>

#include <logger/logger.h>

VIDTK_LOGGER("integral_image_process_txx");

namespace vidtk
{

// Process Definition
template <typename InputPixType, typename OutputPixType>
integral_image_process<InputPixType, OutputPixType>
::integral_image_process( std::string const& _name )
  : process( _name, "integral_image_process" ),
    src_image_( NULL ),
    disabled_( false )
{
  config_.add_parameter( "disabled",
                         "false",
                         "Force disabling of the process" );
}


template <typename InputPixType, typename OutputPixType>
integral_image_process<InputPixType, OutputPixType>
::~integral_image_process()
{
}


template <typename InputPixType, typename OutputPixType>
config_block
integral_image_process<InputPixType, OutputPixType>
::params() const
{
  return config_;
}


template <typename InputPixType, typename OutputPixType>
bool
integral_image_process<InputPixType, OutputPixType>
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    disabled_ = blk.get<bool>( "disabled" );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  // Set internal config block
  config_.update( blk );

  return true;
}


template <typename InputPixType, typename OutputPixType>
bool
integral_image_process<InputPixType, OutputPixType>
::initialize()
{
  return true;
}


template <typename InputPixType, typename OutputPixType>
bool
integral_image_process<InputPixType, OutputPixType>
::step()
{
  // Perform basic validation of inputs
  if( disabled_ )
  {
    return true;
  }

  if( !src_image_ )
  {
    LOG_ERROR( this->name() << ": No valid input image provided!" );
    return false;
  }

  // Reset output buffer and calculate II
  output_ii_ = output_type();
  vil_math_integral_image( *src_image_, output_ii_ );

  // Reset input variables
  src_image_ = NULL;

  return true;
}

// Set internal states / get outputs of the process
template <typename InputPixType, typename OutputPixType>
void
integral_image_process<InputPixType, OutputPixType>
::set_source_image( vil_image_view<InputPixType> const& src )
{
  src_image_ = &src;
}

template <typename InputPixType, typename OutputPixType>
vil_image_view<OutputPixType>
integral_image_process<InputPixType, OutputPixType>
::integral_image() const
{
  return output_ii_;
}

} // end namespace vidtk
