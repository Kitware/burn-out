/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <plugin_loader/plugin_manager.h>
#include <plugin_loader/plugin_factory.h>

#include "track_reader_kw18.h"
#include "track_reader_vsl.h"

#include "track_writer_kw18_col.h"
#include "track_writer_vsl.h"

#include <tracking_data/io/plugin_export.h>

extern "C"  void PLUGIN_EXPORT register_factories( vidtk::plugin_manager* pm );

// ------------------------------------------------------------------
// Register factories in this module
//
void register_factories( vidtk::plugin_manager* pm )
{
  vidtk::plugin_factory_handle_t fact;

  //                 interface type                concrete type
  fact = pm->ADD_FACTORY( vidtk::ns_track_reader::track_reader_interface, vidtk::ns_track_reader::track_reader_vsl );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "VSL track reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads VSL formatted tracks" );

  fact = pm->ADD_FACTORY( vidtk::ns_track_reader::track_reader_interface, vidtk::ns_track_reader::track_reader_kw18 );
  fact->add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "KW18 track reader" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Reads KW18 and related format tracks" );



  fact = pm->ADD_FACTORY( vidtk::track_writer_interface, vidtk::track_writer_kw18 );
  fact->add_attribute( "file-type", "columns_tracks kw18 trk dat kw20" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"kw18\" \"trk\" \"dat\" \"kw20\" - Write tracks in kw18 format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "KW18 track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes KW18 formatted tracks" );

  fact = pm->ADD_FACTORY( vidtk::track_writer_interface, vidtk:: track_writer_col );
  fact->add_attribute( "file-type", "columns_tracks_and_objects" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"columns_tracks_and_objects\" - Write tracks in column format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "KW18-COL track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes column formatted tracks" );

  fact = pm->ADD_FACTORY( vidtk::track_writer_interface, vidtk::track_writer_vsl );
  fact->add_attribute( "file-type", "vsl" )
    .add_attribute( vidtk::plugin_factory::CONFIG_HELP, "\"vsl\" - Write tracks in binary VSL format." )
    .add_attribute( vidtk::plugin_factory::PLUGIN_NAME, "VSL track writer" )
    .add_attribute( vidtk::plugin_factory::PLUGIN_DESCRIPTION, "Writes vsl formatted tracks" );
}
