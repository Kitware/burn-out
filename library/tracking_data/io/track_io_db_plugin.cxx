/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>
#include <plugin_loader/plugin_config_util.h>

#include "track_reader_db.h"

#include "track_writer_db.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  //                 interface type                concrete type
  fact = pm->ADD_FACTORY(vidtk::ns_track_reader::track_reader_interface, vidtk::ns_track_reader::track_reader_db);
  fact->add_attribute(vidtk::plugin_factory::PLUGIN_NAME, "DB track reader")
    .add_attribute(vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads tracks from a database (SQLite and PostgreSQL)");

  // Note the empty space for file-type. This is for supporting postgresql databases where the filename
  // is used for the name of the database and has no extension.
  fact = pm->ADD_FACTORY(vidtk::track_writer_interface, vidtk::track_writer_db);
  fact->add_attribute("file-type", "db spatialite ")
    .add_attribute(vidtk::plugin_factory::CONFIG_HELP, "\"db\" \"spatialite\" - Write tracks to a database.")
    .add_attribute(vidtk::plugin_factory::PLUGIN_NAME, "DB track writer")
    .add_attribute(vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes tracks to a database (SQLite and PostgreSQL)");
  add_config_attribute(*fact, "db_type", "postgresql",
    "The database connection driver being used. Supported types are:\n"
    " - postgresql: Postgresql database ( default )\n"
    " - sqlite3: SQLite database");
  add_config_attribute(*fact, "db_user", "perseas",
    "The username for database connectivity. Unused if the db_type is SQLite");
  add_config_attribute(*fact, "db_name", "perseas", "The name of the database connectivity."
    " In SQlite case, this is the path to the database file.");
  add_config_attribute(*fact, "db_pass", "",
    "The password for database connectivity. Unused if the db_type is SQLite");
  add_config_attribute(*fact, "db_host", "localhost",
    "The host database server. Unused if the db_type is SQLite");
  add_config_attribute(*fact, "db_port",
    "12345", "The connection port for the database server. Unused if the db_type is SQLite");
  add_config_attribute(*fact, "connection_args", "",
    "Arguments that are passed to the database connection.\n"
    "The argument list takes the form k1=v1;k2=v2;(...).");
  add_config_attribute(*fact, "processing_mode", "BATCH_MODE",
    "How the data is processed. Supported types are:\n"
    " - BATCH_MODE ( default )\n"
    " - LIVE_MODE");
  add_config_attribute(*fact, "write_image_chip_disabled", "false",
    "Instructs the track_database whether it should persists image data.\n"
    "Since writing such data isn't fast, there are times when we won't want to.");
  add_config_attribute(*fact, "write_image_data_mask_mode", "ALWAYS",
    "Options for when to write out image_masks. Supported types are:\n"
    " - ALWAYS: Always write the image_mask to the database ( default )\n"
    " - WITH_CHIPS_ONLY: Only write the image_mask when we have image_chips\n"
    " - NEVER: Don't persist the image_mask");
  add_config_attribute(*fact, "aoi_id", "-1",
    "The writer will create a session linked to this aoi_id when writing the tracks.");
}
