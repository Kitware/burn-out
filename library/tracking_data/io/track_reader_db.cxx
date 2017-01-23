/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_reader_db.h"

#include <logger/logger.h>

#include <boost/algorithm/string.hpp>

#include <vul/vul_file.h>

namespace  vidtk
{
  VIDTK_LOGGER("db_reader");

namespace ns_track_reader
{

//----------------------------------------------------------------------------
track_reader_db::track_reader_db()
  : is_connected_(false)
{
}

//----------------------------------------------------------------------------
track_reader_db::~track_reader_db()
{
  if (this->db_conn_)
  {
    this->db_conn_->close_connection();
  }
}

//----------------------------------------------------------------------------
bool track_reader_db::open(std::string const& filename)
{
  if (this->is_connected_)
  {
    return false;
  }

  std::string db_type = this->reader_options_.get_db_type();
  std::transform(db_type.begin(), db_type.end(), db_type.begin(), ::tolower);
  if (db_type != "postgresql" && db_type != "sqlite3")
  {
    return false;
  }

  if (!filename.empty())
  {
    LOG_WARN("Filename is not used when reading from the database."
      " Did you mean to use the db_name parameter ?");
  }

  int ret = db_connection_factory::get_connection(
    db_type,
    this->reader_options_.get_db_name(),
    this->reader_options_.get_db_user(),
    this->reader_options_.get_db_pass(),
    this->reader_options_.get_db_host(),
    this->reader_options_.get_db_port(),
    this->reader_options_.get_connection_args(),
    this->db_conn_);

  if (ret != ERR_NONE)
  {
    std::string error =
      (ret == ERR_NO_BACKEND_SUPPORT) ? "ERR_NO_BACKEND_SUPPORT" : "ERR_VERSION_UPDATED";
    LOG_ERROR("Could not create a database connection using:\n"
      "db_type: " + this->reader_options_.get_db_type() + "\n"
      "db_name_: " + this->reader_options_.get_db_name() + "\n"
      "db_user_: " + this->reader_options_.get_db_user() + "\n"
      "db_pass_: " + this->reader_options_.get_db_pass() + "\n"
      "db_host_: " + this->reader_options_.get_db_host() + "\n"
      "db_port_: " + this->reader_options_.get_db_port() + "\n"
      "connection_args_: " + this->reader_options_.get_connection_args() + "\n"
      "-> ERROR: " + error
      );
    return false;
  }

  bool connected = this->db_conn_->connect();
  if (!connected)
  {
    LOG_ERROR("Database connection failed");
    return false;
  }

  if (this->db_conn_->set_active_session(
    this->reader_options_.get_db_session_id()) != ERR_NONE)
  {
    LOG_ERROR("Error while setting the current session: "
      << this->reader_options_.get_db_session_id());
    return false;
  }

  this->is_connected_ = connected;
  this->track_db_ = this->db_conn_->get_track_db();

  return true;
}

//----------------------------------------------------------------------------
bool track_reader_db
::read_next_terminated(vidtk::track::vector_t&  datum, unsigned& frame)
{
  if (!this->is_connected_)
  {
    LOG_ERROR("Track_reader_db cannot connect to the database.");
    return false;
  }
  return this->track_db_->get_terminated_tracks(frame, datum);
}

//----------------------------------------------------------------------------
size_t track_reader_db::read_all(vidtk::track::vector_t& datum)
{
  if (!this->is_connected_)
  {
    LOG_ERROR("Track_reader_db cannot connect to the database.");
    return -1;
  }

  if (!this->track_db_->get_all_tracks(datum))
  {
    LOG_ERROR("Error while reading the tracks in the track_db.");
    return -1;
  }
  return datum.size();
}

} // end namespace
} // end namespace
