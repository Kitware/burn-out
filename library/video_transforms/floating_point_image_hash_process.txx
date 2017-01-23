/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "floating_point_image_hash_process.h"
#include "floating_point_image_hash_functions.h"

#include <string>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("floating_point_image_hash_process");


template <typename FloatType, typename HashType>
floating_point_image_hash_process<FloatType, HashType>
::floating_point_image_hash_process( std::string const& _name )
  : process( _name, "floating_point_image_hash_process" ),
    algorithm_( SCALE_POSITIVE ),
    round_(true),
    param1_(static_cast<FloatType>(0.22)),
    param2_(static_cast<FloatType>(0.22))
{
  config_.add_parameter( "algorithm",
                         "SCALE_POSITIVE",
                         "Operating mode of the filter. The only option is currently SCALE_POSITIVE "
                         "which will threshold the input image then scale it by a fixed factor." );
  config_.add_parameter( "scale_factor",
                         "0.22",
                         "Scale factor." );
  config_.add_parameter( "max_input_value",
                         "10",
                         "The maximum input value we are interested in reporting in the output." );
}


template <typename FloatType, typename HashType>
config_block
floating_point_image_hash_process<FloatType, HashType>
::params() const
{
  return config_;
}


template<typename FloatType, typename HashType>
bool
floating_point_image_hash_process<FloatType, HashType>
::set_params( config_block const& blk )
{
  std::string alg_str;

  try
  {
    alg_str = blk.get<std::string>( "algorithm" );

    if( alg_str == "SCALE_POSITIVE" )
    {
      algorithm_ = SCALE_POSITIVE;
      param1_ = blk.get<FloatType>( "scale_factor" );
      param2_ = blk.get<FloatType>( "max_input_value" );
    }
    else
    {
      throw config_block_parse_error( "Invalid algorithm type specified" );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template<typename FloatType, typename HashType>
bool
floating_point_image_hash_process<FloatType, HashType>
::initialize()
{
  return true;
}

template<typename FloatType, typename HashType>
bool
floating_point_image_hash_process<FloatType, HashType>
::step()
{
  // Exit right away if we receive a don't process flag
  if( !input_img_ )
  {
    return false;
  }

  // Reset output image
  hashed_img_.clear();
  hashed_img_ = output_type();

  if( algorithm_ == SCALE_POSITIVE )
  {
    scale_float_img_to_interval( input_img_,
                                 hashed_img_,
                                 param1_,
                                 param2_ );
  }
  else
  {
    LOG_ERROR( this->name() << ": unknown algorithm specified!" );
    return false;
  }

  return true;
}

// Input and output accessor functions
template<typename FloatType, typename HashType>
void
floating_point_image_hash_process<FloatType, HashType>
::set_input_image( vil_image_view< FloatType > const& src )
{
  input_img_ = src;
}

template<typename FloatType, typename HashType>
vil_image_view<HashType>
floating_point_image_hash_process<FloatType, HashType>
::hashed_image() const
{
  return hashed_img_;
}


} // end namespace vidtk
