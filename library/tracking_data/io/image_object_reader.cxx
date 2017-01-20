/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_reader.h"
#include "image_object_reader_dummy.h"

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include <boost/foreach.hpp>
#include <logger/logger.h>


namespace vidtk {

VIDTK_LOGGER ("image_object_reader");

// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
image_object_reader
::image_object_reader(std::string const& filename)
  : filename_(filename)
{
  plugin_factory_vector_t factory_list =
    plugin_manager::instance()->get_factories( typeid(vidtk::ns_image_object_reader::image_object_reader_interface).name() );

  BOOST_FOREACH( plugin_factory_handle_t facto, factory_list )
  {
    this->add_reader( facto->create_object< vidtk::ns_image_object_reader::image_object_reader_interface >() );
    // TBD collect help text for each reader
  }
}


image_object_reader
::~image_object_reader()
{
  //readers are smart pointers so they should be auto deleted
}


void
image_object_reader
::add_reader(ns_image_object_reader::image_object_reader_interface * reader)
{
  this->reader_list_.push_back(reader_handle_t(reader));
}


bool
image_object_reader
::open()
{
  // If there is already a reader assigned, indicate file is open.
  if (this->current_reader_)
  {
    return true;
  }

  // Scan the list of file readers to see if one can handle the file.
  BOOST_FOREACH(reader_handle_t reader, reader_list_)
  {
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
    // instantiate a dummy reader in case the caller does not stop
    // calling us after a failed open.
    this->current_reader_ = reader_handle_t( new ns_image_object_reader::image_object_reader_dummy() );
    return false;
  }

  return true;
}


size_t
image_object_reader
::read_all(object_vector_t & datum)
{
  ns_image_object_reader::datum_t local_datum;
  size_t read_count(0);
  datum.clear();

  while ( this->current_reader_->read_next(local_datum) )
  {
    datum.push_back(local_datum);
    read_count++;
  } // end while

  return read_count;
}


bool
image_object_reader
::read_next(ns_image_object_reader::datum_t& datum)
{
  return this->current_reader_->read_next(datum);
}


void
image_object_reader
::read_all(std::vector < ns_image_object_reader::ts_object_vector_t > & datum)
{
  ts_object_vector_t buffer;

  datum.clear();

  // Read image object file timestamp by timestamp
  while ( read_next_timestamp( buffer ) )
  {
    datum.push_back( buffer );
  }
}


bool
image_object_reader
::read_next_timestamp( ns_image_object_reader::ts_object_vector_t& datum )
{
  vidtk::timestamp current_ts;

  datum.second.clear();

  if ( ! this->rnt_last_read_.first.is_valid() )
  {
    if ( ! this->current_reader_->read_next( this->rnt_last_read_ ) )
    {
      // empty file
      return false;
    }
  }

  current_ts = this->rnt_last_read_.first;

  // Add the first set of objects
  datum.first = this->rnt_last_read_.first;
  datum.second.push_back( this->rnt_last_read_.second );

  bool status;
  while ( (status = read_next( this->rnt_last_read_ )) )
  {
    if ( current_ts != this->rnt_last_read_.first )
    {
      break;
    }

    // Add object to list
    datum.second.push_back( this->rnt_last_read_.second );
  } // end while

  if ( ! status )
  {
    this->rnt_last_read_.first = vidtk::timestamp();
  }

  return true;
}

} // end namespace
