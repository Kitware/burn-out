/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "osd_recognizer_process.h"

#include <video_transforms/floating_point_image_hash_functions.h>
#include <video_transforms/automatic_white_balancing.h>

#include <utilities/config_block.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER("osd_recognizer_process");

// Process Definition
template <typename PixType>
osd_recognizer_process<PixType>
::osd_recognizer_process( std::string const& _name )
  : process( _name, "osd_recognizer_process" )
{
  config_.merge( osd_recognizer_settings().config() );
}


template <typename PixType>
osd_recognizer_process<PixType>
::~osd_recognizer_process()
{
}


template <typename PixType>
config_block
osd_recognizer_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
osd_recognizer_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    osd_recognizer_settings settings( blk );

    if( !recognizer_.configure( settings ) )
    {
      throw config_block_parse_error( "Cannot configure recognizer" );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Set internal config block
  config_.update( blk );

  return true;
}


template <typename PixType>
bool
osd_recognizer_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
bool
osd_recognizer_process<PixType>
::step()
{
  // Perform Basic Validation
  if( !classified_image_ )
  {
    LOG_ERROR( this->name() << ": No valid input image provided!" );
    return false;
  }
  if( classified_image_.nplanes() > 2 )
  {
    LOG_ERROR( this->name() << ": Classifier input has wrong number of channels!" );
    return false;
  }

  // Reset output mask for async mode
  output_mask_ = vil_image_view<bool>();
  text_instructions_.clear();
  dynamic_instruction_set_1_.clear();
  dynamic_instruction_set_2_.clear();

  // Perform recognization algorithm
  osd_properties_ = recognizer_.process_frame(
      source_image_, features_, border_, classified_image_, properties_, output_mask_ );

  // Retrieve outputs of algorithm, if present
  if( osd_properties_ )
  {
    if( osd_properties_->actions )
    {
      text_instructions_ = osd_properties_->actions->parse_instructions;
      dynamic_instruction_set_1_ = osd_properties_->actions->dyn_instruction_1;
      dynamic_instruction_set_2_ = osd_properties_->actions->dyn_instruction_2;
    }

    detected_type_ = osd_properties_->type;
  }

  return true;
}


// Accessor functions
template <typename PixType>
bool
osd_recognizer_process<PixType>
::text_parsing_enabled() const
{
  return recognizer_.contains_text_option();
}


template <typename PixType>
void
osd_recognizer_process<PixType>
::set_classified_image( vil_image_view<double> const& src )
{
  classified_image_ = src;
}

template <typename PixType>
void
osd_recognizer_process<PixType>
::set_mask_properties( mask_properties_type const& props )
{
  properties_ = props;
}


template <typename PixType>
void
osd_recognizer_process<PixType>
::set_source_image( vil_image_view<PixType> const& src )
{
  source_image_ = src;
}


template <typename PixType>
void
osd_recognizer_process<PixType>
::set_border( vgl_box_2d<int> const& rect )
{
  border_ = rect;
}


template <typename PixType>
void
osd_recognizer_process<PixType>
::set_input_features( feature_array const& array )
{
  features_ = array;
}


template <typename PixType>
vil_image_view<bool>
osd_recognizer_process<PixType>
::mask_image() const
{
  return output_mask_;
}


template <typename PixType>
std::string
osd_recognizer_process<PixType>
::detected_type() const
{
  return detected_type_;
}


template <typename PixType>
boost::shared_ptr< osd_recognizer_output< PixType, vxl_byte > >
osd_recognizer_process<PixType>
::output() const
{
  return osd_properties_;
}


template <typename PixType>
std::vector< text_parser_instruction< PixType > >
osd_recognizer_process<PixType>
::text_instructions() const
{
  return text_instructions_;
}


template <typename PixType>
std::string
osd_recognizer_process<PixType>
::dynamic_instruction_set_1() const
{
  return dynamic_instruction_set_1_;
}


template <typename PixType>
std::string
osd_recognizer_process<PixType>
::dynamic_instruction_set_2() const
{
  return dynamic_instruction_set_2_;
}


} // end namespace vidtk
