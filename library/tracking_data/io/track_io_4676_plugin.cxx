/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>
#include <plugin_loader/plugin_config_util.h>

#include "track_writer_4676.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  //                 interface type                concrete type
  fact = pm->ADD_FACTORY( vidtk::track_writer_interface, vidtk::track_writer_4676_NITS );
  fact->add_attribute( "file-type", "4676" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"4676\" - Write tracks in 4676 XML format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "NITS 4676 track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes 4676 NITS formatted tracks" );

  add_config_attribute( *fact, "classification", "UNCLASSIFIED", "Classification of tracks");
  add_config_attribute( *fact, "controlsystem", "NONE", "Control system");
  add_config_attribute( *fact, "dissemination", "FOUO", "Dissemination of tracks");
  add_config_attribute( *fact, "policyname", "Default", "Policy name");
  add_config_attribute( *fact, "releaseability", "USA", "Where allowed to be released");
  add_config_attribute( *fact, "stanagVersion", "3.1", "Current stanag version");
  add_config_attribute( *fact, "nationality", "USA", "Nationality");
  add_config_attribute( *fact, "stationID", "E-SIM", "Station ID");
  add_config_attribute( *fact, "exerciseIndicator", "EXERCISE", "exercise indicator");
  add_config_attribute( *fact, "simulationIndicator", "REAL", "simulation indicator");
  add_config_attribute( *fact, "trackItemSource","EO", "Source of track points");
  add_config_attribute( *fact, "trackPointType", "AUTOMATIC ESTIMATED", "Type of track point");
  add_config_attribute( *fact, "flight_num", "0", "Flight number");
  add_config_attribute( *fact, "name", "bd", "Source");
  add_config_attribute( *fact, "output_dir", "./", "Output directory for track files; used in 4676 and Apix Shape files only");
  add_config_attribute( *fact, "cov_scale", "1.0", "Scale factor to apply to covariance values before output");
  add_config_attribute( *fact, "track_num_offset", "0", "Add an offset to the output track's trackNumber field; used for 4676 only");


  fact = pm->ADD_FACTORY( vidtk::track_writer_interface, vidtk::track_writer_4676_SPADE );
  fact->add_attribute( "file-type", "spade" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"space\" - Write tracks in spade format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "SPADE 4676 track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes SPADE 4676 formatted tracks" );

  add_config_attribute( *fact, "classification", "UNCLASSIFIED", "Classification of tracks");
  add_config_attribute( *fact, "controlsystem", "NONE", "Control system");
  add_config_attribute( *fact, "dissemination", "FOUO", "Dissemination of tracks");
  add_config_attribute( *fact, "policyname", "Default", "Policy name");
  add_config_attribute( *fact, "releaseability", "USA", "Where allowed to be released");
  add_config_attribute( *fact, "stanagVersion", "3.1", "Current stanag version");
  add_config_attribute( *fact, "nationality", "USA", "Nationality");
  add_config_attribute( *fact, "stationID", "E-SIM", "Station ID");
  add_config_attribute( *fact, "exerciseIndicator", "EXERCISE", "exercise indicator");
  add_config_attribute( *fact, "simulationIndicator", "REAL", "simulation indicator");
  add_config_attribute( *fact, "trackItemSource","EO", "Source of track points");
  add_config_attribute( *fact, "trackPointType", "AUTOMATIC ESTIMATED", "Type of track point");
  add_config_attribute( *fact, "flight_num", "0", "Flight number");
  add_config_attribute( *fact, "name", "bd", "Source");
  add_config_attribute( *fact, "output_dir", "./", "Output directory for track files; used in 4676 and Apix Shape files only");
  add_config_attribute( *fact, "cov_scale", "1.0", "Scale factor to apply to covariance values before output");
  add_config_attribute( *fact, "track_num_offset", "0", "Add an offset to the output track's trackNumber field; used for 4676 only");

}
