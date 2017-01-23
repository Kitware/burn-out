/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "raw_descriptor.h"

#include <vbl/vbl_smart_ptr.hxx>
#include <sstream>


namespace vidtk
{

// Factory methods
descriptor_sptr
raw_descriptor
::create( std::string const& type )
{
  descriptor_sptr output( new raw_descriptor() );
  output->type_ = type;
  return output;
}


descriptor_sptr
raw_descriptor
::create( descriptor_sptr to_copy )
{
  descriptor_sptr output( new raw_descriptor() );
  *output = *to_copy;
  return output;
}


raw_descriptor
::raw_descriptor()
{
  is_pvo_ = false;
}


raw_descriptor
::~raw_descriptor()
{
}


void
raw_descriptor
::set_type( const descriptor_id_t& type )
{
  this->type_ = type;
}


descriptor_id_t const&
raw_descriptor
::get_type() const
{
  return this->type_;
}


void
raw_descriptor
::add_track_id( track::track_id_t id )
{
  this->track_ids_.push_back( id );
}


void
raw_descriptor
::add_track_ids( const std::vector< track::track_id_t >& ids )
{
  this->track_ids_.insert( this->track_ids_.end(), ids.begin(), ids.end() );
}


std::vector< track::track_id_t > const&
raw_descriptor
::get_track_ids() const
{
  return this->track_ids_;
}


void
raw_descriptor
::set_features( descriptor_data_t const& data )
{
  this->data_ = data;
}


descriptor_data_t const&
raw_descriptor
::get_features() const
{
  return this->data_;
}


descriptor_data_t&
raw_descriptor
::get_features()
{
  return this->data_;
}


double&
raw_descriptor
::at( const size_t idx )
{
  // validate element index
  if ( idx >= this->data_.size() )
  {
    std::stringstream msg;
    msg << "Raw descriptor index " << idx
        << " is beyond the last feature element ("
        << this->data_.size() - 1 << ")";
    throw std::out_of_range( msg.str() );
  }

  return this->data_[idx];
}


double const&
raw_descriptor
::at( const size_t idx ) const
{
  // validate element index
  if ( idx >= this->data_.size() )
  {
    std::stringstream msg;
    msg << "Raw descriptor index " << idx
        << " is beyond the last feature element ("
        << this->data_.size() - 1 << ")";
    throw std::out_of_range( msg.str() );
  }

  return this->data_[idx];
}


size_t
raw_descriptor
::features_size() const
{
  return this->data_.size();
}


void
raw_descriptor
::resize_features( size_t s )
{
  this->data_.resize( s );
}


void
raw_descriptor
::resize_features( size_t s, int v )
{
  this->data_.resize( s, v );
}


bool
raw_descriptor
::has_features() const
{
  return ! ( this->data_.empty() );
}


void
raw_descriptor
::set_history( descriptor_history_t const& hist )
{
  this->history_ = hist;
}


void
raw_descriptor
::add_history_entry( descriptor_history_entry const& hist )
{
  this->history_.push_back( hist );
}


descriptor_history_t const&
raw_descriptor
::get_history() const
{
  return this->history_;
}


void
raw_descriptor
::set_pvo_flag( bool f )
{
  this->is_pvo_ = f;
}


bool
raw_descriptor
::is_pvo() const
{
  return this->is_pvo_;
}



// ================================================================
descriptor_history_entry::
descriptor_history_entry( const vidtk::timestamp& ts,
                          const image_bbox_t& img_loc,
                          const world_bbox_t& world_loc )
  : ts_(ts),
    img_loc_(img_loc),
    world_loc_( world_loc )
{
}


descriptor_history_entry::
descriptor_history_entry( const vidtk::timestamp& ts,
                          const image_bbox_t& img_loc )
  : ts_( ts ),
    img_loc_( img_loc )
{
}


descriptor_history_entry::
~descriptor_history_entry()
{
}


vidtk::timestamp
descriptor_history_entry::
get_timestamp() const
{
  return this->ts_;
}


descriptor_history_entry::image_bbox_t const&
descriptor_history_entry::
get_image_location() const
{
  return this->img_loc_;
}


descriptor_history_entry::world_bbox_t const&
descriptor_history_entry::
get_world_location() const
{
  return this->world_loc_;
}


} // end namespace

VBL_SMART_PTR_INSTANTIATE( vidtk::raw_descriptor );
