/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "bag_of_words_image_descriptor.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "bag_of_words_image_descriptor" );


template < typename PixType, typename WordType, typename MappingType >
bool
bag_of_words_image_descriptor< PixType, WordType, MappingType >
::load_vocabulary( const std::string& filename )
{
  this->vocabulary_.reset( new bag_of_words_model< WordType, MappingType >() );
  return this->vocabulary_->load_model( filename );
}

template < typename PixType, typename WordType, typename MappingType >
bool
bag_of_words_image_descriptor< PixType, WordType, MappingType >
::set_vocabulary( const vocabulary_type& vocab )
{
  this->vocabulary_.reset( new bag_of_words_model< WordType, MappingType >() );
  return this->vocabulary_->set_model( vocab );
}

template < typename PixType, typename WordType, typename MappingType >
bool
bag_of_words_image_descriptor< PixType, WordType, MappingType >
::is_vocabulary_valid()
{
  return this->vocabulary_ != NULL && this->vocabulary_->is_valid();
}

template < typename PixType, typename WordType, typename MappingType >
bag_of_words_model< WordType, MappingType > const&
bag_of_words_image_descriptor< PixType, WordType, MappingType >
::vocabulary()
{
  LOG_ASSERT( this->is_vocabulary_valid(), "Internal vocabulary is invalid!" );
  return *(this->vocabulary_);
}

} // end namespace vidtk
