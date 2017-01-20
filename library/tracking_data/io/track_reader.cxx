/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_reader.h"

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include <boost/foreach.hpp>
#include <logger/logger.h>


namespace vidtk {

// Read modes needed to prevent interleaving read methods.
enum {
  MODE_READ_INIT = 0,
  MODE_READ_ALL,
  MODE_READ_NEXT
};

VIDTK_LOGGER ("track_reader");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
track_reader
::track_reader(std::string const& filename)
  : filename_(filename),
    read_mode_(MODE_READ_INIT)
{
  // Get a list of plugin factories that support our interface type.
  plugin_factory_vector_t factory_list =
    plugin_manager::instance()->get_factories( typeid(vidtk::ns_track_reader::track_reader_interface).name() );

  BOOST_FOREACH( plugin_factory_handle_t facto, factory_list )
  {
    this->add_reader( facto->create_object< vidtk::ns_track_reader::track_reader_interface >() );
  }
}


track_reader
::~track_reader()
{
}


void
track_reader
::add_reader(ns_track_reader::track_reader_interface * reader)
{
  this->reader_list_.push_back(reader_handle_t(reader));
}


bool
track_reader
::open()
{
  // If there is already a reader assigned, indicate file is open.
  if (this->current_reader_)
  {
    return true;
  }

  // Scan the list of file readers to see if one can handle the file.
  BOOST_REVERSE_FOREACH(reader_handle_t reader, this->reader_list_)
  {
    reader->update_options( this->reader_options_ );
    if (reader->open(filename_))
    {
      // file format is readable by this object
      this->current_reader_ = reader;
      break;
    }
  } // end foreach

  // If file not recognised, return failure
  if (! this->current_reader_)
  {
    return false;
  }

  return true;
}


size_t
track_reader
::read_all( vidtk::track::vector_t& datum )
{
  if (this->read_mode_ == MODE_READ_NEXT)
  {
    LOG_ERROR("Interleaving read types is not allowed");
    return 0;
  }

  this->read_mode_ = MODE_READ_ALL;
  return this->current_reader_->read_all( datum );
}


bool
track_reader
::read_next_terminated( vidtk::track::vector_t& datum, unsigned& ts )
{
  if (this->read_mode_ == MODE_READ_ALL)
  {
    LOG_ERROR("Interleaving read types is not allowed");
    return false;
  }

  this->read_mode_ = MODE_READ_NEXT;
  return this->current_reader_->read_next_terminated( datum, ts );
}


void
track_reader
::update_options( vidtk::ns_track_reader::track_reader_options const& opt )
{
  this->reader_options_.update_from( opt );
}


} // end namespace
